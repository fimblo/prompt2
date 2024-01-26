# Source me!

prompt_cmd() {
  PS1="$(bin/prompt)"
}

PROMPT_COMMAND=prompt_cmd
