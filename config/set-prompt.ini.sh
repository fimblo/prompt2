# This file is meant to be sourced!


# Get the full path to the prompt2 binary
CONFIG_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT2_BIN=$(realpath $CONFIG_DIR/../bin/prompt2)
PROMPT2_CONFIG=$(realpath "$CONFIG_DIR/dot.prompt2_config.ini")


# Simplest shell function to set PS1 to the output of prompt2
prompt_cmd() {
  PS1="$($PROMPT2_BIN $PROMPT2_CONFIG)"
}

# Make this function run every time I hit enter
PROMPT_COMMAND=prompt_cmd

unset CONFIG_DIR
