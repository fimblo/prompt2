# Source me!
CONFIG_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT_BIN=$(realpath $CONFIG_DIR/../bin/prompt)
unset CONFIG_DIR

unset GP2_GIT_PROMPT
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}CWD                 '@{CWD}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.is_git_repo    '@{Repo.is_git_repo}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.name           '@{Repo.name}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.branch_name    '@{Repo.branch_name}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.rebase_active  '@{Repo.rebase_active}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.conflicts      '@{Repo.conflicts}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.has_upstream   '@{Repo.has_upstream}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.ahead          '@{Repo.ahead}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.behind         '@{Repo.behind}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.staged         '@{Repo.staged}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.modified       '@{Repo.modified}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.untracked      '@{Repo.untracked}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}AWS.token_is_valid          '@{AWS.token_is_valid}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}AWS.token_remaining_hours   '@{AWS.token_remaining_hours}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}AWS.token_remaining_minutes '@{AWS.token_remaining_minutes}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}$ "
export GP2_GIT_PROMPT

export GP2_CWD_TYPE="home" # can be: 'home', 'git', 'basename', 'full'


prompt_cmd() {
  PS1="$($PROMPT_BIN)"
}

PROMPT_COMMAND=prompt_cmd
