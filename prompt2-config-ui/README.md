# prompt2 Config Editor

<!-- Insert screenshot here -->

A browser-based visual editor for your `~/.prompt2_config.ini`. It's not
strictly necessary — editing the INI file directly works fine, especially
once you're familiar with the widget and attribute names. But if you're
starting out, or if you just want to tweak colours without memorising the
attribute catalogue, this makes it a lot less tedious.

## What it does

- Shows a live preview of what your prompt will look like as you edit
- Lets you drag-and-drop (well, click-to-insert) widgets into the prompt
- Gives you a searchable colour picker with all 658 named RGB colours
- Keeps your original config on the left while showing the updated version
  next to it, so you can see exactly what changed
- Downloads the result as `dot.prompt2_config.ini`, ready to copy to `~`

## Starting it

You'll need Node.js installed. Then, from the repo root:

```bash
cd prompt2-config-ui
npm install
npm run dev
```

Open your browser at the URL it prints (usually `http://localhost:5173`).

## Using it

1. Paste your existing `~/.prompt2_config.ini` into the **Original INI**
   panel on the left, then click **Load config**. If you're starting from
   scratch, the editor loads a reasonable default config for you.

2. Use the **PROMPT** and **PROMPT.GIT** buttons to switch between the two
   prompt sections. The preview updates to show whichever one you're editing.

3. In the **Prompt Tokens** row, click a gap `|` to set your insert position,
   then click a widget from the palette at the bottom to insert it there.

4. Click any token to configure it:
   - **Widget tokens** (`@{Repo.name}` etc.) — set `string_active`,
     `string_inactive`, `max_width`, and colours for the active/inactive states
   - **Text tokens** — edit the text and optionally wrap it in a colour
   - **Attribute tokens** (`%{fg cyan}` etc.) — replace or clear the attribute

5. Use **Widget Defaults** (top right of the editor) to set the fallback
   colour and format that applies to all widgets which don't override it.

6. When you're happy, click **Download dot.prompt2_config.ini** in the bottom
   left panel, then:

```bash
cp ~/Downloads/dot.prompt2_config.ini ~/.prompt2_config.ini
```

## Keeping the data files up to date

The colour catalogue (`src/data/attributes.ts`) is generated from
`src/attributes.c`. If the C source ever gains new colours, regenerate it:

```bash
npm run extract-data
```
