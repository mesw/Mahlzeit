/**
 * Malzeit - Cloudflare Durable Object Pantry Sync Worker
 *
 * Real-time multi-user pantry, favorites & store synchronization using
 * Cloudflare Durable Objects with the WebSocket Hibernation API
 * and Last-Write-Wins (LWW CRDT) per-item timestamps.
 *
 * Deploy with: npx wrangler deploy
 */

const CORS_HEADERS = {
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type, X-Pantry-Token, User-Agent",
};

function normalizeEpochMs(ts) {
    const n = Number(ts) || Date.now();
    // Convert seconds to milliseconds if a legacy client sent epoch seconds
    return n < 100000000000 ? n * 1000 : n;
}

export class PantrySyncRoom {
    constructor(ctx, env) {
        this.ctx = ctx;
        this.storage = ctx.storage;
        this.env = env;
    }

    async loadRoomState() {
        const pantry = (await this.storage.get("pantry")) || {};
        const pantryTimestamps = (await this.storage.get("pantry_ts")) || {};
        const starred = (await this.storage.get("starred")) || [];
        const starredTimestamps = (await this.storage.get("starred_ts")) || {};
        const activeStoreId = (await this.storage.get("active_store_id")) || "";
        const storeTimestamp = (await this.storage.get("store_ts")) || 0;
        const timestamp = (await this.storage.get("last_updated")) || 0;
        return {
            pantry,
            pantryTimestamps,
            starred,
            starredTimestamps,
            activeStoreId,
            storeTimestamp,
            timestamp
        };
    }

    async applyDeltaAndSave(body) {
        const {
            pantry,
            pantryTimestamps,
            starred,
            starredTimestamps,
            activeStoreId,
            storeTimestamp,
            timestamp
        } = await this.loadRoomState();

        const fallbackTs = normalizeEpochMs(body.timestamp);
        const mergedPantry = { ...pantry };
        const mergedPantryTs = { ...pantryTimestamps };

        // 1. If room is empty and client sent a full initial pantry seed, apply it with baseline ts = 1
        if (Object.keys(pantry).length === 0 && body.pantry && typeof body.pantry === "object") {
            for (const [key, val] of Object.entries(body.pantry)) {
                mergedPantry[key] = Boolean(val);
                if (!mergedPantryTs[key]) {
                    mergedPantryTs[key] = 1;
                }
            }
        }

        // 2. Apply explicit per-item LWW delta changes from client
        if (body.pantryDelta && typeof body.pantryDelta === "object") {
            for (const [key, entry] of Object.entries(body.pantryDelta)) {
                const isObj = entry !== null && typeof entry === "object";
                const val = isObj ? Boolean(entry.value) : Boolean(entry);
                const itemTs = isObj && Number(entry.ts) > 0 ? normalizeEpochMs(entry.ts) : fallbackTs;
                const currentItemTs = Number(mergedPantryTs[key]) || 0;

                // Last-Write-Wins (LWW): only apply if incoming edit is at least as recent as stored edit
                if (itemTs >= currentItemTs) {
                    mergedPantry[key] = val;
                    mergedPantryTs[key] = itemTs;
                }
            }
        }

        // 3. Starred recipes: seed if empty, then apply explicit LWW delta
        const starredSet = new Set(starred);
        const mergedStarredTs = { ...starredTimestamps };

        if (starred.length === 0 && Array.isArray(body.starred)) {
            for (const id of body.starred) {
                if (typeof id === "string" && id.trim().length > 0) {
                    const cleanId = id.trim();
                    starredSet.add(cleanId);
                    if (!mergedStarredTs[cleanId]) {
                        mergedStarredTs[cleanId] = 1;
                    }
                }
            }
        }

        if (body.starredDelta && typeof body.starredDelta === "object") {
            for (const [recipeId, entry] of Object.entries(body.starredDelta)) {
                const isObj = entry !== null && typeof entry === "object";
                const isStarred = isObj ? Boolean(entry.value) : Boolean(entry);
                const itemTs = isObj && Number(entry.ts) > 0 ? normalizeEpochMs(entry.ts) : fallbackTs;
                const currentItemTs = Number(mergedStarredTs[recipeId]) || 0;

                if (itemTs >= currentItemTs) {
                    if (isStarred) {
                        starredSet.add(recipeId);
                    } else {
                        starredSet.delete(recipeId);
                    }
                    mergedStarredTs[recipeId] = itemTs;
                }
            }
        }
        const mergedStarred = Array.from(starredSet);

        // 4. Active supermarket profile (LWW)
        let newStoreId = activeStoreId;
        let newStoreTs = Number(storeTimestamp) || 0;
        if (typeof body.activeStoreId === "string" && body.activeStoreId.trim().length > 0) {
            const incomingStoreTs = body.storeTimestamp ? normalizeEpochMs(body.storeTimestamp) : fallbackTs;
            if (!newStoreId || incomingStoreTs >= newStoreTs) {
                newStoreId = body.activeStoreId.trim();
                newStoreTs = incomingStoreTs;
            }
        }

        const newTimestamp = Math.max(Number(timestamp) || 0, fallbackTs);

        await this.storage.put("pantry", mergedPantry);
        await this.storage.put("pantry_ts", mergedPantryTs);
        await this.storage.put("starred", mergedStarred);
        await this.storage.put("starred_ts", mergedStarredTs);
        if (newStoreId) {
            await this.storage.put("active_store_id", newStoreId);
            await this.storage.put("store_ts", newStoreTs);
        }
        await this.storage.put("last_updated", newTimestamp);

        return {
            type: "state",
            pantry: mergedPantry,
            pantryTimestamps: mergedPantryTs,
            starred: mergedStarred,
            starredTimestamps: mergedStarredTs,
            activeStoreId: newStoreId,
            storeTimestamp: newStoreTs,
            timestamp: newTimestamp,
            status: "ok"
        };
    }

    broadcast(messageStr) {
        for (const ws of this.ctx.getWebSockets()) {
            try {
                ws.send(messageStr);
            } catch {
                // Ignore closed/broken sockets; webSocketClose/webSocketError cleans them up
            }
        }
    }

    async fetch(request) {
        if (request.method === "OPTIONS") {
            return new Response(null, { headers: CORS_HEADERS });
        }

        const url = new URL(request.url);
        const clientToken = (url.searchParams.get("token") || request.headers.get("X-Pantry-Token") || "").trim();
        const storedToken = (await this.storage.get("secret_token")) || "";

        // WebSocket upgrade (used by QWebSocket on Desktop & WebAssembly)
        if (request.headers.get("Upgrade") === "websocket") {
            const { 0: client, 1: server } = new WebSocketPair();

            // Hibernation API: Durable Object sleeps between messages (0 idle cost)
            this.ctx.acceptWebSocket(server);

            if (storedToken && clientToken !== storedToken) {
                server.send(JSON.stringify({
                    type: "error",
                    error: "Unauthorized: Invalid pantry token."
                }));
                server.close(4001, "Unauthorized");
                return new Response(null, { status: 101, webSocket: client });
            }

            if (clientToken && !storedToken) {
                await this.storage.put("secret_token", clientToken);
            }

            const state = await this.loadRoomState();
            server.send(JSON.stringify({
                type: "state",
                ...state,
                status: "ok"
            }));

            return new Response(null, { status: 101, webSocket: client });
        }

        // HTTP Fallback (GET / POST)
        if (storedToken && clientToken !== storedToken) {
            return new Response(JSON.stringify({ error: "Unauthorized: Invalid pantry token." }), {
                status: 401,
                headers: { ...CORS_HEADERS, "Content-Type": "application/json" }
            });
        }

        if (request.method === "GET") {
            const state = await this.loadRoomState();
            return new Response(JSON.stringify({
                type: "state",
                ...state,
                status: "ok"
            }), {
                headers: { ...CORS_HEADERS, "Content-Type": "application/json" }
            });
        }

        if (request.method === "POST") {
            try {
                const body = await request.json();
                if (clientToken && !storedToken) {
                    await this.storage.put("secret_token", clientToken);
                }
                const updated = await this.applyDeltaAndSave(body);
                const payload = JSON.stringify(updated);
                this.broadcast(payload);
                return new Response(payload, {
                    headers: { ...CORS_HEADERS, "Content-Type": "application/json" }
                });
            } catch (err) {
                return new Response(JSON.stringify({ error: err.message }), {
                    status: 400,
                    headers: { ...CORS_HEADERS, "Content-Type": "application/json" }
                });
            }
        }

        return new Response("Method not allowed", { status: 405, headers: CORS_HEADERS });
    }

    async webSocketMessage(ws, message) {
        try {
            const body = JSON.parse(message);

            if (body.type === "pull") {
                const state = await this.loadRoomState();
                ws.send(JSON.stringify({
                    type: "state",
                    ...state,
                    status: "ok"
                }));
                return;
            }

            const updated = await this.applyDeltaAndSave(body);
            this.broadcast(JSON.stringify(updated));
        } catch (err) {
            try {
                ws.send(JSON.stringify({ type: "error", error: err.message || "Invalid message" }));
            } catch {
                // Ignore send errors
            }
        }
    }

    async webSocketClose(ws, code, reason) {
        try {
            ws.close(code, reason);
        } catch {
            // Already closed
        }
    }

    async webSocketError(ws) {
        try {
            ws.close(1011, "WebSocket error");
        } catch {
            // Already closed
        }
    }
}

export default {
    async fetch(request, env) {
        const url = new URL(request.url);

        // Health check endpoint
        if (url.pathname === "/" || url.pathname === "/health") {
            return new Response(JSON.stringify({
                app: "Malzeit Pantry Sync Worker",
                status: "healthy",
                durableObjects: true,
                webSockets: true
            }), {
                headers: {
                    "Content-Type": "application/json",
                    ...CORS_HEADERS
                }
            });
        }

        // Room endpoint: /api/room/:roomId
        const match = url.pathname.match(/^\/api\/room\/([a-zA-Z0-9_-]+)/);
        if (!match) {
            return new Response("Endpoint not found. Use /api/room/:roomId", {
                status: 404,
                headers: CORS_HEADERS
            });
        }

        const roomId = match[1];
        const id = env.PANTRY_ROOMS.idFromName(roomId);
        const stub = env.PANTRY_ROOMS.get(id);

        return stub.fetch(request);
    }
};
