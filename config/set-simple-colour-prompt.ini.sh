# Source me!
REPO_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT_BIN=$(realpath $REPO_DIR/bin/prompt)
unset REPO_DIR

export GP2_NON_GIT_PROMPT="${TITLEBAR}\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "


# These are normal terminal colour codes. They pass through the prompt
# binary and are interpreted by the terminal.
green="\[\033[01;32m\]"
blue="\[\033[01;34m\]"
reset="\[\033[00m\]"

GP2_GIT_PROMPT="${green}<@{Repo.name}><@{Repo.branch_name}>${reset}"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT} b/a(@{Repo.behind},@{Repo.ahead})"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT} s/m/u(@{Repo.staged},@{Repo.modified},@{Repo.untracked})"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT} ${blue}@{CWD}${reset}\n$ ";
export GP2_GIT_PROMPT

export GP2_CWD_TYPE="home" # can be: 'home', 'git', 'basename', 'full'

prompt_cmd() {
  PS1="$($PROMPT_BIN)"
}

PROMPT_COMMAND=prompt_cmd
