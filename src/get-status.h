#ifndef GETSTATUS_H
#define GETSTATUS_H
/*
  header file for git-status.c
*/
#include <git2.h>


/*
  The prompt can be 4 * PATH_MAX (4 * 4096)

  This should be enough for many multi-line prompts with an absurd number of
  many escape characters
*/
#define PROMPT_MAX_LEN 16384


enum generic_return_values {
  SUCCESS =  0,
  FAILURE =  1,
  ERROR   = -1,
};

enum aws_related_return_values {
  SUCCESS_HAS_AWS_CONFIG    = 0,
  FAILURE_HAS_NO_AWS_CONFIG = 1,
};

enum git_related_return_values {
  SUCCESS_IS_GIT_REPO        = 0,
  FAILURE_IS_NOT_GIT_REPO    = 1,
  FAILURE_NO_GIT_UPSTREAM    = 2,
  
  ERROR_GIT_REVWALK_CREATION = -2,
  ERROR_GIT_REVWALK_FORWARD  = -3,
  ERROR_GIT_REVWALK_BACKWARD = -4,
  ERROR_GIT_NO_HEAD_REF      = -5,
};

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
  int conflict_num;
  int is_rebase_in_progress;

  int ahead_num;
  int behind_num;
  int staged_num;
  int modified_num;
  int untracked_num;

  int aws_token_is_valid; // 0 if invalid, 1 if valid, -1 if error
  int aws_token_remaining_hours;
  int aws_token_remaining_minutes;
};

/**
 * 
 * Sets up CurrentState with default values
 */
void initialise_state(struct CurrentState *state);


/**
 * Gather all git-related context.
 * @returns 0 if . is inside a git-repo, 1 otherwise
 */
int gather_git_context(struct CurrentState *state);


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
int gather_aws_context(struct CurrentState *state);

/**
 * Generate a path relative to the root of the Git repository, using
 * '+' to represent the root
 */
const char *get_cwd_from_gitrepo(struct CurrentState *state);

/**
 * Retrieve the current working directory path, replacing the home
 * directory part with '~' if applicable
 */
const char *get_cwd_from_home(struct CurrentState *state);

/**
 * Memory management
 */
void cleanup_resources(struct CurrentState *state);

/**
 * Shortens a filesystem path to a specified maximum width by
 * truncating the beginning of the string
 */
void path_truncate_simple(char *original_path, int max_width);

/**
 * Shortens a filesystem path to a specified maximum width by
 * abbreviating intermediate directories while keeping the last
 * directory in full.
 */
void path_truncate_accordion(char *original_path, int max_width);


#endif //GETSTATUS_H
