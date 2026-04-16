<script lang="ts">
  import { derived } from 'svelte/store';
  import Preview from './components/Preview.svelte';
  import PromptEditor from './components/PromptEditor.svelte';
  import WidgetPalette from './components/WidgetPalette.svelte';
  import WidgetConfigPanel from './components/WidgetConfigPanel.svelte';
  import { iniFile, activeSection, selectedItem, terminalBg } from './lib/stores';
  import { parseIni, serializeIni } from './lib/ini-parser';

  // ── Default config loaded on startup ──────────────────────────────────────
  const DEFAULT_CONFIG = `[PROMPT]
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
    <button class="reset-btn" onclick={resetToDefault}>Reset to default</button>
  </header>

  <div class="three-panel">

    <!-- ══════════════ LEFT: INI column ══════════════ -->
    <div class="ini-column">

      <div class="ini-pane">
        <div class="pane-header">
          <span>Original INI</span>
          <span class="pane-hint">paste your config, then Load</span>
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
        </div>
        <textarea
          class="ini-textarea"
          value={$updatedIni}
          readonly
          spellcheck="false"
        ></textarea>
        <button class="download-btn" onclick={downloadConfig}>
          Download dot.prompt2_config.ini
        </button>
      </div>

    </div>

    <!-- ══════════════ RIGHT: Editor column ══════════════ -->
    <div class="editor-column">

      <!-- Toolbar: section + widget defaults -->
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
          class="defaults-btn"
          class:active={$selectedItem?.kind === 'defaults'}
          onclick={() => selectedItem.set(
            $selectedItem?.kind === 'defaults' ? null : { kind: 'defaults' }
          )}
        >Widget Defaults</button>
      </div>

      <Preview />
      <PromptEditor />
      <WidgetConfigPanel />
      <WidgetPalette />

    </div>

  </div>
</div>

<style>
  .app {
    max-width: 1280px;
    margin: 0 auto;
    padding: 20px 24px;
  }

  header {
    margin-bottom: 18px;
    display: flex;
    align-items: center;
    justify-content: space-between;
  }
  h1 { color: #e0e0e0; font-size: 1.4rem; margin: 0; }
  .reset-btn {
    padding: 5px 12px;
    border: 1px solid #555;
    border-radius: 5px;
    background: rgba(255, 255, 255, 0.05);
    color: #777;
    cursor: pointer;
    font-size: 0.78rem;
  }
  .reset-btn:hover {
    border-color: #ff5555;
    color: #ff5555;
    background: rgba(255, 85, 85, 0.08);
  }

  /* ── Three-panel layout ── */
  .three-panel {
    display: grid;
    grid-template-columns: 340px 1fr;
    gap: 20px;
    align-items: start;
  }

  /* ── Left column ── */
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

  /* ── Right column ── */
  .editor-column {
    display: flex;
    flex-direction: column;
    gap: 14px;
  }

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
</style>
