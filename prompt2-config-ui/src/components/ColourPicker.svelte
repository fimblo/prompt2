<script lang="ts">
  import { attributes } from '../data/attributes';
  import { resolveAttributeCombo, termStyleToCssString } from '../lib/ansi-to-css';

  let {
    label,
    value,
    onpick,
    onclear,
  }: {
    label: string;
    value: string;
    onpick: (attrName: string) => void;
    onclear: () => void;
  } = $props();

  // Each ColourPicker instance has its own isolated search state — fixes search reliability
  let searchQuery = $state('');

  const namedFgColors = attributes.filter(a => a.category === 'named_rgb_fg');
  const reservedFg = attributes.filter(a => a.category === 'reserved_fg' && !a.name.includes(' c'));
  const reservedBrightFg = attributes.filter(a => a.category === 'reserved_bright_fg' && !a.name.includes(' c'));
  const styleAttrs = attributes.filter(a => a.category === 'style');

  let filteredColors = $derived(
    searchQuery.trim()
      ? namedFgColors.filter(a => a.name.toLowerCase().includes(searchQuery.toLowerCase()))
      : namedFgColors.slice(0, 48)
  );

  function swatchBg(attrName: string): string {
    const css = termStyleToCssString(resolveAttributeCombo(attrName));
    const m = css.match(/color:\s*([^;]+)/);
    return m ? m[1].trim() : '#444';
  }

  // Extract the attr combo from the %{...} wrapper for display
  let currentAttr = $derived(() => {
    const m = value.match(/^%\{([^}]*)\}$/);
    return m ? m[1] : '';
  });
</script>

<div class="picker">
  <div class="picker-label">{label}</div>

  <div class="current-value">
    <code class="current-code">{value || '(default)'}</code>
    {#if currentAttr()}
      <span class="preview-sample" style={termStyleToCssString(resolveAttributeCombo(currentAttr()))}>sample</span>
    {/if}
    {#if value}
      <button class="clear-btn" onclick={onclear}>clear</button>
    {/if}
  </div>

  <div class="sub-label">Styles</div>
  <div class="style-grid">
    {#each styleAttrs as attr}
      <button class="style-btn" onclick={() => onpick(attr.name)}>{attr.name}</button>
    {/each}
  </div>

  <div class="sub-label">Terminal Colors</div>
  <div class="swatch-row">
    {#each reservedFg as attr}
      <button class="swatch" style="background:{swatchBg(attr.name)}" title={attr.name} onclick={() => onpick(attr.name)}></button>
    {/each}
  </div>
  <div class="swatch-row">
    {#each reservedBrightFg as attr}
      <button class="swatch" style="background:{swatchBg(attr.name)}" title={attr.name} onclick={() => onpick(attr.name)}></button>
    {/each}
  </div>

  <div class="sub-label">Named RGB ({namedFgColors.length} total)</div>
  <input
    class="search"
    type="text"
    placeholder="Search colors…"
    bind:value={searchQuery}
  />
  <div class="named-list">
    {#each filteredColors as attr}
      <button class="named-btn" onclick={() => onpick(attr.name)}>
        <span class="named-swatch" style="background:rgb({attr.rgb?.join(',') ?? '128,128,128'})"></span>
        <span class="named-name">{attr.name.replace('fg-', '')}</span>
      </button>
    {/each}
    {#if !searchQuery.trim() && namedFgColors.length > 48}
      <span class="more-hint">type to search all {namedFgColors.length}…</span>
    {/if}
  </div>

  <div class="sub-label">Custom combo</div>
  <input
    class="custom-input"
    type="text"
    placeholder="e.g. fg cyan, bold  — press Enter"
    onkeydown={(e) => {
      if (e.key === 'Enter') {
        const v = (e.target as HTMLInputElement).value.trim();
        if (v) { onpick(v); (e.target as HTMLInputElement).value = ''; }
      }
    }}
  />
</div>

<style>
  .picker {
    display: flex;
    flex-direction: column;
    gap: 7px;
    min-width: 0;
  }
  .picker-label {
    font-size: 0.72rem;
    text-transform: uppercase;
    letter-spacing: 0.07em;
    color: #7cb8ff;
    font-weight: 600;
    padding-bottom: 2px;
    border-bottom: 1px solid rgba(100,180,255,0.2);
  }
  .current-value {
    display: flex;
    align-items: center;
    gap: 6px;
    flex-wrap: wrap;
    min-height: 24px;
  }
  .current-code {
    font-family: 'SF Mono', monospace;
    font-size: 0.75rem;
    color: #ccc;
    word-break: break-all;
  }
  .preview-sample {
    font-family: 'SF Mono', monospace;
    font-size: 0.78rem;
    padding: 1px 6px;
    border-radius: 3px;
    background: #1e1e2e;
  }
  .clear-btn {
    background: none;
    border: 1px solid #555;
    border-radius: 3px;
    color: #777;
    font-size: 0.65rem;
    padding: 1px 5px;
    cursor: pointer;
    flex-shrink: 0;
  }
  .clear-btn:hover { color: #ff5555; border-color: #ff5555; }

  .sub-label {
    font-size: 0.65rem;
    text-transform: uppercase;
    letter-spacing: 0.07em;
    color: #666;
    margin-top: 2px;
  }
  .style-grid { display: flex; flex-wrap: wrap; gap: 3px; }
  .style-btn {
    padding: 3px 7px;
    background: rgba(255,255,255,0.07);
    border: 1px solid #444;
    border-radius: 3px;
    color: #aaa;
    font-size: 0.72rem;
    cursor: pointer;
    white-space: nowrap;
  }
  .style-btn:hover { background: rgba(255,255,255,0.15); color: #ddd; }

  .swatch-row { display: flex; gap: 3px; flex-wrap: wrap; }
  .swatch {
    width: 22px;
    height: 22px;
    border-radius: 3px;
    border: 1px solid #555;
    cursor: pointer;
    flex-shrink: 0;
    transition: transform 0.1s;
  }
  .swatch:hover { transform: scale(1.25); border-color: #aaa; }

  .search {
    width: 100%;
    background: #1e1e2e;
    border: 1px solid #444;
    border-radius: 4px;
    color: #ccc;
    padding: 5px 8px;
    font-size: 0.82rem;
    box-sizing: border-box;
  }
  .search:focus { outline: none; border-color: #7cb8ff; }

  .named-list {
    display: flex;
    flex-wrap: wrap;
    gap: 3px;
    max-height: 140px;
    overflow-y: auto;
  }
  .named-btn {
    display: flex;
    align-items: center;
    gap: 3px;
    padding: 2px 5px;
    background: rgba(255,255,255,0.04);
    border: 1px solid #2e2e2e;
    border-radius: 3px;
    cursor: pointer;
    color: inherit;
  }
  .named-btn:hover { background: rgba(255,255,255,0.11); }
  .named-swatch {
    width: 12px;
    height: 12px;
    border-radius: 2px;
    border: 1px solid #555;
    flex-shrink: 0;
  }
  .named-name { font-size: 0.68rem; color: #aaa; }
  .more-hint { font-size: 0.7rem; color: #555; padding: 3px; }

  .custom-input {
    width: 100%;
    background: #1e1e2e;
    border: 1px solid #444;
    border-radius: 4px;
    color: #ccc;
    padding: 5px 8px;
    font-family: 'SF Mono', monospace;
    font-size: 0.78rem;
    box-sizing: border-box;
  }
  .custom-input:focus { outline: none; border-color: #7cb8ff; }
</style>
