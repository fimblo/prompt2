CONFIG_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT_BIN=$(realpath $CONFIG_DIR/../bin/prompt)
unset CONFIG_DIR

export GP2_NON_GIT_PROMPT="${TITLEBAR}\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "
export GP2_GIT_PROMPT="<@{Repo.name}><@{Repo.branch_name}> b/a(@{Repo.behind},@{Repo.ahead}) s(@{Repo.staged}) m(@{Repo.modified}) u(@{Repo.untracked}) @{CWD}\n$ ";
export GP2_CWD_TYPE="home" # can be: 'home', 'git', 'basename', 'full'

prompt_cmd() {
  PS1="$($PROMPT_BIN)"
}

PROMPT_COMMAND=prompt_cmd
