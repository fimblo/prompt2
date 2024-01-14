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


struct CurrentState {
  // internal - probably uninteresting for user
  git_repository  *repo_obj;
  const char      *repo_path;
  git_reference   *head_ref;
  const git_oid   *head_oid;
  git_status_list *status_list;


  // external - probably useful for user
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

/**
 * Sets up CurrentState with default values
 */
void initialiseState(struct CurrentState *state);

/**
 * Given a path, returns root of git repo or empty string
 */
const char *findGitRepositoryPath(const char *path);

/**
 * Gather all git-related context.
 */
int gatherGitContext(struct CurrentState *state);

/**
 * Check the validity of the AWS SSO login token and calculates the
 * remaining time until the token expires
 */
int gatherAWSContext(struct CurrentState *state);

/**
 * Retrieve the full path of the current working directory
 */
const char *getCWDFull(struct CurrentState *state);

/**
 * Obtain the basename (the last component) of the current working directory
 */
const char *getCWDBasename(struct CurrentState *state);

/**
 * Generate a path relative to the root of the Git repository, using
 * '+' to represent the root
 */
const char *getCWDFromGitRepo(struct CurrentState *state);

/**
 * Retrieve the current working directory path, replacing the home
 * directory part with '~' if applicable
 */
const char *getCWDFromHome(struct CurrentState *state);

/**
 * Memory management
 */
void cleanupResources(struct CurrentState *state);

/**
 * Shortens a filesystem path to a specified maximum width by
 * truncating the beginning of the string
 */
void pathTruncateSimple(char *originalPath, int maxWidth);

/**
 * Shortens a filesystem path to a specified maximum width by
 * abbreviating intermediate directories while keeping the last
 * directory in full.
 */
void pathTruncateAccordion(char *originalPath, int maxWidth);

#endif //GETSTATUS_H
