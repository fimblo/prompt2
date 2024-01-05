#ifndef GETSTATUS_H
#define GETSTATUS_H
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


// data for internal functions
struct RepoContext {
  git_repository  *repo_obj;
  const char      *repo_path;

  git_reference   *head_ref;
  const git_oid   *head_oid;

  git_status_list *status_list;
};

// data for the user
struct CurrentState {
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

  int aws_token_is_valid;
  int aws_token_remaining_hours;
  int aws_token_remaining_minutes;
};

void setDefaultValues(struct RepoContext *repo_context, struct CurrentState *state);
int populateRepoContext(struct RepoContext *context, const char *path);
const char * getRepoName(struct RepoContext *context, struct CurrentState *state);
const char * getBranchName(struct RepoContext *context, struct CurrentState *state);
int getRepoStatus(struct RepoContext *context, struct CurrentState *state);
int getRepoDivergence(struct RepoContext *context,
                       struct CurrentState *state);
int checkForInteractiveRebase(struct RepoContext *context, struct CurrentState *state);
int getAWSContext(struct CurrentState *state);
const char *getCWDFull(struct CurrentState *state);
const char *getCWDBasename(struct CurrentState *state);
const char *getCWDFromGitRepo(struct RepoContext *context, struct CurrentState *state);
const char *getCWDFromHome(struct CurrentState *state);
void cleanupResources(struct RepoContext *context);

#endif //GETSTATUS_H
