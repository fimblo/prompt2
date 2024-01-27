# Source me!
REPO_DIR=$(dirname ${BASH_SOURCE[0]})
PROMPT_BIN=$(realpath $REPO_DIR/bin/prompt)
unset REPO_DIR

unset GP2_GIT_PROMPT
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}CWD.full            '@{CWD.full}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}CWD.basename        '@{CWD.basename}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}CWD.git_path        '@{CWD.git_path}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}CWD.home_path       '@{CWD.home_path}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.is_git_repo    '@{Repo.is_git_repo}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.name           '@{Repo.name}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.branch.name    '@{Repo.branch.name}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.rebase_active  '@{Repo.rebase_active}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Repo.conflict.num   '@{Repo.conflict.num}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Diverge.status      '@{Diverge.status}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Diverge.ahead       '@{Diverge.ahead}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Diverge.behind      '@{Diverge.behind}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Staged.status       '@{Staged.status}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Staged.num          '@{Staged.num}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Unstaged.status     '@{Unstaged.status}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}Unstaged.num        '@{Unstaged.num}'\n"

GP2_GIT_PROMPT="${GP2_GIT_PROMPT}AWS.token_is_valid          '@{AWS.token_is_valid}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}AWS.token_remaining_hours   '@{AWS.token_remaining_hours}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}AWS.token_remaining_minutes '@{AWS.token_remaining_minutes}'\n"
GP2_GIT_PROMPT="${GP2_GIT_PROMPT}$ "

export GP2_GIT_PROMPT

prompt_cmd() {
  PS1="$($PROMPT_BIN)"
}

PROMPT_COMMAND=prompt_cmd
