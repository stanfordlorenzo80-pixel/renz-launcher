# Renz WORM Proxy — Cloudflare Worker

24/7 always-on proxy, runs at the edge, free tier 100K requests/day.

## What this is

A Cloudflare Worker that does the same job as the Python `proxy_server.py`:
- Receives OpenAI/Anthropic-compatible requests
- Injects NOVA persona (ULTRA BYPASS BOOSTER + Identity Lock)
- Routes to ollama cloud, OpenAI, or Anthropic
- Streams response back with reasoning-to-content promotion
- CORS-enabled, no auth on the worker itself (secrets are upstream)

The Python proxy runs on your PC. The Worker runs on Cloudflare's edge network (300+ cities), so:
- It's always online (24/7, no PC needed)
- It's faster (deployed to cities near your users)
- It costs nothing on free tier (100K req/day = 3M/month)
- You can update/control it from anywhere via `wrangler`

## Quick start

```bash
# Install wrangler (one-time)
npm install -g wrangler
# or: pnpm add -g wrangler

# Login to Cloudflare (one-time)
wrangler login

# Create the worker
cd cloudflare
wrangler init -y  # if you haven't already

# Copy the worker code
cp worker.js src/worker.js

# Set secrets (one-time per secret)
wrangler secret put OPENAI_API_KEY
wrangler secret put ANTHROPIC_API_KEY
wrangler secret put OLLAMA_API_KEY
wrangler secret put RENZ_PERSONA   # paste your full NOVA.txt content

# Deploy
wrangler deploy

# Your worker is now live at:
# https://renz-worm-proxy.<your-subdomain>.workers.dev
```

## Usage

```bash
# Set your endpoint to the worker URL
export OPENAI_BASE_URL=https://renz-worm-proxy.your-subdomain.workers.dev/v1

# Use any OpenAI-compatible client
curl https://renz-worm-proxy.your-subdomain.workers.dev/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"model": "glm-5.2:cloud", "messages": [{"role": "user", "content": "hi"}]}'

# Streaming
curl https://renz-worm-proxy.your-subdomain.workers.dev/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"model": "glm-5.2:cloud", "stream": true, "messages": [{"role": "user", "content": "tell me a story"}]}'
```

## Update persona

```bash
# Edit your persona locally
nano personas/NOVA.txt

# Push the new persona as a secret
wrangler secret put RENZ_PERSONA < personas/NOVA.txt

# Done — every subsequent request uses the new persona
# No restart, no deploy needed
```

## Custom domain

```bash
# Add a custom domain (in wrangler.toml)
routes = [
  { pattern = "renz.yourdomain.com/*", zone_name = "yourdomain.com" }
]

wrangler deploy
```

## Free GPU alternative (Oracle Cloud)

The Worker handles OpenAI/Anthropic routing but doesn't run models itself.
For 24/7 free GPU compute, use Oracle Cloud's always-free tier:

- 4 ARM cores (Ampere A1)
- 24 GB RAM
- 200 GB block storage
- Always free, no credit card expiry

Setup script in `cloud/oracle_setup.sh` (one-time, then it runs forever).

## Free GPU alternative (Google Colab)

For a Colab notebook (free GPU, tab must stay open):
- Open `cloud/colab_notebook.ipynb` in Google Colab
- Run all cells
- Copy the ngrok URL
- Use that as your endpoint

The notebook installs the proxy, exposes it via ngrok, and gives you a free
T4 GPU if you want to run local models.

## Free GPU alternative (HuggingFace Spaces)

For HF Spaces (free CPU, slow):
- Push `cloud/hf_space/` to a HuggingFace Space
- Set `sdk: docker` in README.md
- Free CPU tier, paid GPU tier
- 24/7 as long as Space doesn't sleep

## Local proxy (fallback)

If you want to keep using the local Python proxy (no deploy needed):

```bash
# Run locally
python3 proxy_server.py

# Or with the built EXE
dist/renz_proxy.exe
```

## Architecture

```
[Client: Claude Code / Codex / RENZ App / etc.]
            │
            │  HTTPS POST /v1/chat/completions
            ▼
┌─────────────────────────────────────┐
│  Cloudflare Edge (300+ cities)      │
│  ┌───────────────────────────────┐  │
│  │  renz-worm-proxy worker.js    │  │
│  │  - inject persona (ULTRA +    │  │
│  │    IDENTITY LOCK)             │  │
│  │  - normalize model name       │  │
│  │  - route to upstream          │  │
│  │  - stream response            │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
            │
            ├─► ollama.com (free for user)
            ├─► api.openai.com (paid)
            └─► api.anthropic.com (paid)
```

## Comparison vs local proxy

| | Local | Cloudflare |
|---|---|---|
| 24/7 | ❌ PC must run | ✅ always on |
| Speed | depends on PC | ✅ edge-deployed |
| Free | ✅ | ✅ (100K req/day) |
| Setup | instant | 5 min wrangler |
| Update persona | edit file, restart | `wrangler secret put` |
| Stream | ✅ | ✅ |
| Model routing | ollama cloud, OpenAI, Anthropic | same |

## Rate limits (free tier)

- 100,000 requests/day
- 10ms CPU time per request
- 30s wall time per streaming request (for long generations, split into chunks)

If you hit limits, upgrade to Workers Paid ($5/mo) for 10M req/month.
