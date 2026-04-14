/**
 * Parses attributes.c and emits a TypeScript data file with all
 * terminal attribute entries (names, ANSI codes, RGB values, categories).
 *
 * Run with: npx tsx scripts/extract-attributes.ts
 */
import { readFileSync, writeFileSync, mkdirSync } from 'fs';
import { resolve, dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface RawEntry {
  name: string;
  code: string;
}

const srcPath = resolve(__dirname, '../../src/attributes.c');
const outPath = resolve(__dirname, '../src/data/attributes.ts');

const src = readFileSync(srcPath, 'utf-8');

// Match lines like:  { "fg red",  "31" },
const re = /\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}/g;
const entries: RawEntry[] = [];
let m: RegExpExecArray | null;
while ((m = re.exec(src)) !== null) {
  entries.push({ name: m[1], code: m[2] });
}

// Deduplicate (the C file has slot aliases on the same line)
const seen = new Set<string>();
const unique = entries.filter(e => {
  if (seen.has(e.name)) return false;
  seen.add(e.name);
  return true;
});

function categorise(name: string, code: string): string {
  if (name.startsWith('reset')) return 'reset';
  if (['bold','dim','italic','underline','blink','reverse','hidden','strikethrough'].includes(name)) return 'style';
  if (code.startsWith('38;2;')) return 'named_rgb_fg';
  if (code.startsWith('48;2;')) return 'named_rgb_bg';
  if (name.startsWith('fg bright')) return 'reserved_bright_fg';
  if (name.startsWith('bg bright')) return 'reserved_bright_bg';
  if (name.startsWith('fg')) return 'reserved_fg';
  if (name.startsWith('bg')) return 'reserved_bg';
  return 'style';
}

function parseRgb(code: string): [number, number, number] | null {
  // 38;2;R;G;B or 48;2;R;G;B
  const m = code.match(/^(?:38|48);2;(\d+);(\d+);(\d+)$/);
  if (!m) return null;
  return [parseInt(m[1]), parseInt(m[2]), parseInt(m[3])];
}

const lines = unique.map(e => {
  const cat = categorise(e.name, e.code);
  const rgb = parseRgb(e.code);
  const rgbStr = rgb ? `, rgb: [${rgb.join(', ')}] as [number, number, number]` : '';
  return `  { name: ${JSON.stringify(e.name)}, ansiCode: ${JSON.stringify(e.code)}, category: ${JSON.stringify(cat)}${rgbStr} },`;
});

const output = `// Auto-generated from attributes.c — do not edit manually.
// Run: npx tsx scripts/extract-attributes.ts

export interface AttributeEntry {
  name: string;
  ansiCode: string;
  category: 'reset' | 'style' | 'reserved_fg' | 'reserved_bg' | 'reserved_bright_fg' | 'reserved_bright_bg' | 'named_rgb_fg' | 'named_rgb_bg';
  rgb?: [number, number, number];
}

export const attributes: AttributeEntry[] = [
${lines.join('\n')}
];
`;

mkdirSync(dirname(outPath), { recursive: true });
writeFileSync(outPath, output, 'utf-8');
console.log(`Wrote ${unique.length} attributes to ${outPath}`);
