<script lang="ts">
  import { derived } from 'svelte/store';
  import Preview from './components/Preview.svelte';
  import PromptEditor from './components/PromptEditor.svelte';
  import WidgetPalette from './components/WidgetPalette.svelte';
  import WidgetConfigPanel from './components/WidgetConfigPanel.svelte';
  import IniHighlight from './components/IniHighlight.svelte';
  import { iniFile, activeSection, selectedItem, terminalBg } from './lib/stores';
  import { parseIni, serializeIni } from './lib/ini-parser';

  // ── Default config loaded on startup ──────────────────────────────────────
  const DEFAULT_CONFIG = `[SYSTEM]
extra_backslash=false

[PROMPT]
prompt="@{SYS.username}@@{SYS.hostname}:@{CWD}\\n@{SYS.promptchar} "
cwd_type="home"

[PROMPT.GIT]
prompt="<@{Repo.name}><@{Repo.branch_name}> [@{Repo.behind},@{Repo.ahead}|@{Repo.staged},@{Repo.modified},@{Repo.untracked}] @{CWD}\\n@{SYS.promptchar} "
cwd_type="git"

[WIDGET_DEFAULT]
string_active="%s"
string_inactive="%s"
colour_on="%{fg-darkOrchid}"
colour_off="%{fg cyan,dim}"
max_width=200

[CWD]
colour_on="%{fg blue}"

[Repo.name]
string_inactive="-"
colour_on="%{fg cyan}"
colour_off="%{fg cyan, dim}"

[Repo.branch_name]
string_inactive="-"
colour_on="%{fg cyan}"
colour_off="%{fg cyan, dim}"
max_width=20

[Repo.staged]
colour_on="%{fg magenta}"
[Repo.modified]
colour_on="%{fg magenta}"
[Repo.untracked]
colour_on="%{fg magenta}"
[Repo.ahead]
colour_on="%{fg magenta}"
[Repo.behind]
colour_on="%{fg magenta}"
`;

  // ── Restore persisted state from localStorage ─────────────────────────────
  const LS_ORIGINAL = 'prompt2_original_ini';
  const LS_EDITOR   = 'prompt2_editor_ini';
  const LS_BG       = 'prompt2_preview_bg';

  const savedOriginal = localStorage.getItem(LS_ORIGINAL);
  const savedEditor   = localStorage.getItem(LS_EDITOR);
  const savedBg       = localStorage.getItem(LS_BG);

  if (savedBg) terminalBg.set(savedBg);

  // ── Left column state ──────────────────────────────────────────────────────
  let originalText = $state(savedOriginal ?? DEFAULT_CONFIG);
  let parseError = $state('');

  // Initialise iniFile from saved editor state, falling back to originalText
  if (savedEditor) {
    try { iniFile.set(parseIni(savedEditor)); } catch { iniFile.set(parseIni(originalText)); }
  } else {
    iniFile.set(parseIni(originalText));
  }

  // Live-updated INI derived from the editor state
  const updatedIni = derived(iniFile, ($ini) => serializeIni($ini));

  // ── Persist state to localStorage on every change ─────────────────────────
  $effect(() => { localStorage.setItem(LS_ORIGINAL, originalText); });
  $effect(() => { localStorage.setItem(LS_EDITOR,   $updatedIni); });
  $effect(() => { localStorage.setItem(LS_BG,       $terminalBg); });

  function loadConfig() {
    try {
      iniFile.set(parseIni(originalText));
      parseError = '';
    } catch (e) {
      parseError = String(e);
    }
  }

  function downloadConfig() {
    const blob = new Blob([$updatedIni], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'dot.prompt2_config.ini';
    a.click();
    URL.revokeObjectURL(url);
  }

  // ── Maximised pane ────────────────────────────────────────────────────────
  let maximizedPane = $state<null | 'original' | 'updated'>(null);

  // ── Reset ─────────────────────────────────────────────────────────────────
  function resetToDefault() {
    originalText = DEFAULT_CONFIG;
    parseError = '';
    iniFile.set(parseIni(DEFAULT_CONFIG));
    terminalBg.set('#1e1e2e');
  }

  // ── Section / defaults toolbar ─────────────────────────────────────────────
  function toggleSection(s: 'prompt' | 'promptGit') {
    activeSection.set(s);
    if (s === 'promptGit') {
      iniFile.update(ini => {
        if (!ini.promptGit) ini.promptGit = { prompt: '' };
        return { ...ini };
      });
    }
  }
</script>

<div class="app">
  <header>
    <h1>prompt2 Config Editor</h1>
  </header>

  <div class="main-grid">

    <!-- ══ Row 1, Left: reset (shares row height with toolbar) ══ -->
    <div class="left-toolbar">
      <button class="reset-btn" onclick={resetToDefault}>Reset to default</button>
    </div>

    <!-- ══ Row 1, Right: section toolbar ══ -->
    <div class="toolbar">
      <div class="section-toggle">
        <button
          class="tog-btn"
          class:active={$activeSection === 'prompt'}
          onclick={() => toggleSection('prompt')}
        >PROMPT</button>
        <button
          class="tog-btn"
          class:active={$activeSection === 'promptGit'}
          onclick={() => toggleSection('promptGit')}
        >PROMPT.GIT</button>
      </div>
      <button
        class="tog-btn macos-btn"
        class:active={$iniFile.system?.extra_backslash === true}
        title="extra_backslash — enable on macOS (iniparser 4.2.x)"
        onclick={() => iniFile.update(ini => ({
          ...ini,
          system: { extra_backslash: !(ini.system?.extra_backslash ?? false) }
        }))}
      >macOS</button>
      <button
        class="defaults-btn"
        class:active={$selectedItem?.kind === 'defaults'}
        onclick={() => selectedItem.set(
          $selectedItem?.kind === 'defaults' ? null : { kind: 'defaults' }
        )}
      >Widget Defaults</button>
    </div>

    <!-- ══ Row 2, Left: INI column ══ -->
    <div class="ini-column">

      <div class="ini-pane">
        <div class="pane-header">
          <span>Original INI</span>
          <span class="pane-hint">paste your config, then Load</span>
          <button class="expand-btn" onclick={() => maximizedPane = 'original'} data-tooltip="Expand to full screen">
            <svg width="11" height="11" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M5.828 10.172a.5.5 0 0 0-.707 0l-4.096 4.096V11.5a.5.5 0 0 0-1 0v3.975a.5.5 0 0 0 .5.5H4.5a.5.5 0 0 0 0-1H1.732l4.096-4.096a.5.5 0 0 0 0-.707zm4.344 0a.5.5 0 0 1 .707 0l4.096 4.096V11.5a.5.5 1 0 1 1 0v3.975a.5.5 0 0 1-.5.5H11.5a.5.5 0 0 1 0-1h2.768l-4.096-4.096a.5.5 0 0 1 0-.707zm0-4.344a.5.5 0 0 0 .707 0l4.096-4.096V4.5a.5.5 1 0 0 1 0V.525a.5.5 0 0 0-.5-.5H11.5a.5.5 0 0 0 0 1h2.768l-4.096 4.096a.5.5 0 0 0 0 .707zm-4.344 0a.5.5 0 0 1-.707 0L1.025 1.732V4.5a.5.5 0 0 1-1 0V.525a.5.5 0 0 1 .5-.5H4.5a.5.5 0 0 1 0 1H1.732l4.096 4.096a.5.5 0 0 1 0 .707z"/></svg>
          </button>
        </div>
        <textarea
          class="ini-textarea"
          bind:value={originalText}
          spellcheck="false"
          placeholder="Paste your ~/.prompt2_config.ini here…"
        ></textarea>
        {#if parseError}
          <div class="parse-error">{parseError}</div>
        {/if}
        <button class="load-btn" onclick={loadConfig}>Load config</button>
      </div>

      <div class="ini-pane">
        <div class="pane-header">
          <span>Updated INI</span>
          <span class="pane-hint">live — reflects your edits</span>
          <button class="expand-btn" onclick={() => maximizedPane = 'updated'} data-tooltip="Expand to full screen">
            <svg width="11" height="11" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M5.828 10.172a.5.5 0 0 0-.707 0l-4.096 4.096V11.5a.5.5 0 0 0-1 0v3.975a.5.5 0 0 0 .5.5H4.5a.5.5 0 0 0 0-1H1.732l4.096-4.096a.5.5 0 0 0 0-.707zm4.344 0a.5.5 0 0 1 .707 0l4.096 4.096V11.5a.5.5 1 0 1 1 0v3.975a.5.5 0 0 1-.5.5H11.5a.5.5 0 0 1 0-1h2.768l-4.096-4.096a.5.5 0 0 1 0-.707zm0-4.344a.5.5 0 0 0 .707 0l4.096-4.096V4.5a.5.5 1 0 0 1 0V.525a.5.5 0 0 0-.5-.5H11.5a.5.5 0 0 0 0 1h2.768l-4.096 4.096a.5.5 0 0 0 0 .707zm-4.344 0a.5.5 0 0 1-.707 0L1.025 1.732V4.5a.5.5 0 0 1-1 0V.525a.5.5 0 0 1 .5-.5H4.5a.5.5 0 0 1 0 1H1.732l4.096 4.096a.5.5 0 0 1 0 .707z"/></svg>
          </button>
        </div>
        <IniHighlight text={$updatedIni} />
        <button class="download-btn" onclick={downloadConfig}>
          Download dot.prompt2_config.ini
        </button>
      </div>

    </div>

    <!-- ══ Row 2, Right: editor components ══ -->
    <div class="editor-column">
      <Preview />
      <PromptEditor />
      <WidgetConfigPanel />
      <WidgetPalette />
    </div>

  </div>
</div>

{#if maximizedPane}
  <!-- svelte-ignore a11y-click-events-have-key-events a11y-no-static-element-interactions -->
  <div class="modal-backdrop" onclick={() => maximizedPane = null}>
    <div class="modal-pane" onclick={(e) => e.stopPropagation()}>
      <div class="modal-header">
        <span>{maximizedPane === 'original' ? 'Original INI' : 'Updated INI'}</span>
        {#if maximizedPane === 'original'}
          <span class="modal-hint">paste your config, then Load</span>
        {:else}
          <span class="modal-hint">live — reflects your edits</span>
        {/if}
        <button class="modal-close" onclick={() => maximizedPane = null}>&times;</button>
      </div>

      {#if maximizedPane === 'original'}
        <textarea class="modal-textarea" bind:value={originalText} spellcheck="false"></textarea>
        {#if parseError}<div class="parse-error modal-error">{parseError}</div>{/if}
        <div class="modal-actions">
          <button class="load-btn" onclick={loadConfig}>Load config</button>
        </div>
      {:else}
        <IniHighlight text={$updatedIni} />
        <div class="modal-actions">
          <button class="download-btn" onclick={downloadConfig}>Download dot.prompt2_config.ini</button>
        </div>
      {/if}
    </div>
  </div>
{/if}

<style>
  .app {
    max-width: 1280px;
    margin: 0 auto;
    padding: 20px 24px;
  }

  header { margin-bottom: 18px; }
  h1 { color: #e0e0e0; font-size: 1.4rem; margin: 0; }

  /* ── Main grid: 2 cols × 2 rows ── */
  .main-grid {
    display: grid;
    grid-template-columns: 340px 1fr;
    column-gap: 20px;
    row-gap: 14px;
    align-items: start;
  }

  /* ── Row 1, Left: reset button ── */
  .left-toolbar {
    display: flex;
    align-items: center;
  }
  .reset-btn {
    padding: 6px 14px;
    border: 1px solid #444;
    border-radius: 4px;
    background: rgba(255, 255, 255, 0.05);
    color: #888;
    cursor: pointer;
    font-family: 'SF Mono', monospace;
    font-size: 0.8rem;
  }
  .reset-btn:hover {
    border-color: #ff5555;
    color: #ff5555;
    background: rgba(255, 85, 85, 0.08);
  }

  /* ── Row 2, Left: INI column ── */
  .ini-column {
    display: flex;
    flex-direction: column;
    gap: 16px;
    position: sticky;
    top: 20px;
  }

  .ini-pane {
    display: flex;
    flex-direction: column;
    gap: 8px;
    background: #252535;
    border: 1px solid #333;
    border-radius: 8px;
    overflow: hidden;
    padding: 0 0 10px;
  }

  .pane-header {
    background: #2a2a3e;
    padding: 5px 12px;
    display: flex;
    align-items: baseline;
    gap: 8px;
    font-size: 0.8rem;
    color: #ccc;
    font-weight: 500;
  }
  .pane-hint {
    color: #666;
    font-size: 0.7rem;
    font-weight: 400;
  }

  .ini-textarea {
    font-family: 'SF Mono', 'Fira Code', monospace;
    font-size: 0.78rem;
    line-height: 1.5;
    background: #1e1e2e;
    color: #bbb;
    border: none;
    resize: none;
    padding: 10px 12px;
    height: 280px;
    outline: none;
    width: 100%;
    box-sizing: border-box;
  }
  .ini-textarea[readonly] {
    color: #888;
    cursor: default;
  }

  .parse-error {
    margin: 0 10px;
    padding: 5px 8px;
    background: rgba(255,85,85,0.1);
    border: 1px solid rgba(255,85,85,0.3);
    border-radius: 4px;
    color: #ff5555;
    font-size: 0.75rem;
  }

  .load-btn, .download-btn {
    margin: 0 10px;
    padding: 7px 14px;
    border-radius: 5px;
    border: 1px solid;
    cursor: pointer;
    font-size: 0.8rem;
    text-align: center;
  }
  .load-btn {
    background: rgba(255,255,255,0.08);
    border-color: #444;
    color: #bbb;
  }
  .load-btn:hover { background: rgba(255,255,255,0.15); color: #ddd; }

  .download-btn {
    background: rgba(100,180,255,0.15);
    border-color: rgba(100,180,255,0.35);
    color: #7cb8ff;
  }
  .download-btn:hover { background: rgba(100,180,255,0.25); }

  /* ── Row 1, Right: toolbar ── */
  .toolbar {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  .section-toggle { display: flex; gap: 2px; }
  .tog-btn {
    padding: 6px 14px;
    border: 1px solid #444;
    border-radius: 4px;
    background: rgba(255,255,255,0.05);
    color: #888;
    cursor: pointer;
    font-family: 'SF Mono', monospace;
    font-size: 0.8rem;
  }
  .tog-btn.active {
    background: rgba(100,180,255,0.15);
    color: #7cb8ff;
    border-color: rgba(100,180,255,0.3);
  }
  .tog-btn:not(.active):hover { background: rgba(255,255,255,0.1); color: #bbb; }

  .macos-btn.active {
    background: rgba(255, 180, 80, 0.15);
    color: #ffb450;
    border-color: rgba(255, 180, 80, 0.35);
  }

  .defaults-btn {
    padding: 6px 14px;
    border: 1px solid #444;
    border-radius: 4px;
    background: rgba(255,255,255,0.05);
    color: #888;
    cursor: pointer;
    font-size: 0.8rem;
    margin-left: auto;
  }
  .defaults-btn.active {
    background: rgba(180,140,255,0.15);
    color: #c084fc;
    border-color: rgba(180,140,255,0.3);
  }
  .defaults-btn:not(.active):hover { background: rgba(255,255,255,0.1); color: #bbb; }

  /* ── Expand button in pane headers ── */
  .expand-btn {
    margin-left: auto;
    background: none;
    border: none;
    color: #666;
    cursor: pointer;
    padding: 2px 4px;
    border-radius: 3px;
    display: flex;
    align-items: center;
    line-height: 1;
    position: relative;
  }
  .expand-btn:hover { color: #aaa; background: rgba(255,255,255,0.08); }
  .expand-btn[data-tooltip]::after {
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
    z-index: 10;
    letter-spacing: normal;
    text-transform: none;
    font-weight: normal;
  }
  .expand-btn[data-tooltip]:hover::after { opacity: 1; }

  /* ── Maximised pane modal ── */
  .modal-backdrop {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.65);
    z-index: 1000;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 32px;
  }
  .modal-pane {
    background: #252535;
    border: 1px solid #444;
    border-radius: 8px;
    width: 100%;
    max-width: 960px;
    height: 80vh;
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }
  .modal-header {
    background: #2a2a3e;
    padding: 7px 14px;
    display: flex;
    align-items: baseline;
    gap: 10px;
    font-size: 0.85rem;
    color: #ccc;
    font-weight: 500;
    flex-shrink: 0;
    border-radius: 8px 8px 0 0;
  }
  .modal-hint { color: #666; font-size: 0.72rem; font-weight: 400; }
  .modal-close {
    margin-left: auto;
    background: none;
    border: none;
    color: #888;
    font-size: 1.3rem;
    cursor: pointer;
    line-height: 1;
    padding: 0 2px;
  }
  .modal-close:hover { color: #ff5555; }
  .modal-textarea {
    flex: 1;
    font-family: 'SF Mono', 'Fira Code', monospace;
    font-size: 0.82rem;
    line-height: 1.6;
    background: #1e1e2e;
    color: #bbb;
    border: none;
    resize: none;
    padding: 14px 18px;
    outline: none;
    width: 100%;
    box-sizing: border-box;
  }
  .modal-textarea[readonly] { color: #888; cursor: default; }
  .modal-actions {
    padding: 10px 14px;
    flex-shrink: 0;
  }
  .modal-error { margin: 0 14px 0; }

  /* ── IniHighlight sizing per context ── */
  .ini-pane :global(.ini-highlight) { height: 280px; }
  .modal-pane :global(.ini-highlight) { flex: 1; height: auto; min-height: 0; }

  /* ── Row 2, Right: editor components ── */
  .editor-column {
    display: flex;
    flex-direction: column;
    gap: 14px;
  }
</style>
