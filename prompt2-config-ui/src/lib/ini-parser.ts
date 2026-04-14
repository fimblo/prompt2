/**
 * Client-side INI parser and serializer for prompt2 config files.
 *
 * Handles: line continuation (\), comments (#), quoted values,
 * and the specific section structure used by prompt2.
 */

export interface WidgetConfig {
  string_active?: string;
  string_inactive?: string;
  colour_on?: string;
  colour_off?: string;
  max_width?: number;
}

export interface PromptSection {
  prompt: string;
  cwd_type?: string;
}

export interface IniFile {
  prompt: PromptSection;
  promptGit?: PromptSection;
  widgetDefault: WidgetConfig;
  widgets: Record<string, WidgetConfig>;
}

export function defaultIniFile(): IniFile {
  return {
    prompt: { prompt: '@{SYS.promptchar} ' },
    widgetDefault: {
      string_active: '%s',
      string_inactive: '%s',
      colour_on: '',
      colour_off: '',
      max_width: 256,
    },
    widgets: {},
  };
}

/**
 * Parse an INI config string into an IniFile structure.
 */
export function parseIni(text: string): IniFile {
  const result = defaultIniFile();
  const lines = text.split('\n');

  let currentSection = '';
  let currentKey = '';
  let currentValue = '';
  let continuation = false;

  function commitKeyValue() {
    if (!currentKey || !currentSection) return;

    // Strip quotes
    let val = currentValue.trim();
    if ((val.startsWith('"') && val.endsWith('"')) ||
        (val.startsWith("'") && val.endsWith("'"))) {
      val = val.slice(1, -1);
    }

    const sectionLower = currentSection.toLowerCase();

    if (sectionLower === 'prompt' || sectionLower === 'prompt.git') {
      const section: PromptSection = sectionLower === 'prompt'
        ? result.prompt
        : (result.promptGit ??= { prompt: '' });

      if (currentKey === 'prompt') {
        section.prompt = val;
      } else if (currentKey === 'cwd_type') {
        section.cwd_type = val;
      }
    } else if (sectionLower === 'widget_default') {
      assignWidgetKey(result.widgetDefault, currentKey, val);
    } else {
      // Per-widget section — use original case from the section header
      if (!result.widgets[currentSection]) {
        result.widgets[currentSection] = {};
      }
      assignWidgetKey(result.widgets[currentSection], currentKey, val);
    }

    currentKey = '';
    currentValue = '';
  }

  function assignWidgetKey(wc: WidgetConfig, key: string, val: string) {
    switch (key) {
      case 'string_active': wc.string_active = val; break;
      case 'string_inactive': wc.string_inactive = val; break;
      case 'colour_on': wc.colour_on = val; break;
      case 'colour_off': wc.colour_off = val; break;
      case 'max_width': wc.max_width = parseInt(val, 10) || 256; break;
    }
  }

  for (const rawLine of lines) {
    let line = rawLine;

    // Handle line continuation
    if (continuation) {
      // Strip inline comment (only outside quotes — simplified: strip after last #)
      const commentIdx = line.indexOf('#');
      if (commentIdx >= 0) {
        // Only strip if # is not inside a %{...} or @{...}
        const beforeHash = line.slice(0, commentIdx);
        const openBraces = (beforeHash.match(/[%@]\{/g) || []).length;
        const closeBraces = (beforeHash.match(/\}/g) || []).length;
        if (openBraces <= closeBraces) {
          line = line.slice(0, commentIdx);
        }
      }

      if (line.trimEnd().endsWith('\\')) {
        currentValue += line.trimEnd().slice(0, -1);
        continue;
      } else {
        currentValue += line;
        continuation = false;
        commitKeyValue();
        continue;
      }
    }

    // Strip full-line comments and empty lines
    const trimmed = line.trim();
    if (trimmed === '' || trimmed.startsWith('#')) continue;

    // Section header
    const sectionMatch = trimmed.match(/^\[([^\]]+)\]$/);
    if (sectionMatch) {
      commitKeyValue();
      currentSection = sectionMatch[1];
      continue;
    }

    // Key=value
    const eqIdx = line.indexOf('=');
    if (eqIdx >= 0) {
      commitKeyValue();
      currentKey = line.slice(0, eqIdx).trim().toLowerCase();
      let val = line.slice(eqIdx + 1);

      // Strip inline comment (simplified)
      const commentIdx = val.indexOf('#');
      if (commentIdx >= 0) {
        const beforeHash = val.slice(0, commentIdx);
        const openBraces = (beforeHash.match(/[%@]\{/g) || []).length;
        const closeBraces = (beforeHash.match(/\}/g) || []).length;
        if (openBraces <= closeBraces) {
          val = val.slice(0, commentIdx);
        }
      }

      if (val.trimEnd().endsWith('\\')) {
        currentValue = val.trimEnd().slice(0, -1);
        continuation = true;
      } else {
        currentValue = val;
        commitKeyValue();
      }
    }
  }

  // Final pending value
  commitKeyValue();

  return result;
}

/**
 * Serialize an IniFile back to INI text.
 */
export function serializeIni(ini: IniFile): string {
  const lines: string[] = [];

  // [PROMPT]
  lines.push('[PROMPT]');
  lines.push(`prompt="${ini.prompt.prompt}"`);
  if (ini.prompt.cwd_type) {
    lines.push(`cwd_type="${ini.prompt.cwd_type}"`);
  }
  lines.push('');

  // [PROMPT.GIT]
  if (ini.promptGit) {
    lines.push('[PROMPT.GIT]');
    lines.push(`prompt="${ini.promptGit.prompt}"`);
    if (ini.promptGit.cwd_type) {
      lines.push(`cwd_type="${ini.promptGit.cwd_type}"`);
    }
    lines.push('');
  }

  // [WIDGET_DEFAULT]
  const wd = ini.widgetDefault;
  if (wd.string_active || wd.string_inactive || wd.colour_on || wd.colour_off || wd.max_width) {
    lines.push('[WIDGET_DEFAULT]');
    if (wd.string_active !== undefined) lines.push(`string_active="${wd.string_active}"`);
    if (wd.string_inactive !== undefined) lines.push(`string_inactive="${wd.string_inactive}"`);
    if (wd.colour_on !== undefined) lines.push(`colour_on="${wd.colour_on}"`);
    if (wd.colour_off !== undefined) lines.push(`colour_off="${wd.colour_off}"`);
    if (wd.max_width !== undefined) lines.push(`max_width=${wd.max_width}`);
    lines.push('');
  }

  // Per-widget sections
  for (const [name, wc] of Object.entries(ini.widgets)) {
    const entries: string[] = [];
    if (wc.string_active !== undefined) entries.push(`string_active="${wc.string_active}"`);
    if (wc.string_inactive !== undefined) entries.push(`string_inactive="${wc.string_inactive}"`);
    if (wc.colour_on !== undefined) entries.push(`colour_on="${wc.colour_on}"`);
    if (wc.colour_off !== undefined) entries.push(`colour_off="${wc.colour_off}"`);
    if (wc.max_width !== undefined) entries.push(`max_width=${wc.max_width}`);

    if (entries.length > 0) {
      lines.push(`[${name}]`);
      lines.push(...entries);
      lines.push('');
    }
  }

  return lines.join('\n') + '\n';
}
