<script lang="ts">
  import { previewSpans, currentTokens, selectedItem, cursorIndex, hoveredTokenIndex, terminalBg } from '../lib/stores';
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

  $: lines = splitIntoLines($previewSpans);
</script>

<div class="preview">
  <div class="preview-label">
    <span>Preview — click text to select a token</span>
    <label class="bg-picker-label" data-tooltip="Change preview background colour">
      <input class="bg-picker-input" type="color" bind:value={$terminalBg} />
      <span class="bg-picker-btn">
        <svg width="13" height="13" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true">
          <path d="M8 5a1.5 1.5 0 1 0 0-3 1.5 1.5 0 0 0 0 3m4 3a1.5 1.5 0 1 0 0-3 1.5 1.5 0 0 0 0 3M5.5 7a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0m.5 6a1.5 1.5 0 1 0 0-3 1.5 1.5 0 0 0 0 3"/>
          <path d="M16 8c0 3.15-1.866 2.585-3.567 2.07C11.42 9.763 10.465 9.473 10 10c-.603.683-.475 1.819-.351 2.92C9.826 14.495 9.996 16 8 16a8 8 0 1 1 8-8m-8 7c.611 0 .654-.171.655-.176.078-.146.124-.464.07-1.119-.014-.168-.037-.37-.061-.591-.052-.464-.112-1.005-.118-1.462-.01-.707.083-1.61.704-2.314.369-.417.845-.578 1.272-.618.404-.038.812.026 1.16.104.343.077.702.186 1.07.299C15.5 9.909 15.5 9.575 15.5 8A7.5 7.5 0 1 0 .5 8c0 3.715 2.7 6.808 6.288 7.373"/>
        </svg>
        <span class="bg-picker-swatch" style="background: {$terminalBg}"></span>
      </span>
    </label>
  </div>
  <div class="terminal" style="background: {$terminalBg}">
    {#each lines as line, i}
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
    padding: 4px 8px 4px 12px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    display: flex;
    align-items: center;
    justify-content: space-between;
  }
  .bg-picker-label {
    cursor: pointer;
    position: relative;
  }
  .bg-picker-input {
    opacity: 0;
    position: absolute;
    width: 100%;
    height: 100%;
    top: 0;
    left: 0;
    cursor: pointer;
  }
  .bg-picker-btn {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
    padding: 3px 5px 2px;
    border: 1px solid #555;
    border-radius: 4px;
    color: #aaa;
    line-height: 1;
    pointer-events: none;
  }
  .bg-picker-label:hover .bg-picker-btn {
    border-color: #888;
    color: #ccc;
  }
  .bg-picker-label[data-tooltip]::after {
    content: attr(data-tooltip);
    position: absolute;
    top: calc(100% + 6px);
    right: 0;
    background: #111827;
    color: #e0e0e0;
    font-size: 0.7rem;
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    padding: 4px 8px;
    border-radius: 4px;
    white-space: nowrap;
    pointer-events: none;
    opacity: 0;
    transition: opacity 0.15s;
    border: 1px solid #374151;
    z-index: 100;
    letter-spacing: normal;
    text-transform: none;
  }
  .bg-picker-label[data-tooltip]:hover::after {
    opacity: 1;
  }
  .bg-picker-swatch {
    width: 13px;
    height: 3px;
    border-radius: 1px;
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
  .terminal-line:last-child::after {
    content: '█';
    color: #ccc;
    animation: blink 1s step-end infinite;
  }
  @keyframes blink {
    50% { opacity: 0; }
  }
</style>
