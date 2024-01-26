# Source me!
export GP2_PROMPT="<@{Repo.name}><@{Repo.branch.name}> b/a(@{Diverge.behind},@{Diverge.ahead}) s(@{Staged.num}) u(@{Unstaged.num}) @{CWD.home_path}\n$ ";

prompt_cmd() {
  PS1="$(bin/prompt)"
}

PROMPT_COMMAND=prompt_cmd
