#ifndef GETSTATUS_H
#define GETSTATUS_H
/*
  header file for git-status.c
*/
#include <git2.h>


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

  int is_git_repo;

  int has_upstream;

  int ahead_num;
  int behind_num;
  int staged_num;
  int modified_num;
  int untracked_num;

  int conflict_num;
  int is_rebase_in_progress;

  int aws_token_is_valid; // 0 if invalid, 1 if valid, -1 if error
  int aws_token_remaining_hours;
  int aws_token_remaining_minutes;
};

/**
 * Sets up CurrentState with default values
 */
void initialiseState(struct CurrentState *state);

/**
 * Checks if the given path is a git repository
 */
int isGitRepo(const char *path);

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
 * remaining time until the token expires.
 * 
 * Note that this function breaks convention by returning 1 if
 * successful(valid), to mirror the value stored in
 * `state->aws_token_is_valid`.
 *
 * @return -1 for errors, 0 if token is invalid, 1 if it's valid.
   */
int gatherAWSContext(struct CurrentState *state);

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
