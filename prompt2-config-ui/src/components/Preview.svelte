<script lang="ts">
  import { previewSpans } from '../lib/stores';
  import { termStyleToCssString } from '../lib/ansi-to-css';

  // Split spans into lines at newline characters
  function splitIntoLines(spans: { text: string; style: import('../lib/ansi-to-css').TermStyle }[]) {
    const lines: typeof spans[] = [[]];
    for (const span of spans) {
      if (span.text === '\n') {
        lines.push([]);
      } else {
        lines[lines.length - 1].push(span);
      }
    }
    return lines;
  }
</script>

<div class="preview">
  <div class="preview-label">Preview</div>
  <div class="terminal">
    {#each splitIntoLines($previewSpans) as line}
      <div class="terminal-line">
        {#each line as span}
          <span style={termStyleToCssString(span.style)}>{span.text}</span>
        {/each}
      </div>
    {/each}
    <span class="cursor">&#x2588;</span>
  </div>
</div>

<style>
  .preview {
    background: #1e1e2e;
    border: 1px solid #333;
    border-radius: 8px;
    overflow: hidden;
  }
  .preview-label {
    background: #2a2a3e;
    color: #888;
    font-size: 0.75rem;
    padding: 4px 12px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
  }
  .terminal {
    font-family: 'SF Mono', 'Fira Code', 'Cascadia Code', 'Menlo', monospace;
    font-size: 0.95rem;
    padding: 12px 16px;
    color: #ccc;
    line-height: 1.5;
    min-height: 2.5em;
  }
  .terminal-line {
    white-space: pre;
  }
  .cursor {
    color: #ccc;
    animation: blink 1s step-end infinite;
  }
  @keyframes blink {
    50% { opacity: 0; }
  }
</style>
