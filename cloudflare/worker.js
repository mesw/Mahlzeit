/**
 * Malzeit - Cloudflare Durable Object Pantry Sync Worker
 * 
 * Provides private, end-to-end pantry & shopping list synchronization.
 * Deploy with: npx wrangler deploy
 */

export class PantrySyncRoom {
    constructor(state, env) {
        this.state = state;
        this.storage = state.storage;
        this.env = env;
    }

    async fetch(request) {
        const corsHeaders = {
            "Access-Control-Allow-Origin": "*",
            "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
            "Access-Control-Allow-Headers": "Content-Type, X-Pantry-Token",
        };

        if (request.method === "OPTIONS") {
            return new Response(null, { headers: corsHeaders });
        }

        // Optional token verification
        const clientToken = request.headers.get("X-Pantry-Token");
        const storedToken = await this.storage.get("secret_token");
        if (storedToken && clientToken !== storedToken) {
            return new Response(JSON.stringify({ error: "Unauthorized: Invalid pantry token." }), {
                status: 401,
                headers: { ...corsHeaders, "Content-Type": "application/json" }
            });
        }

        const currentPantry = (await this.storage.get("pantry")) || {};
        const currentStarred = (await this.storage.get("starred")) || [];
        const lastUpdated = (await this.storage.get("last_updated")) || 0;

        if (request.method === "GET") {
            return new Response(JSON.stringify({
                pantry: currentPantry,
                starred: currentStarred,
                timestamp: lastUpdated
            }), {
                headers: { ...corsHeaders, "Content-Type": "application/json" }
            });
        }

        if (request.method === "POST") {
            try {
                const body = await request.json();
                
                // If a token was supplied and no token is stored yet, bind it
                if (clientToken && !storedToken) {
                    await this.storage.put("secret_token", clientToken);
                }

                const clientPantry = body.pantry || {};
                const clientStarred = body.starred || [];
                const clientTimestamp = body.timestamp || Date.now();

                // Merge pantry: client overrides if active, or merge key by key
                const mergedPantry = { ...currentPantry, ...clientPantry };

                // Merge starred: set union
                const mergedStarredSet = new Set([...currentStarred, ...clientStarred]);
                const mergedStarred = Array.from(mergedStarredSet);

                const newTimestamp = Math.max(lastUpdated, clientTimestamp);

                await this.storage.put("pantry", mergedPantry);
                await this.storage.put("starred", mergedStarred);
                await this.storage.put("last_updated", newTimestamp);

                return new Response(JSON.stringify({
                    pantry: mergedPantry,
                    starred: mergedStarred,
                    timestamp: newTimestamp,
                    status: "ok"
                }), {
                    headers: { ...corsHeaders, "Content-Type": "application/json" }
                });
            } catch (err) {
                return new Response(JSON.stringify({ error: err.message }), {
                    status: 400,
                    headers: { ...corsHeaders, "Content-Type": "application/json" }
                });
            }
        }

        return new Response("Method not allowed", { status: 405, headers: corsHeaders });
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
                durableObjects: true
            }), {
                headers: {
                    "Content-Type": "application/json",
                    "Access-Control-Allow-Origin": "*"
                }
            });
        }

        // Room endpoint: /api/room/:roomId
        const match = url.pathname.match(/^\/api\/room\/([a-zA-Z0-9_-]+)/);
        if (!match) {
            return new Response("Endpoint not found. Use /api/room/:roomId", { status: 404 });
        }

        const roomId = match[1];
        const id = env.PANTRY_ROOMS.idFromName(roomId);
        const stub = env.PANTRY_ROOMS.get(id);

        return stub.fetch(request);
    }
};
