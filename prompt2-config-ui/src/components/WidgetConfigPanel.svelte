<script lang="ts">
  import { selectedItem, iniFile, activeSection, currentTokens, cursorIndex, inactiveOverrides } from '../lib/stores';
  import { serializeTokens, type PromptToken } from '../lib/prompt-tokenizer';
  import type { WidgetConfig } from '../lib/ini-parser';
  import ColourPicker from './ColourPicker.svelte';

  // ── Helpers ────────────────────────────────────────────────────────────────

  function close() { selectedItem.set(null); }

  function getSection() {
    return $activeSection === 'prompt' ? $iniFile.prompt : $iniFile.promptGit;
  }

  // ── Widget / Defaults helpers ───────────────────────────────────────────────

  function getConfig(): WidgetConfig {
    const s = $selectedItem;
    if (!s) return {};
    if (s.kind === 'defaults') return $iniFile.widgetDefault;
    if (s.kind === 'widget') return $iniFile.widgets[s.name] ?? {};
    return {};
  }

  function updateWidgetField(field: string, value: string | number) {
    const s = $selectedItem;
    if (!s || (s.kind !== 'widget' && s.kind !== 'defaults')) return;
    iniFile.update(ini => {
      if (s.kind === 'defaults') {
        (ini.widgetDefault as Record<string, unknown>)[field] = value;
      } else {
        if (!ini.widgets[s.name]) ini.widgets[s.name] = {};
        (ini.widgets[s.name] as Record<string, unknown>)[field] = value;
      }
      return { ...ini };
    });
  }

  function setColour(field: 'colour_on' | 'colour_off', attrName: string) {
    updateWidgetField(field, `%{${attrName}}`);
  }

  function clearColour(field: 'colour_on' | 'colour_off') {
    const s = $selectedItem;
    if (!s || (s.kind !== 'widget' && s.kind !== 'defaults')) return;
    iniFile.update(ini => {
      if (s.kind === 'defaults') {
        delete (ini.widgetDefault as Record<string, unknown>)[field];
      } else {
        if (ini.widgets[s.name]) {
          delete (ini.widgets[s.name] as Record<string, unknown>)[field];
        }
      }
      return { ...ini };
    });
  }

  // ── CWD cwd_type ───────────────────────────────────────────────────────────

  const CWD_EXAMPLES: Record<string, string> = {
    home:     '~/projects/myapp',
    basename: 'myapp',
    full:     '/home/user/projects/myapp',
    git:      'projects/myapp  (from repo root)',
  };

  function getCwdType(): string {
    return getSection()?.cwd_type ?? 'home';
  }

  function setCwdType(val: string) {
    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (section) section.cwd_type = val;
      return { ...ini };
    });
  }

  // ── Text token helpers ──────────────────────────────────────────────────────

  function getTextValue(): string {
    const s = $selectedItem;
    if (s?.kind !== 'text') return '';
    const token = $currentTokens[s.index];
    return token?.type === 'text' ? token.value : '';
  }

  function setTextValue(value: string) {
    const s = $selectedItem;
    if (s?.kind !== 'text') return;
    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;
      const tokens: PromptToken[] = [...$currentTokens];
      if (tokens[s.index]?.type === 'text') tokens[s.index] = { type: 'text', value };
      section.prompt = serializeTokens(tokens);
      return { ...ini };
    });
  }

  /**
   * Wrap the selected text token in %{attr} … %{} attribute tokens.
   * Suppresses inserting a duplicate reset if one already follows.
   */
  function wrapTextWithColour(attrName: string) {
    const s = $selectedItem;
    if (s?.kind !== 'text') return;

    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;
      const tokens: PromptToken[] = [...$currentTokens];

      // Insert colour attr before the text
      tokens.splice(s.index, 0, { type: 'attribute', value: attrName });
      // Text is now at s.index + 1

      // Insert reset after the text, unless one already exists there
      const afterIdx = s.index + 2;
      const existingAfter = tokens[afterIdx];
      const alreadyReset =
        existingAfter?.type === 'attribute' &&
        (existingAfter.value.trim() === '' || existingAfter.value.trim().toLowerCase() === 'reset');

      if (!alreadyReset) {
        tokens.splice(afterIdx, 0, { type: 'attribute', value: '' });
      }

      section.prompt = serializeTokens(tokens);
      selectedItem.set({ kind: 'text', index: s.index + 1 });
      cursorIndex.set(s.index + 1);
      return { ...ini };
    });
  }

  // ── Attribute token helpers ─────────────────────────────────────────────────

  function getAttributeValue(): string {
    const s = $selectedItem;
    if (s?.kind !== 'attribute') return '';
    const token = $currentTokens[s.index];
    return token?.type === 'attribute' ? token.value : '';
  }

  function replaceAttributeValue(attrName: string) {
    const s = $selectedItem;
    if (s?.kind !== 'attribute') return;
    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;
      const tokens: PromptToken[] = [...$currentTokens];
      if (tokens[s.index]?.type === 'attribute') {
        tokens[s.index] = { type: 'attribute', value: attrName };
      }
      section.prompt = serializeTokens(tokens);
      return { ...ini };
    });
  }

  function setAttributeDirectly(value: string) {
    const s = $selectedItem;
    if (s?.kind !== 'attribute') return;
    iniFile.update(ini => {
      const section = $activeSection === 'prompt' ? ini.prompt : ini.promptGit;
      if (!section) return ini;
      const tokens: PromptToken[] = [...$currentTokens];
      if (tokens[s.index]?.type === 'attribute') {
        tokens[s.index] = { type: 'attribute', value };
      }
      section.prompt = serializeTokens(tokens);
      return { ...ini };
    });
  }

  // ── Reactive derived state ──────────────────────────────────────────────────

  let panelTitle = $derived(() => {
    const s = $selectedItem;
    if (!s) return '';
    if (s.kind === 'widget') return `Widget: ${s.name}`;
    if (s.kind === 'text') return 'Text Token';
    if (s.kind === 'attribute') return 'Attribute Token';
    if (s.kind === 'defaults') return 'Widget Defaults  [WIDGET_DEFAULT]';
    return '';
  });

  let currentConfig = $derived(getConfig());
  let isCwd = $derived($selectedItem?.kind === 'widget' && $selectedItem.name.toUpperCase() === 'CWD');
</script>

{#if $selectedItem}
<div class="panel">
  <div class="panel-header">
    <span class="panel-title">{panelTitle()}</span>
    <button class="close-btn" onclick={close}>&times;</button>
  </div>

  <div class="panel-body">

    <!-- ═══════════════════ CWD cwd_type ═══════════════════ -->
    {#if isCwd}
      <div class="section-label">Display style</div>
      <div class="radio-group">
        {#each Object.entries(CWD_EXAMPLES) as [opt, example]}
          <label class="radio-label">
            <input type="radio" name="cwd_type" value={opt}
              checked={getCwdType() === opt}
              onchange={() => setCwdType(opt)} />
            <span class="radio-key">{opt}</span>
            <span class="radio-example">{example}</span>
          </label>
        {/each}
      </div>
      <hr class="divider" />
    {/if}

    <!-- ═══════════════════ TEXT TOKEN ═══════════════════ -->
    {#if $selectedItem.kind === 'text'}
      <label class="field-group">
        <span class="field-label">Content</span>
        <input type="text" class="field-input"
          value={getTextValue()}
          oninput={(e) => setTextValue((e.target as HTMLInputElement).value)} />
      </label>
      <div class="info-box">
        Pick a color below to automatically wrap this text with
        <code>{'%{color}'}</code>&thinsp;…&thinsp;<code>{'%{}'}</code>.
      </div>
      <ColourPicker
        label="Wrap text with color"
        value=""
        onpick={wrapTextWithColour}
        onclear={() => {}}
      />

    <!-- ═══════════════════ ATTRIBUTE TOKEN ═══════════════════ -->
    {:else if $selectedItem.kind === 'attribute'}
      <div class="current-attr-row">
        <span class="field-label">Current value:</span>
        <code class="attr-code">{getAttributeValue() || '(reset)'}</code>
      </div>
      <label class="field-group">
        <span class="field-label">Edit directly <span class="hint">— press Enter to apply</span></span>
        <input type="text" class="field-input"
          value={getAttributeValue()}
          onkeydown={(e) => {
            if (e.key === 'Enter') {
              setAttributeDirectly((e.target as HTMLInputElement).value);
            }
          }} />
      </label>
      <ColourPicker
        label="Or pick a color / style"
        value={getAttributeValue() ? `%{${getAttributeValue()}}` : ''}
        onpick={replaceAttributeValue}
        onclear={() => setAttributeDirectly('')}
      />

    <!-- ═══════════════════ WIDGET / DEFAULTS ═══════════════════ -->
    {:else}
      <!-- Preview state toggle (only for individual widgets, not defaults) -->
      {#if $selectedItem?.kind === 'widget'}
        {@const name = $selectedItem.name}
        {@const isInactive = $inactiveOverrides.has(name)}
        <div class="preview-state-row">
          <span class="field-label">Preview state</span>
          <div class="state-toggle">
            <button
              class="state-btn"
              class:state-active={!isInactive}
              onclick={() => inactiveOverrides.update(s => { s.delete(name); return new Set(s); })}
            >active</button>
            <button
              class="state-btn"
              class:state-inactive={isInactive}
              onclick={() => inactiveOverrides.update(s => { s.add(name); return new Set(s); })}
            >inactive</button>
          </div>
        </div>
      {/if}

      <!-- String format fields (not shown for CWD which has no format) -->
      {#if !isCwd}
        <div class="string-fields">
          <label class="field-group">
            <span class="field-label">string_active <span class="hint">(%s = value)</span></span>
            <input type="text" class="field-input"
              value={currentConfig?.string_active ?? ''}
              oninput={(e) => updateWidgetField('string_active', (e.target as HTMLInputElement).value)}
              placeholder="%s" />
          </label>
          <label class="field-group">
            <span class="field-label">string_inactive <span class="hint">(when zero/empty)</span></span>
            <input type="text" class="field-input"
              value={currentConfig?.string_inactive ?? ''}
              oninput={(e) => updateWidgetField('string_inactive', (e.target as HTMLInputElement).value)}
              placeholder="%s" />
          </label>
          <label class="field-group narrow">
            <span class="field-label">max_width</span>
            <input type="number" class="field-input"
              value={currentConfig?.max_width ?? ''}
              oninput={(e) => updateWidgetField('max_width', parseInt((e.target as HTMLInputElement).value) || 256)}
              placeholder="256" />
          </label>
        </div>
      {/if}

      <!-- Two-column colour pickers -->
      <div class="colour-columns">
        <ColourPicker
          label="colour_on  (active state)"
          value={currentConfig?.colour_on ?? ''}
          onpick={(a) => setColour('colour_on', a)}
          onclear={() => clearColour('colour_on')}
        />
        <ColourPicker
          label="colour_off  (inactive state)"
          value={currentConfig?.colour_off ?? ''}
          onpick={(a) => setColour('colour_off', a)}
          onclear={() => clearColour('colour_off')}
        />
      </div>
    {/if}

  </div>
</div>
{/if}

<style>
  .panel {
    background: #252535;
    border: 1px solid #333;
    border-radius: 8px;
    overflow: hidden;
  }
  .panel-header {
    background: #2a2a3e;
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 6px 12px;
  }
  .panel-title {
    color: #7cb8ff;
    font-family: 'SF Mono', monospace;
    font-size: 0.85rem;
  }
  .close-btn {
    background: none; border: none; color: #888; font-size: 1.2rem; cursor: pointer;
  }
  .close-btn:hover { color: #ff5555; }

  .panel-body {
    padding: 14px;
    display: flex;
    flex-direction: column;
    gap: 12px;
    max-height: 600px;
    overflow-y: auto;
  }

  /* CWD */
  .section-label {
    font-size: 0.7rem;
    text-transform: uppercase;
    letter-spacing: 0.07em;
    color: #777;
  }
  .radio-group { display: flex; flex-direction: column; gap: 6px; }
  .radio-label {
    display: flex;
    align-items: baseline;
    gap: 10px;
    cursor: pointer;
    font-size: 0.84rem;
  }
  .radio-label input { cursor: pointer; flex-shrink: 0; }
  .radio-key { font-family: 'SF Mono', monospace; color: #ccc; min-width: 78px; }
  .radio-example { font-family: 'SF Mono', monospace; font-size: 0.75rem; color: #666; }

  .divider { border: none; border-top: 1px solid #333; margin: 0; }

  /* Text token */
  .info-box {
    background: rgba(100,180,255,0.07);
    border: 1px solid rgba(100,180,255,0.18);
    border-radius: 4px;
    padding: 7px 10px;
    font-size: 0.78rem;
    color: #9ac;
  }
  .info-box code {
    font-family: 'SF Mono', monospace;
    background: rgba(255,255,255,0.08);
    padding: 1px 4px;
    border-radius: 3px;
  }

  /* Attribute token */
  .current-attr-row {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }
  .attr-code {
    font-family: 'SF Mono', monospace;
    font-size: 0.82rem;
    color: #ffb46e;
  }

  /* Preview state toggle */
  .preview-state-row {
    display: flex;
    align-items: center;
    gap: 10px;
  }
  .state-toggle {
    display: flex;
    gap: 2px;
  }
  .state-btn {
    padding: 4px 10px;
    border: 1px solid #444;
    border-radius: 4px;
    background: rgba(255,255,255,0.04);
    color: #666;
    cursor: pointer;
    font-size: 0.75rem;
    font-family: 'SF Mono', monospace;
  }
  .state-btn.state-active {
    background: rgba(100, 220, 120, 0.15);
    color: #86efac;
    border-color: rgba(100, 220, 120, 0.35);
  }
  .state-btn.state-inactive {
    background: rgba(255, 160, 80, 0.15);
    color: #fdba74;
    border-color: rgba(255, 160, 80, 0.35);
  }

  /* Widget / Defaults */
  .string-fields {
    display: grid;
    grid-template-columns: 1fr 1fr auto;
    gap: 10px;
    align-items: start;
  }
  .field-group {
    display: flex;
    flex-direction: column;
    gap: 3px;
  }
  .field-group.narrow { width: 100px; }
  .field-label {
    font-size: 0.72rem;
    font-family: 'SF Mono', monospace;
    color: #888;
  }
  .hint { font-family: sans-serif; color: #666; font-style: italic; }
  .field-input {
    background: #1e1e2e;
    border: 1px solid #444;
    border-radius: 4px;
    color: #ccc;
    padding: 5px 8px;
    font-family: 'SF Mono', monospace;
    font-size: 0.82rem;
    width: 100%;
    box-sizing: border-box;
  }
  .field-input:focus { outline: none; border-color: #7cb8ff; }

  /* Two-column colour pickers */
  .colour-columns {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 18px;
  }
</style>
