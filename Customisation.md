# Customisation

Customising prompt2 involves modifying the INI configuration file to
adjust the appearance and behavior of the prompt components, known as
Widgets. The configuration file allows you to set various parameters
for each widget, such as text formatting, active and inactive states,
and style Attributes.

## Defining your prompts

Currently, there are two prompts you can configure:

- **the default prompt**: Set this for a prompt for any situation. To
  configure this, add a `prompt` entry under the `PROMPT` section.
- **the git prompt**: overrides the default when standing somewhere
  inside of a git repo. To configure this, add a `prompt` entry under
  the `PROMPT.GIT` section. This prompt config is optional, but very
  useful if you use git.

For example, a minimal prompt setup could look like this:

```ini
  [PROMPT]
  prompt="my prompt $ "
```

This snippet would, if applied to your shell, result in the prompt
`my prompt $` followed by a space.

But this is no fun, so let's add a dynamic element to this prompt.
These dynamic instructions are called Widgets. Each Widget is wrapped
like so: `@{widgetname}` and are at runtime replaced with whatever
they represent at the time.

```ini
  # use '#' if you're root, else '$'
  [PROMPT]
  prompt="@{SYS.promptchar} "
```

Now let's complicate this a bit more by adding a widget which shows
you where you are in the filesystem:

```ini
  [PROMPT]
  prompt="@{CWD} @{SYS.promptchar} "
```

CWD is shorthand for 'Current Working Directory' - that is, the name
of the directory you're standing in. This can traditionally be shown
in one of two ways: the full path from the root of the file system or
the path starting from your HOME directory.

prompt2 supports four styles of CWD:
  - `home`: show the path from your home directory. This is used if
    `cwd_type` is unset.
  - `basename`: show only the name of the directory you're standing
    in.
  - `full`: show the full path from /
  - `git`: show the path from the root of the git repo. 

You can modify the CWD style for your prompts:

```ini
  [PROMPT]
  prompt="@{CWD} @{SYS.promptchar} "
  cwd_type="full"
```

In the example above, you will override the default (path from HOME)
with a full path from root.


Ok, now it's time to override the default prompt when standing in a
git repo. To do this, you add a new INI section. Let's make this a bit
more playful by getting it to show:

- The repository name
- The branch name
- How many commits behind or ahead of upstream the repo is right now
- How many staged, modified and untracked files are in the repo

You can use `\` to separate the config over many lines, as well as
the `\n` to insert a newline into the resulting prompt.

> [!NOTE]
> **Macos users**: The iniparser library on macos requires an extra
> backslash, so to insert a newline you'll need to write `\\n`.



```ini
  [PROMPT]
  prompt="@{CWD} @{SYS.promptchar} "

  [PROMPT.GIT]
  prompt="\
  <@{Repo.name}><@{Repo.branch_name}>\
  [@{Repo.behind},@{Repo.ahead}|@{Repo.staged},@{Repo.modified},@{Repo.untracked}] \
  @{CWD}\n\
  @{SYS.promptchar} "
  cwd_type="git"
```

Before we dive into widgets and how to configure them, let's take a
short detour and talk about Attributes...

## Style Attributes

Typically referred to as Attributes, these are style instructions
which you can add to the prompt or Widget configuration. An attribute
starts with a `%` followed by curly braces, and contains an
instruction. It could look like this `%{bold}`. All attributes are
case-insensitive.

### Text attributes

To change the style of the font, you can use these attributes. These
are: `bold`, `dim`, `italic`, `underline`, `blink`, `reverse`,
`hidden`, `strikethrough`.


Here we change the default prompt style by making everything
strike-through, as the cool kids do:

```ini
  [PROMPT]
  prompt="%{Strikethrough}My prompt%{Reset} $ "
```

### Reset attributes

Used to reset part or all of the set attributes. These are: `reset`,
`reset bold/dim`, `reset italic`, `reset underline`, `reset blink`,
`reset reverse`, `reset hidden`, `reset fg`, `reset bg`.

The empty attribute `%{}` is shorthand for `%{reset}`.

### Reserved colour slot attributes

Modern terminals have eight reserved colours: black, red, green,
yellow, blue, magenta, cyan, and white. The actual colours may vary
depending on your terminal settings but retain their slot names.

You can also refer to these slots by the slot number, zero-indexed.

To differentiate between foreground and background colours, prefix
with `fg` or `bg`.

```
    +-------------------------+-------------------------+
    |         BY NAME         |         BY SLOT         |
    +------------+------------+------------+------------+
    | FOREGROUND | BACKGROUND | FOREGROUND | BACKGROUND |
    +------------+------------+------------+------------+
    | fg black   | bg black   | fg c0      | bg c0      |
    | fg red     | bg red     | fg c1      | bg c1      |
    | fg green   | bg green   | fg c2      | bg c2      |
    | fg yellow  | bg yellow  | fg c3      | bg c3      |
    | fg blue    | bg blue    | fg c4      | bg c4      |
    | fg magenta | bg magenta | fg c5      | bg c5      |
    | fg cyan    | bg cyan    | fg c6      | bg c6      |
    | fg white   | bg white   | fg c7      | bg c7      |
    +------------+------------+------------+------------+
```


```ini
  [PROMPT]
  # blue and yellow foreground text
  prompt="%{fg blue}My %{fg c3}prompt%{reset}$ "
```

Note: each of the above have a corresponding `bright` version. So you
can get a bright magenta background by writing `bg bright magenta` or
`bg bright c5`

### Named RGB codes

Using a terminal palette is useful, but sometimes you might want to
There are 658 named full RGB colours. These names come from the
[[https://en.wikipedia.org/wiki/X11_color_names][original X11 colour-set]] which can be found littering the internet. The
file and its license can be found in the [[etc/][etc/]] directory of this
project.

Using these are easy - prefix the colour name with `fg-` or `bg-` to
indicate if you want to colour the text or the background. The hyphen
is needed to differentiate from the named reserved colours.

```ini
  [PROMPT]
  # Cadet Blue text, with empty attribute to reset the colour
  prompt="%{fg-cadetblue}TEXT%{} $ "
```

Note: it's sometimes hard to know what colours look nice. I've
supplied a small script in `scripts/print_colours.pl` which prints all
the colours with their names. Run it in your terminal like this:
  
```bash
  $ path/to/repo/scripts/print_colours.pl
```

### Combining attributes

To save space and to avoid cognitive overload, it's possible to
combine attributes. For example, if you want:
- foreground palegreen2
- background grey10
- bold
- strikethrough

Then you can do this:

```ini
  [PROMPT]
  prompt="%{fg-palegreen2, bg-grey10, bold, strikethrough}TEXT%{} $ "
```

### Note: Plain terminal escape codes

These aren't really attributes - it's just the normal terminal escape
codes. If you want to insert a terminal bell (`\[\a\]`) or for some
reason want to use the escape sequence for an RGB colour
(`\[\e[38;2;30;30;30\]`), prompt2 will ignore this as long as you wrap
the code in escaped brackets as specified in the section `[PROMPTING]`
in the bash man-page.

## Widgets

Widgets are the dynamic elements in your prompt. We've already seen
two types: `@{CWD}` and `@{SYS.promptchar}` - but there's many more:

```
CWD                          # current working directory
Repo.is_git_repo             # if . is a git repo
Repo.name                    # name of git repo
Repo.branch_name             # name of branch
Repo.rebase_active           # if rebase is in progress
Repo.conflicts               # number of conflicts in repo
Repo.ahead                   # number of commits ahead of remote
Repo.behind                  # number of commits behind remote
Repo.staged                  # number of staged files
Repo.modified                # number of changed modified files
Repo.untracked               # number of untracked files
AWS.token_is_valid           # if there is a valid AWS SSO token
AWS.token_remaining_hours    # AWS SSO token: how many hours are remaining
AWS.token_remaining_minutes  # AWS SSO token: how many minutes are remaining
SYS.username                 # username
SYS.hostname                 # hostname
SYS.uid                      # the effective uid of the user
SYS.gid                      # the effective gid of the user
SYS.promptchar               # set to '#' if the uid==0, '$' otherwise
SPC                          # spacefiller (see below)
```

All but the `@{SPC}` can be active or inactive. Each of these two
states have its own text replacement and colour.


Notes on two special widgets:
- `CWD`: This widget, which prints the path to your location in the
  filesystem, will automatically be truncated if the path won't fit in
  your terminal.
- `SPC`: Can be used once on each line of the prompt to align the
  remaining widgets to the right of the terminal.

All widgets are case-insensitive.

## Customising Widgets

So far, we've only been configuring the prompt itself. But you can
also configure the widgets themselves - what the widgets should be
replaced with when active or inactive and what colours to use.

To customise the Default settings for all widgets, you can modify its
settings in the INI file as follows:

The default hard-coded config looks like this:

- `max_width`: 256 characters
- `string_active`: the format string "%s"
- `string_inactive`: the format string "%s"
- `colour_on`: no style
- `colour_off`: no style

These can be overridden with your own defaults, by creating your own
`[WIDGET_DEFAULT]` section like this:

```ini
  [WIDGET_DEFAULT]
  max_width=128              # shorten if longer than this
  string_active="%s"
  string_inactive="%s"
  colour_on="%{fg bright cyan}"
  colour_off="%{fg cyan; dim}"
```

- `max_width`: The space a widget takes in the prompt, sans terminal
  escape codes. All widgets will be shortened on the right edge except
  for CWD where the beginning is cut away. Like mentioned above, CWD
  is automatically shortened if it doesn't fit in the terminal (so you
  only need to set `max_width` if you want to).

- `string_active` and `string_inactive`: These parameters define the
  format strings used when the widget is active or inactive. It's
  inserted directly into the printf (or snprintf) function, so you can
  use what you know from that, if you want. Otherwise, the important
  thing is that `%s` is the placeholder for the value of the widget.
  Use that and you'll be all set.

- `colour_on` and `colour_off`: These parameters control the text
  attributes for the active and inactive states of the widget. See
  [[#style-attributes][Style Attributes]] for details on what these are.

  For example, to set the foreground colour to a specific shade of
  gold using RGB values, you would use `colour_on="%{fg-goldenrod}"`.
  Similarly, to set a background colour using RGB, you might use
  `colour_off="%{bg-grey10}"`.


### Overriding the default widget

Each individual widget can override the default config, for example
for `Repo.branch_name` here, where we normally would shorten longer
branch names to save space in the terminal.

```ini
  [Repo.branch_name]
  max_width=20
```

We might also want to override `string_inactive` to show a hyphen if
there is no defined branch name yet. Remember: the other settings are
inherited from the default, so we don't need to tell it to print the
branch name if there is one.

```ini
  [Repo.branch_name]
  max_width=20
  string_inactive="-"
```

Here's another example, where prompt2 will only show the conflict
widget if there is a git conflict. In other situations, it will not be
visible.

```ini
  [Repo.conflicts]
  string_active="<conflicts: %s>"
  string_inactive=""
  colour_on="%{fg-orangeRed1}"
  colour_off=""
```

### Nesting widgets

prompt2 supports nested widgets (only one layer deep). Here's an
example with some annotation to make things a little easier to
understand.

```ini
  # Here we have a simple prompt which has one widget in it -
  # AWS.token_is_valid. This widget is active if you have a valid aws
  # token right now.

  [PROMPT]
  prompt="@{AWS.token_is_valid} $ "

  # The widget in question will, if active (valid token), be replaced
  # with the two widgets below, with some decorations.
  # As you can see, when it's inactive, then it's replaced with nothing.
  [AWS.token_is_valid]
  string_active="<@{AWS.token_remaining_hours}:@{AWS.token_remaining_minutes}>"
  string_inactive=""

  # The minute portion of the token's remaining validity will be shown
  # regardless of if it's active or not. However, if the remaining time
  # goes under a threshold of ten minutes, it will go active - and
  # change to a red colour.
  [AWS.token_remaining_minutes]
  colour_on="%{fg-red1}"
  colour_off=""
```


[Back to README](./)
