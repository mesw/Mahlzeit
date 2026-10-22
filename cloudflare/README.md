# Malzeit Cloudflare Durable Object Setup

Malzeit is built strictly with a **Privacy-First** architecture: by default, all your data (pantry stock, shopping lists, starred recipes) is saved strictly locally inside your browser or device.

If you wish to share or synchronize your pantry and shopping list with family members or across multiple devices, you can deploy your own private **Cloudflare Durable Object** in less than 3 minutes using Cloudflare's free tier.

---

## 3-Minute Deployment Guide

### Prerequisites
1. A free [Cloudflare Account](https://dash.cloudflare.com/sign-up).
2. [Node.js](https://nodejs.org/) installed on your computer.

### Step 1: Install Wrangler & Log In
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
Wrangler will output your unique worker URL, for example:
```
https://malzeit-pantry-sync.<your-subdomain>.workers.dev
```

### Step 3: Enter Details in Malzeit App
Open **Malzeit** and go to **Settings (Einstellungen)** -> **Cloudflare Synchronisation**:
1. **Worker URL**: Enter your deployed URL (e.g. `https://malzeit-pantry-sync.<your-subdomain>.workers.dev`).
2. **Raum-ID (Room ID)**: Enter a shared family room name (e.g. `familie-mueller-2026`).
3. **Sync-Token (Optional)**: Set a secret passphrase so only your devices can access this room.
4. Tap **"Jetzt synchronisieren"**.

Both you and your family members can now enter the same Room ID and Token on any phone or desktop to stay in sync!
