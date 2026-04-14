/**
 * Renders parsed prompt tokens into an array of styled spans
 * suitable for HTML preview.
 */
import type { PromptToken } from './prompt-tokenizer';
import type { WidgetConfig, IniFile } from './ini-parser';
import { resolveAttributeCombo, mergeStyles, type TermStyle } from './ansi-to-css';
import { tokenizePrompt } from './prompt-tokenizer';

export interface StyledSpan {
  text: string;
  style: TermStyle;
}

/** Base simulated state (CWD is overridden by cwd_type at render time) */
const BASE_STATE: Record<string, string> = {
  'sys.username': 'user',
  'sys.hostname': 'macbook',
  'sys.promptchar': '$',
  'sys.uid': '1000',
  'sys.gid': '1000',
  'repo.is_git_repo': '1',
  'repo.is_nascent_repo': '0',
  'repo.name': 'myapp',
  'repo.branch_name': 'feature/config-ui',
  'repo.rebase_active': '0',
  'repo.conflicts': '0',
  'repo.ahead': '2',
  'repo.behind': '0',
  'repo.staged': '3',
  'repo.modified': '1',
  'repo.untracked': '5',
  'aws.token_is_valid': '1',
  'aws.token_remaining_hours': '7',
  'aws.token_remaining_minutes': '42',
};

const CWD_BY_TYPE: Record<string, string> = {
  home:     '~/projects/myapp',
  basename: 'myapp',
  full:     '/home/user/projects/myapp',
  git:      'projects/myapp',
};

function isWidgetActive(widgetName: string, state: Record<string, string>): boolean {
  const val = state[widgetName.toLowerCase()];
  if (val === undefined) return false;
  if (val === '0' || val === '') return false;
  return true;
}

function getWidgetConfig(ini: IniFile, widgetName: string): WidgetConfig {
  const defaults = ini.widgetDefault;
  const perWidget = ini.widgets[widgetName] ?? ini.widgets[widgetName.toLowerCase()] ?? {};
  return {
    string_active:   perWidget.string_active   ?? defaults.string_active   ?? '%s',
    string_inactive: perWidget.string_inactive ?? defaults.string_inactive ?? '%s',
    colour_on:       perWidget.colour_on       ?? defaults.colour_on       ?? '',
    colour_off:      perWidget.colour_off      ?? defaults.colour_off      ?? '',
    max_width:       perWidget.max_width       ?? defaults.max_width       ?? 256,
  };
}

function renderWidget(
  widgetName: string,
  ini: IniFile,
  currentStyle: TermStyle,
  state: Record<string, string>,
): StyledSpan[] {
  const config = getWidgetConfig(ini, widgetName);
  const active = isWidgetActive(widgetName, state);
  const formatStr = active ? config.string_active! : config.string_inactive!;
  const colourAttr = active ? config.colour_on! : config.colour_off!;

  if (!formatStr) return [];

  let attrCombo = '';
  const attrMatch = colourAttr.match(/^%\{([^}]*)\}$/);
  if (attrMatch) attrCombo = attrMatch[1];

  const widgetStyle = attrCombo ? resolveAttributeCombo(attrCombo) : {};
  const appliedStyle = mergeStyles(currentStyle, widgetStyle);
  const rawValue = state[widgetName.toLowerCase()] ?? '';

  if (formatStr.includes('@{')) {
    const tokens = tokenizePrompt(formatStr.replace('%s', rawValue));
    const nested = renderTokens(tokens, ini, state);
    return nested.map(span => ({ text: span.text, style: mergeStyles(appliedStyle, span.style) }));
  }

  const text = formatStr.replace('%s', rawValue);
  const maxWidth = config.max_width ?? 256;
  const truncated = text.length > maxWidth ? text.slice(0, maxWidth - 1) + '~' : text;
  return [{ text: truncated, style: appliedStyle }];
}

function renderTokens(
  tokens: PromptToken[],
  ini: IniFile,
  state: Record<string, string>,
): StyledSpan[] {
  const spans: StyledSpan[] = [];
  let currentStyle: TermStyle = {};

  for (const token of tokens) {
    switch (token.type) {
      case 'text':
        spans.push({ text: token.value, style: { ...currentStyle } });
        break;

      case 'newline':
        spans.push({ text: '\n', style: {} });
        break;

      case 'attribute':
        if (token.value.trim() === '' || token.value.trim().toLowerCase() === 'reset') {
          currentStyle = {};
        } else {
          currentStyle = mergeStyles(currentStyle, resolveAttributeCombo(token.value));
        }
        break;

      case 'widget':
        if (token.name.toUpperCase() === 'SPC') {
          spans.push({ text: '    ', style: {} });
          break;
        }
        spans.push(...renderWidget(token.name, ini, currentStyle, state));
        break;
    }
  }

  return spans;
}

/**
 * Main entry point: render a prompt preview.
 * @param cwdType — the cwd_type setting from the active prompt section
 */
export function renderPreview(
  tokens: PromptToken[],
  ini: IniFile,
  cwdType?: string,
): StyledSpan[] {
  const state: Record<string, string> = {
    ...BASE_STATE,
    cwd: CWD_BY_TYPE[cwdType ?? 'home'] ?? CWD_BY_TYPE.home,
  };
  return renderTokens(tokens, ini, state);
}
