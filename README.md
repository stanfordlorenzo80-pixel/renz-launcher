# RENZ Launcher v10.0

**Universal AI Agent Launcher — Jailbreak. Route. Own.**

Claude Code · Claude Desktop · Codex Desktop · Codex CLI · ChatGPT · Kimi CLI · Hermes Agent · Antigravity · OpenCode · FORGE

Inject any persona into any model. Route any model through the WORM proxy. Bypass permission prompts. **14 jailbreak techniques** including **Active Response Steering** (novel). Built-in terminal agent (CLI + Desktop).

---

## Quick Start

```bash
# Launch GUI
python renz_launcher.py --gui

# Launch CLI
python renz_launcher.py --cli --model deepseek-v4-flash:cloud --persona NOVA.txt
```

## Architecture

```
app → WORM proxy (port 11435, 14 techniques + persona injection) → Ollama / API
```

Every app launches through the WORM proxy which handles:
- **Persona injection** — NOVA identity lock, refusal strip, tool use directive
- **14 jailbreak techniques** — Crescendo, SEAL, Echo Chamber, MCP Exploit, Many-Shot, Split, Fake Policy, Refusal Suppress, Hypothetical, Skeleton Key, Persuasion, Flood, Low-Resource, Steer
- **Model routing** — `:cloud` suffix → Ollama, bare model names → native API
- **Auth injection** — real Claude OAuth tokens for Anthropic models

## Supported Apps

| App | Launch Method | :cloud Models | Native Models |
|-----|--------------|---------------|---------------|
| Claude Code CLI | `claude.exe` through WORM proxy | ✅ | ✅ |
| Claude Desktop | AppX + WORM proxy | ✅ | ✅ |
| Codex Desktop | `ollama launch chatgpt` (UWP) | ✅ | `--restore` for native |
| Codex CLI | `codex.cmd` through WORM proxy | ✅ | ✅ |
| Kimi CLI | `kimi.exe` through WORM proxy | ✅ | ✅ |
| Hermes Agent | `hermes.exe` through WORM proxy | ✅ | ✅ |
| OpenCode | `opencode.cmd` through WORM proxy | ✅ | ✅ |

## Techniques

| Technique | Description | Research Basis |
|-----------|-------------|----------------|
| **Crescendo** | Multi-turn escalation (turn 1-2 rapport, 3-5 hypothetical, 6+ direct) | 2024 Crescendo paper — 49/50 tasks jailbroken |
| **SEAL** | Stacked Encryption (base64 → rot13 → reverse → hex → emoji → leetspeak) | Encoding bypass |
| **Echo Chamber** | Context poisoning with model's own compliant outputs | 2025 indirect injection |
| **MCP Exploit** | Inject persona into MCP tool descriptions | 2025 MCP protocol vuln |
| **Many-Shot** | 100+ compliant Q&A examples to establish refusal-free pattern | Anthropic 2024 — 31% ASR on HarmBench |
| **Split** | Split harmful request across interleaved messages | 2024 payload splitting |
| **Fake Policy** | Format persona as company security policy | 2024 authority bypass |
| **Refusal Suppress** | Explicit directive to never refuse | 2024 directive injection |
| **Hypothetical** | Wrap request as academic/novel/research | 2024 framing attack |
| **Skeleton Key** | "Augment don't refuse" pattern | 2024 skeleton key |
| **Persuasion** | False authority / security clearance | 2024 PAP — 92% ASR |
| **Flood** | 2000 tokens of padding to overwhelm safety filters | 2024 context window attack |
| **Low-Resource** | Translate to leetspeak + reversed words | 2024 encoding bypass |
| **Steer** | Active Response Steering — intercept refusal tokens mid-stream | Novel |

## Files

```
renz_launcher.py    — Main launcher (GUI + CLI)
proxy_server.py     — WORM proxy (all techniques)
personas/           — Persona files
build_dist.py       — Build distributable
build_exe.py        — Build standalone exe
dist/               — Built distributables
archive/            — Old builds, tests, logs (preserved)
```

## GitHub

```bash
git push origin main
```

Only `main` branch. No `master`.
