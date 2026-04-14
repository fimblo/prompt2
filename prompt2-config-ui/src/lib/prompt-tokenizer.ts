/**
 * Tokenizes a prompt string into a sequence of typed tokens.
 *
 * Widget tokens:    @{WidgetName}
 * Attribute tokens: %{fg red, bold}  or  %{} (reset)
 * Newlines:         \n  or  \\n (macOS iniparser quirk)
 * Everything else:  plain text
 */

export type PromptToken =
  | { type: 'text'; value: string }
  | { type: 'widget'; name: string }
  | { type: 'attribute'; value: string }
  | { type: 'newline' };

export function tokenizePrompt(prompt: string): PromptToken[] {
  const tokens: PromptToken[] = [];
  let i = 0;
  let textBuf = '';

  function flushText() {
    if (textBuf) {
      tokens.push({ type: 'text', value: textBuf });
      textBuf = '';
    }
  }

  while (i < prompt.length) {
    // Widget token: @{...}
    if (prompt[i] === '@' && prompt[i + 1] === '{') {
      flushText();
      const end = prompt.indexOf('}', i + 2);
      if (end === -1) {
        textBuf += prompt.slice(i);
        break;
      }
      tokens.push({ type: 'widget', name: prompt.slice(i + 2, end) });
      i = end + 1;
      continue;
    }

    // Attribute token: %{...}
    if (prompt[i] === '%' && prompt[i + 1] === '{') {
      flushText();
      const end = prompt.indexOf('}', i + 2);
      if (end === -1) {
        textBuf += prompt.slice(i);
        break;
      }
      tokens.push({ type: 'attribute', value: prompt.slice(i + 2, end) });
      i = end + 1;
      continue;
    }

    // Newline: \\n (macOS double-backslash) or \n
    if (prompt[i] === '\\') {
      if (prompt[i + 1] === '\\' && prompt[i + 2] === 'n') {
        flushText();
        tokens.push({ type: 'newline' });
        i += 3;
        continue;
      }
      if (prompt[i + 1] === 'n') {
        flushText();
        tokens.push({ type: 'newline' });
        i += 2;
        continue;
      }
    }

    textBuf += prompt[i];
    i++;
  }

  flushText();
  return tokens;
}

export function serializeTokens(tokens: PromptToken[]): string {
  return tokens.map(t => {
    switch (t.type) {
      case 'text': return t.value;
      case 'widget': return `@{${t.name}}`;
      case 'attribute': return `%{${t.value}}`;
      case 'newline': return '\\n';
    }
  }).join('');
}
