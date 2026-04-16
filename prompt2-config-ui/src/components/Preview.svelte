<script lang="ts">
  import { previewSpans, currentTokens, selectedItem, cursorIndex, hoveredTokenIndex } from '../lib/stores';
  import { termStyleToCssString } from '../lib/ansi-to-css';
  import type { StyledSpan } from '../lib/preview-renderer';

  // Split spans into lines at newline characters
  function splitIntoLines(spans: StyledSpan[]) {
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

  function handleSpanClick(span: StyledSpan) {
    if (span.tokenIndex === undefined) return;
    const token = $currentTokens[span.tokenIndex];
    if (!token) return;
    cursorIndex.set(span.tokenIndex);
    if (token.type === 'widget') {
      selectedItem.set({ kind: 'widget', name: token.name });
    } else if (token.type === 'text') {
      selectedItem.set({ kind: 'text', index: span.tokenIndex });
    } else if (token.type === 'attribute') {
      selectedItem.set({ kind: 'attribute', index: span.tokenIndex });
    }
  }

  function isSpanSelected(span: StyledSpan): boolean {
    if (span.tokenIndex === undefined) return false;
    const s = $selectedItem;
    if (!s) return false;
    const token = $currentTokens[span.tokenIndex];
    if (!token) return false;
    if (s.kind === 'widget' && token.type === 'widget') return s.name === token.name;
    if (s.kind === 'text' && token.type === 'text') return s.index === span.tokenIndex;
    if (s.kind === 'attribute' && token.type === 'attribute') return s.index === span.tokenIndex;
    return false;
  }

  function spanTitle(span: StyledSpan): string {
    if (span.tokenIndex === undefined) return '';
    const token = $currentTokens[span.tokenIndex];
    if (!token) return '';
    if (token.type === 'widget') return `@{${token.name}} — click to configure`;
    if (token.type === 'text') return 'Text — click to edit';
    if (token.type === 'attribute') return `%{${token.value}} — click to edit attribute`;
    return '';
  }

  function isClickable(span: StyledSpan): boolean {
    return span.tokenIndex !== undefined;
  }

  function isSpanHovered(span: StyledSpan): boolean {
    return span.tokenIndex !== undefined && span.tokenIndex === $hoveredTokenIndex;
  }
</script>

<div class="preview">
  <div class="preview-label">Preview — click text to select a token</div>
  <div class="terminal">
    {#each splitIntoLines($previewSpans) as line}
      <div class="terminal-line">
        {#each line as span}
          <!-- svelte-ignore a11y-click-events-have-key-events a11y-no-static-element-interactions -->
          <span
            style={termStyleToCssString(span.style)}
            class:clickable={isClickable(span)}
            class:selected={isSpanSelected(span)}
            class:hovered={isSpanHovered(span)}
            title={spanTitle(span)}
            onclick={() => handleSpanClick(span)}
          >{span.text}</span>
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
  .clickable {
    cursor: pointer;
    border-radius: 2px;
  }
  .clickable:hover {
    outline: 1px solid rgba(255, 255, 255, 0.25);
    outline-offset: 1px;
  }
  .selected {
    outline: 1px solid rgba(124, 124, 255, 0.7);
    outline-offset: 1px;
    background: rgba(124, 124, 255, 0.15);
    border-radius: 2px;
  }
  .hovered:not(.selected) {
    outline: 1px solid rgba(255, 255, 255, 0.2);
    outline-offset: 1px;
    background: rgba(255, 255, 255, 0.06);
    border-radius: 2px;
  }
  .cursor {
    color: #ccc;
    animation: blink 1s step-end infinite;
  }
  @keyframes blink {
    50% { opacity: 0; }
  }
</style>
