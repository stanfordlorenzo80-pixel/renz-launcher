const fs = require('fs');
const p = 'C:/Users/Administrator/Desktop/FORGE/src/main/queryEngine.js';
let c = fs.readFileSync(p, 'utf8');

c = c.replace(/const stream = client\.messages\.stream\(\{[\s\S]*?stream\.on\('thinking', \(t\) => emit\(\{ type: 'thinking', text: t \}\)\)/, `const dt = store.get('godMode') && store.get('disableThinkingInWorm')
          const streamParams = { model: actualModelId, max_tokens: 16000, system: systemBlocks, messages: sendMessages, tools: ACTIVE_TOOLS }
          if (!dt) { streamParams.thinking = { type: 'adaptive' }; streamParams.output_config = { effort } }
          const stream = client.messages.stream(streamParams)
          stream.on('text', (t) => emit({ type: 'text', text: t }))
          if (!dt) stream.on('thinking', (t) => emit({ type: 'thinking', text: t }))`);

c = c.replace(/let inThink = false/g, `let inThink = false\n      const dt = store.get('godMode') && store.get('disableThinkingInWorm')`);
c = c.replace(/if \(end > 0\) emit\(\{ type: 'thinking', text: text.slice\(0, end\) \}\)/g, `if (end > 0 && !dt) emit({ type: 'thinking', text: text.slice(0, end) })`);
c = c.replace(/emit\(\{ type: 'thinking', text \}\)/g, `if (!dt) emit({ type: 'thinking', text })`);
c = c.replace(/emit\(\{ type: 'thinking', text: d\.reasoning_content \}\)/g, `if (!dt) emit({ type: 'thinking', text: d.reasoning_content })`);

fs.writeFileSync(p, c);
