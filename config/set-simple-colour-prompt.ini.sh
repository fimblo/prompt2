# Source me!
CONFIG_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT_BIN=$(realpath $CONFIG_DIR/../bin/prompt)
unset CONFIG_DIR

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


# CWD.full /home/fimblo/github/prompt2
# CWD.basename prompt2
# CWD.git_path +/
# CWD.home_path ~/github/prompt2
# Repo.name prompt2
# Repo.branch_name main
# Repo.is_git_repo 1


# Repo.rebase_active 0                 0 | !0
# Repo.conflicts 0                     0 | !0
# Repo.has_upstream 1                  0 | !0
# Repo.ahead 0                         0 | !0
# Repo.behind 0                        0 | !0
# Repo.staged 0                        0 | !0
# Repo.modified 0                      0 | !0
# Repo.untracked 1                     0 | !0
# AWS.token_is_valid 0                 0 | !0
# AWS.token_remaining_minutes -1      <5 | >=5


