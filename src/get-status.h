#ifndef GETSTATUS_H
#define GETSTATUS_H
/*
  header file for git-status.c
*/
#include <git2.h>


enum aws_related_return_values {
  SUCCESS_HAS_AWS_CONFIG    = 0,
  FAILURE_HAS_NO_AWS_CONFIG = 1,
};

enum git_related_return_values {
  SUCCESS_IS_GIT_REPO        = 0,
  FAILURE_IS_NOT_GIT_REPO    = 1,
  
  /*
   * FAILURE_GIT_UPSTREAM_UNKNOWN
   * 
   * If the upstream_oid is NULL, it could be because the reference is
   * unset, or broken, or has not been fetched yet. Regardless, we
   * won't be able to tell the user if and how much we are ahead or
   * behind upstream.
   * 
  */
  FAILURE_GIT_UPSTREAM_UNKNOWN = 2,

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

  const char *username;
  const char *hostname;
  int uid;
  int gid;

  int is_git_repo;
  int is_nascent_repo;
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
 * Gather regular system context
 * 
 * Currently this covers:
 * - username
 * - hostname
*/
int gather_system_context(struct CurrentState *state);


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
 * CWD selector
 */
char * get_cwd(struct CurrentState *state, const char *cwd_type);


/**
 * Memory management
 */
void cleanup_resources(struct CurrentState *state);



#endif //GETSTATUS_H
