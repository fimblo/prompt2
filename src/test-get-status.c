/*
 * test-get-status.c
 *
 * Dumps the current environment state for use in dynamic shell prompt
 * generation. Primarily used for testing get-status.c
 *
 * This utility acts as a helper binary for prompt2. It serves to
 * output all the contextual data that can be utilized by prompt2 to
 * construct a dynamic shell prompt.
 *
 * Each row has two values, whitespace separated:
 *
 * - Widget token: the string representing a widget. Used in
 *   `.prompt2_config.ini` to insert a widget.
 * - Widget value: the current value of the widget.
 *
 */

#include <git2.h>
#include <stdio.h>

#include "constants.h"
#include "get-status.h"

int main(void) {
  struct CurrentState state;

  git_libgit2_init();
  initialise_state(&state);
  gather_git_context(&state);

  const char* cwd_from_gitrepo = get_cwd_from_gitrepo(&state);
  const char* cwd_from_home = get_cwd_from_home(&state);

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


  gather_aws_context(&state);
  printf("AWS.token_is_valid %d\n", state.aws_token_is_valid);
  printf("AWS.token_remaining_hours %d\n", state.aws_token_remaining_hours);
  printf("AWS.token_remaining_minutes %d\n", state.aws_token_remaining_minutes);

  gather_system_context(&state);
  printf("SYS.username %s\n",         state.username);
  printf("SYS.hostname %s\n",         state.hostname);
  printf("SYS.uid %d\n",              state.uid);
  printf("SYS.gid %d\n",              state.gid);
  printf("SYS.promptchar %s\n",       state.uid ? "$" : "#");


  cleanup_resources(&state);
  git_libgit2_shutdown();
  return SUCCESS; // enforce happy ending
}
