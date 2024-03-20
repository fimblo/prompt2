# Prompt Configuration Proposal

## Preamble
Despite the authoritative way this file is written, none of this is done yet
(except the label names). I'd like to gather some ideas/feedback first.

I'm testing the github issues manager - if you have ideas, please add a note in this issue:
https://github.com/fimblo/prompt2/issues/26 

## Summary
This file outlines the configuration schema for customizing the
command-line prompt. It specifies how to set different labels that
change dynamically based on the state of the working directory, git
repository status, and AWS token validity. Each label can be
configured to display specific information when its condition is
either OFF (inactive/default) or ON (active/non-default).

## Labels and Conditions

| LABEL                          | OFF          | ON          |
| ------------------------------ | ------------ | ----------- |
| `CWD.full`                     | empty string | string      |
| `CWD.basename`                 | empty string | string      |
| `CWD.git_path`                 | empty string | string      |
| `CWD.home_path`                | empty string | string      |
| `Repo.name`                    | empty string | string      |
| `Repo.branch_name`             | empty string | string      |
| `Repo.is_git_repo`             | 0            | !0          |
| `Repo.rebase_active`           | 0            | !0          |
| `Repo.conflicts`               | 0            | !0          |
| `Repo.has_upstream`            | 0            | !0          |
| `Repo.ahead`                   | 0            | !0          |
| `Repo.behind`                  | 0            | !0          |
| `Repo.staged`                  | 0            | !0          |
| `Repo.modified`                | 0            | !0          |
| `Repo.untracked`               | 0            | !0          |
| `AWS.token_is_valid`           | 0            | !0          |
| `AWS.token_remaining_hours`    | >0           | <=0         |
| `AWS.token_remaining_minutes`  | >10          | <=10        |


## Format

Replace `<label-name>` with any of the labels above. '%v' is used as
a placeholder for the value of the label.


These first are used to decide what to print if the label is off
(inactive) or on (active).

```bash
export GP2_<label-name>_STRING_OFF="...%v..." 
export GP2_<label-name>_STRING_ON="...%v..."
```

These two are used to decide if and what colours to use for the
label in ON or OFF state.

```bash
export GP2_<label-name>_COLOUR_OFF="term-escape-sequence"
export GP2_<label-name>_COLOUR_ON="term-escape-sequence"
```

Examples

Repo.rebase_active: If the value of Repo.rebase_active is 0, then
insert the empty string. If it's not 0, then insert the string
'<interactive rebase>' in the prompt.

```bash
export GP2_Repo.rebase_active_STRING_OFF=""                    
export GP2_Repo.rebase_active_STRING_ON="<interactive rebase>"
```
  
Repo.conflicts: If the value of Repo.conflicts is 0, then insert the
empty string. If it's not 0, then insert the string '<conflict:%v>' in
the prompt, where %v is the number of conflicts in the git repo.

```bash
export GP2_Repo.conflicts_STRING_OFF=""
export GP2_Repo.conflicts_STRING_ON="<conflict:%v>"
```

Repo.staged: If the value of Repo.staged is 0, then insert a '0',
coloured green into the prompt. If it's not 0, then insert the number
of staged files in the colour red.
  
```bash
export GP2_Repo.staged_STRING_OFF="(-)"
export GP2_Repo.staged_STRING_ON="(%v)"                     
export GP2_Repo.staged_COLOUR_OFF="" # no colour
export GP2_Repo.staged_COLOUR_ON="\[\033[01;32m\]" # green
```


Example C code to use these variables

```c
const char* staged_string_off = getenv("GP2_Repo.staged_STRING_OFF") ?: "(-)";
const char* staged_string_on = getenv("GP2_Repo.staged_STRING_ON") ?: "(%v)";
const char* staged_colour_off = getenv("GP2_Repo.staged_COLOUR_OFF") ?: ""; // Default no colour
const char* staged_colour_on = getenv("GP2_Repo.staged_COLOUR_ON") ?: "\033[01;32m"; // Default green
```
