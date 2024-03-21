# This file is meant to be sourced!


# Get the full path to the prompt2 binary
CONFIG_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT_BIN=$(realpath $CONFIG_DIR/../bin/prompt2)
unset CONFIG_DIR

# Set the prompt to use for a non-git repo directory
# Note that it's just a plain terminal prompt string. I took
# this from my vanilla PS1
export GP2_NON_GIT_PROMPT="\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "

# Set the prompt to use in a git repo
export GP2_GIT_PROMPT="<@{Repo.name}><@{Repo.branch_name}> b/a(@{Repo.behind},@{Repo.ahead}) s(@{Repo.staged}) m(@{Repo.modified}) u(@{Repo.untracked}) @{CWD}\n$ ";

# Set how I want my CWD to look
export GP2_CWD_TYPE="basename" # can be: 'home', 'git', 'basename', 'full'


# Simplest shell function to set PS1 to the output of prompt2
prompt_cmd() {
  PS1="$($PROMPT_BIN)"
}

# Make this function run every time I hit enter
PROMPT_COMMAND=prompt_cmd
