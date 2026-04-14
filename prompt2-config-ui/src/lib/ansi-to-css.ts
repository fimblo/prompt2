/**
 * Converts ANSI escape codes and prompt2 attribute names to CSS styles
 * for the preview renderer.
 */
import { attributes, type AttributeEntry } from '../data/attributes';

export interface TermStyle {
  color?: string;
  backgroundColor?: string;
  fontWeight?: string;
  opacity?: number;
  fontStyle?: string;
  textDecoration?: string;
}

// Standard terminal palette (typical dark terminal theme)
const PALETTE: Record<number, string> = {
  30: '#1a1a1a', 31: '#cc3333', 32: '#33cc33', 33: '#cccc33',
  34: '#3333cc', 35: '#cc33cc', 36: '#33cccc', 37: '#cccccc',
  40: '#1a1a1a', 41: '#cc3333', 42: '#33cc33', 43: '#cccc33',
  44: '#3333cc', 45: '#cc33cc', 46: '#33cccc', 47: '#cccccc',
  // Bright variants
  90: '#666666', 91: '#ff5555', 92: '#55ff55', 93: '#ffff55',
  94: '#5555ff', 95: '#ff55ff', 96: '#55ffff', 97: '#ffffff',
  100: '#666666', 101: '#ff5555', 102: '#55ff55', 103: '#ffff55',
  104: '#5555ff', 105: '#ff55ff', 106: '#55ffff', 107: '#ffffff',
};

/**
 * Parse a single ANSI code number and apply its effect to a TermStyle.
 */
function applyCode(code: number, style: TermStyle): void {
  // Reset
  if (code === 0) {
    Object.keys(style).forEach(k => delete (style as Record<string, unknown>)[k]);
    return;
  }

  // Styles
  if (code === 1) { style.fontWeight = 'bold'; return; }
  if (code === 2) { style.opacity = 0.5; return; }
  if (code === 3) { style.fontStyle = 'italic'; return; }
  if (code === 4) { style.textDecoration = 'underline'; return; }
  if (code === 9) { style.textDecoration = 'line-through'; return; }

  // Reset styles
  if (code === 22) { delete style.fontWeight; delete style.opacity; return; }
  if (code === 23) { delete style.fontStyle; return; }
  if (code === 24 || code === 29) { delete style.textDecoration; return; }
  if (code === 39) { delete style.color; return; }
  if (code === 49) { delete style.backgroundColor; return; }

  // Standard + bright foreground/background
  if (PALETTE[code]) {
    if ((code >= 30 && code <= 37) || (code >= 90 && code <= 97)) {
      style.color = PALETTE[code];
    } else {
      style.backgroundColor = PALETTE[code];
    }
  }
}

/**
 * Parse a semicolon-separated ANSI code string (e.g. "38;2;255;0;0")
 * and return the resulting CSS style.
 */
export function ansiCodeToCss(codeStr: string): TermStyle {
  const style: TermStyle = {};
  const parts = codeStr.split(';').map(Number);

  let i = 0;
  while (i < parts.length) {
    // 24-bit color: 38;2;R;G;B (fg) or 48;2;R;G;B (bg)
    if ((parts[i] === 38 || parts[i] === 48) && parts[i + 1] === 2 && i + 4 < parts.length) {
      const rgb = `rgb(${parts[i + 2]}, ${parts[i + 3]}, ${parts[i + 4]})`;
      if (parts[i] === 38) {
        style.color = rgb;
      } else {
        style.backgroundColor = rgb;
      }
      i += 5;
      continue;
    }
    applyCode(parts[i], style);
    i++;
  }

  return style;
}

/**
 * Build a lookup map from attribute name → ANSI code string.
 */
const attrLookup = new Map<string, string>();
for (const entry of attributes) {
  attrLookup.set(entry.name.toLowerCase(), entry.ansiCode);
}

/**
 * Resolve a comma-separated attribute combo (e.g. "fg red, bold")
 * into a merged CSS TermStyle.
 */
export function resolveAttributeCombo(combo: string): TermStyle {
  if (!combo.trim()) {
    // Empty attribute = reset
    return {};
  }

  const merged: TermStyle = {};
  const parts = combo.split(',').map(s => s.trim().toLowerCase());

  for (const part of parts) {
    // Check for fg-rgb-R;G;B or bg-rgb-R;G;B (custom RGB)
    const rgbMatch = part.match(/^(fg|bg)-rgb-(.+)$/);
    if (rgbMatch) {
      const rgb = rgbMatch[2]; // e.g. "255;0;0"
      const prefix = rgbMatch[1] === 'fg' ? '38' : '48';
      const style = ansiCodeToCss(`${prefix};2;${rgb}`);
      Object.assign(merged, style);
      continue;
    }

    // Lookup in attribute database
    const code = attrLookup.get(part);
    if (code) {
      const style = ansiCodeToCss(code);
      Object.assign(merged, style);
    }
  }

  return merged;
}

/**
 * Merge one TermStyle into another (non-destructively).
 * Reset (empty source) clears all properties.
 */
export function mergeStyles(base: TermStyle, overlay: TermStyle): TermStyle {
  return { ...base, ...overlay };
}

/**
 * Convert a TermStyle to a CSS style string for use in inline styles.
 */
export function termStyleToCssString(style: TermStyle): string {
  const parts: string[] = [];
  if (style.color) parts.push(`color: ${style.color}`);
  if (style.backgroundColor) parts.push(`background-color: ${style.backgroundColor}`);
  if (style.fontWeight) parts.push(`font-weight: ${style.fontWeight}`);
  if (style.opacity !== undefined) parts.push(`opacity: ${style.opacity}`);
  if (style.fontStyle) parts.push(`font-style: ${style.fontStyle}`);
  if (style.textDecoration) parts.push(`text-decoration: ${style.textDecoration}`);
  return parts.join('; ');
}
