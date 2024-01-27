# Source me!
export GP2_NON_GIT_PROMPT="${TITLEBAR}\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "
export GP2_GIT_PROMPT="<@{Repo.name}><@{Repo.branch.name}> b/a(@{Diverge.behind},@{Diverge.ahead}) s(@{Staged.num}) u(@{Unstaged.num}) @{CWD.home_path}\n$ ";

prompt_cmd() {
  PS1="$(bin/prompt)"
}

PROMPT_COMMAND=prompt_cmd
