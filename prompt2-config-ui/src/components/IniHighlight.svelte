<script lang="ts">
  let { text }: { text: string } = $props();

  type TokenType = 'section' | 'key' | 'widget' | 'attribute' | 'value' | 'comment';
  interface Token { text: string; type: TokenType; }

  /** Scan a value string, pulling out @{widget} and %{attr} tokens. */
  function tokenizeValue(str: string): Token[] {
    const out: Token[] = [];
    let i = 0;
    let buf = '';
    const flush = () => { if (buf) { out.push({ text: buf, type: 'value' }); buf = ''; } };
    while (i < str.length) {
      if (str[i] === '@' && str[i + 1] === '{') {
        flush();
        const end = str.indexOf('}', i + 2);
        if (end !== -1) { out.push({ text: str.slice(i, end + 1), type: 'widget' }); i = end + 1; continue; }
      }
      if (str[i] === '%' && str[i + 1] === '{') {
        flush();
        const end = str.indexOf('}', i + 2);
        if (end !== -1) { out.push({ text: str.slice(i, end + 1), type: 'attribute' }); i = end + 1; continue; }
      }
      buf += str[i++];
    }
    flush();
    return out;
  }

  function tokenize(src: string): Token[] {
    const out: Token[] = [];
    const lines = src.split('\n');
    for (let li = 0; li < lines.length; li++) {
      if (li > 0) out.push({ text: '\n', type: 'value' });
      const line = lines[li];
      if (!line.trim()) continue;
      const trimmed = line.trimStart();
      if (trimmed.startsWith(';') || trimmed.startsWith('#')) {
        out.push({ text: line, type: 'comment' }); continue;
      }
      if (trimmed.startsWith('[')) {
        out.push({ text: line, type: 'section' }); continue;
      }
      const eq = line.indexOf('=');
      if (eq !== -1) {
        out.push({ text: line.slice(0, eq), type: 'key' });
        out.push({ text: '=', type: 'value' });
        out.push(...tokenizeValue(line.slice(eq + 1)));
        continue;
      }
      out.push({ text: line, type: 'value' });
    }
    return out;
  }

  const tokens = $derived(tokenize(text));
</script>

<pre class="ini-highlight">{#each tokens as tok}<span class="t-{tok.type}">{tok.text}</span>{/each}</pre>

<style>
  .ini-highlight {
    font-family: 'SF Mono', 'Fira Code', 'Cascadia Code', 'Menlo', monospace;
    font-size: 0.78rem;
    line-height: 1.5;
    background: #1e1e2e;
    color: #bbb;
    padding: 10px 12px;
    margin: 0;
    white-space: pre-wrap;
    overflow-y: auto;
    box-sizing: border-box;
    width: 100%;
  }

  /* Section headings  [PROMPT], [CWD], … */
  .t-section  { color: #c084fc; font-weight: 600; }

  /* Variable names  prompt=, colour_on=, … */
  .t-key      { color: #86efac; }

  /* @{widget} tokens */
  .t-widget   { color: #7cb8ff; }

  /* %{attr} tokens */
  .t-attribute { color: #ffb46e; }

  /* Plain values, punctuation, newlines */
  .t-value    { color: #bbb; }

  /* Comments */
  .t-comment  { color: #555; font-style: italic; }
</style>
