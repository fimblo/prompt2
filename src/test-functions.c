#include <stdio.h>
#include <git2.h>
#include "git-status.h"

//int main(int argc, char *argv[]) {
int main(void) {
  struct RepoContext context;
  struct RepoStatus status;

  git_libgit2_init();
  setDefaultValues(&context, &status);

  populateRepoContext(&context, ".");
  getRepoStatus(&context, &status);

  getRepoDivergence(&context, &status);


  printf("CWD.full %s\n",         getCWDFull(&status));
  printf("CWD.basename %s\n",     getCWDBasename(&status));
  printf("CWD.git_path %s\n",     getCWDFromGitRepo(&context, &status));
  printf("CWD.home_path %s\n",    getCWDFromHome(&status));
  
  printf("Repo.name %s\n",        getRepoName(&context, &status));
  printf("Repo.branch.name %s\n", getBranchName(&context, &status));

  printf("Repo.status %s\n",      state_names[status.status_repo]);
  printf("Repo.ahead %d\n",       status.ahead);
  printf("Repo.behind %d\n",      status.behind);


  printf("Staged.status %s\n",    state_names[status.status_staged]);
  printf("Staged.num %d\n",       status.staged_changes_num);
  printf("Unstaged.status %s\n",  state_names[status.status_unstaged]);
  printf("Unstaged.num %d\n",     status.unstaged_changes_num);


  cleanupResources(&context);
  git_libgit2_shutdown();
  return 0;
}
