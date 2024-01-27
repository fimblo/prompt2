#include <git2.h>
#include <stdio.h>

#include "get-status.h"

//int main(int argc, char *argv[]) {
int main(void) {
  struct CurrentState state;

  git_libgit2_init();
  initialiseState(&state);
  gatherGitContext(&state);

  const char* cwd_from_gitrepo = getCWDFromGitRepo(&state);
  const char* cwd_from_home = getCWDFromHome(&state);

  printf("CWD.full %s\n",           state.cwd_full);
  printf("CWD.basename %s\n",       state.cwd_basename);
  printf("CWD.git_path %s\n",       cwd_from_gitrepo);
  printf("CWD.home_path %s\n",      cwd_from_home);

  printf("Repo.is_git_repo %d\n",   state.is_git_repo);
  printf("Repo.name %s\n",          state.repo_name);
  printf("Repo.branch_name %s\n",   state.branch_name);
  printf("Repo.rebase_active %d\n", state.is_rebase_in_progress);
  printf("Repo.conflicts %d\n",     state.conflict_num);

  printf("Repo.has_upstream %d\n",  state.has_upstream);
  printf("Repo.ahead %d\n",         state.ahead_num);
  printf("Repo.behind %d\n",        state.behind_num);

  printf("Repo.staged %d\n",        state.staged_num);
  printf("Repo.modified %d\n",      state.modified_num);
  printf("Repo.untracked %d\n",     state.untracked_num);


  gatherAWSContext(&state);
  printf("AWS.token_is_valid %d\n", state.aws_token_is_valid);
  printf("AWS.token_remaining_hours %d\n", state.aws_token_remaining_hours);
  printf("AWS.token_remaining_minutes %d\n", state.aws_token_remaining_minutes);

  cleanupResources(&state);
  git_libgit2_shutdown();
  return 0;
}
