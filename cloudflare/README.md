# Malzeit Cloudflare Durable Object Setup

Malzeit is built strictly with a **Privacy-First** architecture: by default, all your data (pantry stock, shopping lists, starred recipes, active supermarket) is saved strictly locally inside your browser or device.

If you wish to share and synchronize your pantry and shopping list with household members or across multiple devices, you can deploy your own private **Cloudflare Durable Object** (backed by SQLite on Cloudflare's free tier) in less than 3 minutes.

---

## How Real-Time Multi-User Sync Works

Once your **Worker URL** and **Room ID** are configured in the app (and *"Automatisch im Hintergrund synchronisieren"* is enabled), `SyncManager` keeps all household devices in sync in real time via **`QWebSocket` + Cloudflare's WebSocket Hibernation API**:

1. **WebSocket Connection & Hibernation (`wss://<worker>/api/room/:roomId`)**:
   - On startup, the client connects via `QWebSocket` (`wss://`). Because WebSockets do not trigger browser CORS preflight (`OPTIONS`) checks on custom HTTP headers, this works seamlessly when hosted on GitHub Pages (Qt WebAssembly) as well as on Desktop and Mobile.
   - The Durable Object accepts the socket using `this.ctx.acceptWebSocket(server)` (Hibernation API), immediately sends the current room state (`pantry`, `starred`, `activeStoreId`), and hibernates between messages with zero idle CPU/duration billing.
   - If the connection drops, `SyncManager` automatically reconnects every **3 seconds**.
2. **Real-Time Delta-Push, Persistent Offline Queue & Last-Write-Wins (LWW CRDT)**:
   - Whenever you toggle an ingredient in the Pantry, mark an item as *"Gekauft ✓"* on the Shopping List, star/unstar a recipe, or switch the active supermarket, the app records `{ value, ts }` (epoch milliseconds) and persists the pending delta in `QSettings` + browser `localStorage`.
   - Even if you are offline in a supermarket and close/reload the browser tab, your queued offline edits survive and are automatically pushed as soon as the WebSocket reconnects. Pending deltas are only removed locally after the server acknowledges a state with `serverItemTs >= localPendingTs`.
   - The Durable Object merges each key using **Last-Write-Wins (LWW)** per-item timestamps (`pantry_ts`, `starred_ts`, `store_ts`) and broadcasts the updated state to all connected household devices (`this.ctx.getWebSockets()`) in real time:
     - Edits to **different items** merge without conflict.
     - Concurrent/offline edits to the **exact same item** deterministically keep whichever edit occurred later in real time (`itemTs >= currentItemTs`).

---

## 3-Minute Deployment Guide

### Prerequisites
1. A free [Cloudflare Account](https://dash.cloudflare.com/sign-up).
2. [Node.js](https://nodejs.org/) installed on your computer.

### Step 1: Log In to Wrangler
Open your terminal inside this `cloudflare/` directory:
```bash
npx wrangler login
```
A browser window will open asking you to authorize Cloudflare Wrangler.

### Step 2: Deploy the Worker
Run:
```bash
npx wrangler deploy
```
> **Note:** `wrangler.toml` uses `new_sqlite_classes = ["PantrySyncRoom"]` as required by Cloudflare's SQLite-backed Durable Objects on the free and paid plans.

Wrangler will output your unique worker URL, for example:
```text
https://malzeit-pantry-sync.<your-subdomain>.workers.dev
```

### Step 3: Enter Details in the Malzeit App
Open **Malzeit** and go to **Einstellungen (Settings)** -> **Cloudflare Vorratskammer-Sync**:
1. **Worker URL**: Enter your deployed URL (e.g. `https://malzeit-pantry-sync.<your-subdomain>.workers.dev`).
2. **Gemeinsame Raum-ID (Room ID)**: Enter a shared household room name (e.g. `familie-mueller-2026`).
3. **Geheimer Token (Optional)**: Set a secret passphrase so only your household devices can access this room (bound on the first sync).
4. Ensure **"Automatisch im Hintergrund synchronisieren"** is enabled and tap **"🔄 Jetzt synchronisieren"** once to verify the connection.

All devices using the same Worker URL, Room ID, and Token will now stay in sync automatically!
