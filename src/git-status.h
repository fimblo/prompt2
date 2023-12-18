#ifndef GITSTATUS_H
#define GITSTATUS_H
/*
  header file for git-status.c
*/
#include <git2.h>


enum states {
  NO_DATA,
  NO_UPSTREAM,
  UP_TO_DATE,
  MODIFIED,
  ENUM_SIZE
};

extern const char *state_names[ENUM_SIZE];

enum exit_code {
  // success codes
  EXIT_GIT_PROMPT        =  0,
  EXIT_DEFAULT_PROMPT    =  1,
  EXIT_ABSENT_LOCAL_REF  =  2,

  // failure codes
  EXIT_FAIL_GIT_STATUS   = -1,
  EXIT_FAIL_REPO_OBJ     = -2,
};

// data for internal functions
struct RepoContext {
  git_repository  *repo_obj;
  const char      *repo_path;

  git_reference   *head_ref;
  const git_oid   *head_oid;

  git_status_list *status_list;
};

// data for the user
struct RepoStatus {
  const char *cwd_full;
  const char *cwd_basename;
  const char *cwd_git_path;
  

  const char *repo_name;
  const char *branch_name;

  int status_repo;
  int ahead;
  int behind;

  int status_staged;
  int staged_changes_num;

  int status_unstaged;
  int unstaged_changes_num;

  int conflict_num;
  int rebase_in_progress;
};

void initializeRepoContext(struct RepoContext *repo_context);
void initializeRepoStatus(struct RepoStatus *repo_status);
int populateRepoContext(struct RepoContext *context, const char *path);
const char * getRepoName(struct RepoContext *context, struct RepoStatus *status);
const char * getBranchName(struct RepoContext *context, struct RepoStatus *status);
int getRepoStatus(struct RepoContext *context, struct RepoStatus *status);
int getRepoDivergence(struct RepoContext *context,
                       struct RepoStatus *status);

const char *getCWDFull(struct RepoStatus *status);
const char *getCWDBasename(struct RepoStatus *status);
const char *getCWDFromGitRepo(struct RepoContext *context, struct RepoStatus *status);
const char *getCWDFromHome(struct RepoStatus *status);
void cleanupResources(struct RepoContext *context);

#endif //GITSTATUS_H
