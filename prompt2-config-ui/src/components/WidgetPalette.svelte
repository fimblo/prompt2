<script lang="ts">
  import { widgets } from '../data/widgets';
  import { iniFile, activeSection, cursorIndex, currentTokens, selectedItem } from '../lib/stores';
  import { serializeTokens, type PromptToken } from '../lib/prompt-tokenizer';

  function insertWidget(token: string) {
    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;

      const tokens: PromptToken[] = [...$currentTokens];
      const insertAt = $cursorIndex >= 0 ? Math.min($cursorIndex, tokens.length) : tokens.length;

      tokens.splice(insertAt, 0, { type: 'widget', name: token });
      section.prompt = serializeTokens(tokens);

      cursorIndex.set(insertAt + 1);
      selectedItem.set({ kind: 'widget', name: token });

      return { ...ini };
    });
  }

  function insertNewline() {
    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;

      const tokens: PromptToken[] = [...$currentTokens];
      const insertAt = $cursorIndex >= 0 ? Math.min($cursorIndex, tokens.length) : tokens.length;

      tokens.splice(insertAt, 0, { type: 'newline' });
      section.prompt = serializeTokens(tokens);
      cursorIndex.set(insertAt + 1);

      return { ...ini };
    });
  }

  function insertText() {
    const text = prompt('Enter text to insert:');
    if (!text) return;

    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;

      const tokens: PromptToken[] = [...$currentTokens];
      const insertAt = $cursorIndex >= 0 ? Math.min($cursorIndex, tokens.length) : tokens.length;

      tokens.splice(insertAt, 0, { type: 'text', value: text });
      section.prompt = serializeTokens(tokens);
      cursorIndex.set(insertAt + 1);

      return { ...ini };
    });
  }

  function insertAttribute() {
    const attr = prompt('Enter attribute (e.g. "fg red, bold"):');
    if (attr === null) return;

    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;

      const tokens: PromptToken[] = [...$currentTokens];
      const insertAt = $cursorIndex >= 0 ? Math.min($cursorIndex, tokens.length) : tokens.length;

      tokens.splice(insertAt, 0, { type: 'attribute', value: attr });
      section.prompt = serializeTokens(tokens);
      cursorIndex.set(insertAt + 1);

      return { ...ini };
    });
  }

  // Group widgets
  const groups = ['SYS', 'Repo', 'AWS', 'Other'] as const;
  const grouped = groups.map(g => ({
    name: g,
    widgets: widgets.filter(w => w.group === g),
  }));
</script>

<div class="palette">
  <div class="palette-header">
    <span class="palette-label">Widget Palette</span>
    <div class="toolbar">
      <button class="tool-btn" onclick={insertText} title="Insert literal text">Aa</button>
      <button class="tool-btn" onclick={insertAttribute} title="Insert attribute (color/style)">{'%{}'}</button>
      <button class="tool-btn" onclick={insertNewline} title="Insert newline">{'\\n'}</button>
    </div>
  </div>

  <div class="groups">
    {#each grouped as group}
      <div class="group">
        <div class="group-label">{group.name}</div>
        <div class="group-widgets">
          {#each group.widgets as w}
            <button
              class="widget-btn"
              class:dimmed={w.gitOnly && $activeSection === 'prompt'}
              onclick={() => insertWidget(w.token)}
              title={w.description}
            >
              <span class="widget-token">{w.token}</span>
              <span class="widget-desc">{w.description}</span>
            </button>
          {/each}
        </div>
      </div>
    {/each}
  </div>
</div>

<style>
  .palette {
    background: #252535;
    border: 1px solid #333;
    border-radius: 8px;
    overflow: hidden;
  }
  .palette-header {
    background: #2a2a3e;
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 4px 12px;
  }
  .palette-label {
    color: #888;
    font-size: 0.75rem;
    text-transform: uppercase;
    letter-spacing: 0.05em;
  }
  .toolbar {
    display: flex;
    gap: 4px;
  }
  .tool-btn {
    background: rgba(255, 255, 255, 0.08);
    border: 1px solid #444;
    color: #aaa;
    padding: 2px 8px;
    border-radius: 4px;
    cursor: pointer;
    font-family: 'SF Mono', monospace;
    font-size: 0.75rem;
  }
  .tool-btn:hover {
    background: rgba(255, 255, 255, 0.15);
    color: #ddd;
  }
  .groups {
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .group-label {
    color: #777;
    font-size: 0.7rem;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    margin-bottom: 6px;
  }
  .group-widgets {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
  }
  .widget-btn {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    padding: 6px 10px;
    background: rgba(100, 180, 255, 0.08);
    border: 1px solid rgba(100, 180, 255, 0.2);
    border-radius: 6px;
    cursor: pointer;
    transition: all 0.15s;
    color: inherit;
  }
  .widget-btn:hover {
    background: rgba(100, 180, 255, 0.2);
    border-color: rgba(100, 180, 255, 0.4);
  }
  .widget-btn.dimmed {
    opacity: 0.4;
  }
  .widget-token {
    font-family: 'SF Mono', monospace;
    font-size: 0.8rem;
    color: #7cb8ff;
  }
  .widget-desc {
    font-size: 0.7rem;
    color: #888;
    margin-top: 2px;
  }
</style>
