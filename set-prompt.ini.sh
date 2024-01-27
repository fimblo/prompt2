# Source me!
REPO_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT_BIN=$(realpath $REPO_DIR/bin/prompt)
unset REPO_DIR

export GP2_NON_GIT_PROMPT="${TITLEBAR}\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "
export GP2_GIT_PROMPT="<@{Repo.name}><@{Repo.branch.name}> b/a(@{Diverge.behind},@{Diverge.ahead}) s(@{Staged.num}) u(@{Unstaged.num}) n(@{Untracked.num}) @{CWD.home_path}\n$ ";

prompt_cmd() {
  PS1="$($PROMPT_BIN)"
}

PROMPT_COMMAND=prompt_cmd
