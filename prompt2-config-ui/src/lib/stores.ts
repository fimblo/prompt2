/**
 * Svelte stores for the config UI state.
 */
import { writable, derived } from 'svelte/store';
import { defaultIniFile, type IniFile } from './ini-parser';
import { tokenizePrompt } from './prompt-tokenizer';
import { renderPreview } from './preview-renderer';

export const iniFile = writable<IniFile>(defaultIniFile());
export const activeSection = writable<'prompt' | 'promptGit'>('prompt');

/** Index in the token array where the next widget insert should go */
export const cursorIndex = writable<number>(-1);

/** Token index currently being hovered in the prompt editor, or null */
export const hoveredTokenIndex = writable<number | null>(null);

/** Background colour of the preview terminal */
export const terminalBg = writable<string>('#1e1e2e');

/**
 * What is currently selected/open in the config panel.
 */
export type SelectedItem =
  | { kind: 'widget'; name: string }
  | { kind: 'text'; index: number }
  | { kind: 'attribute'; index: number }
  | { kind: 'defaults' }
  | null;

export const selectedItem = writable<SelectedItem>(null);

/** Current prompt tokens derived from the active section */
export const currentTokens = derived(
  [iniFile, activeSection],
  ([$iniFile, $activeSection]) => {
    const section = $activeSection === 'prompt' ? $iniFile.prompt : $iniFile.promptGit;
    if (!section) return [];
    return tokenizePrompt(section.prompt);
  }
);

/** Preview spans derived from current tokens, config, and active cwd_type */
export const previewSpans = derived(
  [currentTokens, iniFile, activeSection],
  ([$currentTokens, $iniFile, $activeSection]) => {
    const section = $activeSection === 'prompt' ? $iniFile.prompt : $iniFile.promptGit;
    return renderPreview($currentTokens, $iniFile, section?.cwd_type);
  }
);
