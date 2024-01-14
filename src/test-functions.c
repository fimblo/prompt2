#include <git2.h>
#include <stdio.h>

#include "get-status.h"

//int main(int argc, char *argv[]) {
int main(void) {
  struct CurrentState state;

  git_libgit2_init();
  initialiseState(&state);

  gatherGitContext(&state);


  printf("CWD.full %s\n",           getCWDFull(&state));
  printf("CWD.basename %s\n",       getCWDBasename(&state));
  printf("CWD.git_path %s\n",       getCWDFromGitRepo(&state));
  printf("CWD.home_path %s\n",      getCWDFromHome(&state));

  printf("Repo.name %s\n",          state.repo_name);
  printf("Repo.branch.name %s\n",   state.branch_name);
  printf("Repo.rebase_active %d\n", state.rebase_in_progress);
  printf("Repo.conflict.num %d\n",  state.conflict_num);

  printf("Diverge.status %s\n",     state_names[state.status_repo]);
  printf("Diverge.ahead %d\n",      state.ahead);
  printf("Diverge.behind %d\n",     state.behind);


  printf("Staged.status %s\n",      state_names[state.status_staged]);
  printf("Staged.num %d\n",         state.staged_changes_num);
  printf("Unstaged.status %s\n",    state_names[state.status_unstaged]);
  printf("Unstaged.num %d\n",       state.unstaged_changes_num);


  gatherAWSContext(&state);
  printf("AWS.token_is_valid %d\n", state.aws_token_is_valid);
  printf("AWS.token_remaining_hours %d\n", state.aws_token_remaining_hours);
  printf("AWS.token_remaining_minutes %d\n", state.aws_token_remaining_minutes);

  cleanupResources(&state);
  git_libgit2_shutdown();
  return 0;
}
