<script lang="ts">
  import { currentTokens, iniFile, activeSection, cursorIndex, selectedItem } from '../lib/stores';
  import { serializeTokens, type PromptToken } from '../lib/prompt-tokenizer';
  import { resolveAttributeCombo, termStyleToCssString } from '../lib/ansi-to-css';

  function handleTokenClick(index: number, token: PromptToken) {
    cursorIndex.set(index);
    if (token.type === 'widget') {
      selectedItem.set({ kind: 'widget', name: token.name });
    } else if (token.type === 'text') {
      selectedItem.set({ kind: 'text', index });
    } else if (token.type === 'attribute') {
      selectedItem.set({ kind: 'attribute', index });
    } else {
      selectedItem.set(null);
    }
  }

  function handleGapClick(index: number) {
    cursorIndex.set(index);
    selectedItem.set(null);
  }

  function removeToken(index: number) {
    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;
      const tokens = [...$currentTokens];
      tokens.splice(index, 1);
      section.prompt = serializeTokens(tokens);
      return { ...ini };
    });
    selectedItem.set(null);
  }

  function getTokenPreviewStyle(token: PromptToken): string {
    if (token.type === 'attribute') {
      const style = resolveAttributeCombo(token.value);
      if (style.color) return `border-bottom: 3px solid ${style.color}`;
      if (style.backgroundColor) return `border-bottom: 3px solid ${style.backgroundColor}`;
    }
    return '';
  }

  function tokenLabel(token: PromptToken): string {
    switch (token.type) {
      case 'text': return token.value || '(empty)';
      case 'widget': return `@{${token.name}}`;
      case 'attribute': return token.value ? `%{${token.value}}` : '%{reset}';
      case 'newline': return '↵';
    }
  }

  function isSelected(index: number, token: PromptToken): boolean {
    const s = $selectedItem;
    if (!s) return false;
    if (s.kind === 'widget' && token.type === 'widget') return s.name === token.name;
    if (s.kind === 'text' && token.type === 'text') return s.index === index;
    return false;
  }
</script>

<div class="editor">
  <div class="editor-label">Prompt Tokens — click a token to configure it, click a gap <span class="gap-sample">|</span> to set insert position</div>
  <div class="token-list">
    <button
      class="gap"
      class:active={$cursorIndex === 0}
      onclick={() => handleGapClick(0)}
      aria-label="Insert position 0"
    >|</button>

    {#each $currentTokens as token, i}
      <button
        class="token token-{token.type}"
        class:selected={isSelected(i, token)}
        style={getTokenPreviewStyle(token)}
        onclick={() => handleTokenClick(i, token)}
        title={token.type === 'widget' ? 'Click to configure' : token.type === 'text' ? 'Click to edit / add color' : token.type === 'attribute' ? 'Click to edit attribute' : ''}
      >
        <span class="token-label">{tokenLabel(token)}</span>
        <span
          class="remove"
          role="button"
          tabindex="0"
          onclick={(e) => { e.stopPropagation(); removeToken(i); }}
          onkeydown={(e) => { if (e.key === 'Enter') { e.stopPropagation(); removeToken(i); }}}
          title="Remove"
        >&times;</span>
      </button>

      <button
        class="gap"
        class:active={$cursorIndex === i + 1}
        onclick={() => handleGapClick(i + 1)}
        aria-label="Insert position {i + 1}"
      >|</button>
    {/each}
  </div>
</div>

<style>
  .editor {
    background: #252535;
    border: 1px solid #333;
    border-radius: 8px;
    overflow: hidden;
  }
  .editor-label {
    background: #2a2a3e;
    color: #888;
    font-size: 0.75rem;
    padding: 4px 12px;
    letter-spacing: 0.03em;
  }
  .gap-sample {
    font-weight: bold;
    color: #7c7cff;
  }
  .token-list {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 2px;
    padding: 12px;
    min-height: 48px;
  }
  .gap {
    width: 12px;
    text-align: center;
    color: transparent;
    cursor: pointer;
    border: none;
    background: none;
    padding: 4px 0;
    font-size: 0.75rem;
    border-radius: 2px;
    transition: all 0.15s;
  }
  .gap:hover, .gap.active {
    color: #7c7cff;
    background: rgba(124, 124, 255, 0.1);
  }
  .token {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 4px 8px;
    border-radius: 4px;
    border: 1px solid #444;
    cursor: pointer;
    font-family: 'SF Mono', 'Fira Code', monospace;
    font-size: 0.85rem;
    transition: all 0.15s;
    background: none;
    color: inherit;
  }
  .token-widget {
    background: rgba(100, 180, 255, 0.15);
    color: #7cb8ff;
    border-color: rgba(100, 180, 255, 0.3);
  }
  .token-widget.selected {
    border-color: #7cb8ff;
    box-shadow: 0 0 6px rgba(100, 180, 255, 0.3);
  }
  .token-attribute {
    background: rgba(255, 180, 100, 0.15);
    color: #ffb46e;
    border-color: rgba(255, 180, 100, 0.3);
    font-size: 0.8rem;
  }
  .token-newline {
    background: rgba(150, 150, 150, 0.15);
    color: #999;
    border-color: rgba(150, 150, 150, 0.3);
  }
  .token-text {
    background: rgba(200, 200, 200, 0.1);
    color: #bbb;
    border-color: rgba(200, 200, 200, 0.2);
  }
  .token-text.selected {
    border-color: #aaa;
    box-shadow: 0 0 6px rgba(200, 200, 200, 0.2);
  }
  .token:hover {
    border-color: #666;
  }
  .token-label {
    white-space: pre;
  }
  .remove {
    color: transparent;
    font-size: 0.9rem;
    cursor: pointer;
    padding: 0 2px;
    border: none;
    background: none;
    line-height: 1;
    transition: color 0.1s;
  }
  .token:hover .remove {
    color: #666;
  }
  .remove:hover {
    color: #ff5555 !important;
  }
</style>
