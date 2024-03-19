#ifdef __linux__
  #define _XOPEN_SOURCE
  #define _POSIX_C_SOURCE 200809L
  #define _GNU_SOURCE
#endif

#include <dirent.h>
#include <git2.h>
#include <json-c/json.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "get-status.h"

/* ================================================== */
/* Helper functions                                   */
/* ================================================== */


/**
 * Checks if the given path is a git repository
 * @return 0 if true, 1 if false
 */
int __is_git_repo(const char *path) {
	int is_repo = FAILURE_IS_NOT_GIT_REPO;
	git_repository *repo = NULL;
	int error = git_repository_open_ext(&repo, path, 0, NULL);
	if (error == 0) {
		is_repo = SUCCESS_IS_GIT_REPO;
		git_repository_free(repo);
	}
	return is_repo;
}

/**
 * Given a path, returns root of git repo or empty string
 */
const char *__find_git_repository_path(const char *path) {
  git_buf repo_path = { 0 };
  int error = git_repository_discover(&repo_path, path, 0, NULL);

  if (error == 0) {
    char *last_slash = strstr(repo_path.ptr, "/.git/");
    if (last_slash) *last_slash = '\0';

    char *result = strdup(repo_path.ptr);
    git_buf_free(&repo_path);
    return result;
  }

  return strdup("");
}


/**
 * Helper: Figure out divergence between local and upstream branches.
 */
int __calculate_divergence(git_repository *repo,
                          const git_oid *local_oid,
                          const git_oid *upstream_oid,
                          int *ahead,
                          int *behind) {
  int ahead_count = 0;
  int behind_count = 0;
  git_oid id;

  // init walker
  git_revwalk *walker = NULL;
  if (git_revwalk_new(&walker, repo) != 0) {
    return ERROR_GIT_REVWALK_CREATION;
  }

  // count number of commits ahead
  if (git_revwalk_push(walker, local_oid)    != 0 ||  // set where I want to start
      git_revwalk_hide(walker, upstream_oid) != 0) {  // set where the walk ends (exclusive)
    git_revwalk_free(walker);
    return ERROR_GIT_REVWALK_FORWARD;
  }
  while (git_revwalk_next(&id, walker) == 0) ahead_count++;

  // count number of commits behind
  git_revwalk_reset(walker);
  if (git_revwalk_push(walker, upstream_oid) != 0 || // set where I want to start
      git_revwalk_hide(walker, local_oid)    != 0) { // set where the walk ends (exclusive)
    git_revwalk_free(walker);
    return ERROR_GIT_REVWALK_BACKWARD;
  }
  while (git_revwalk_next(&id, walker) == 0) behind_count++;

  *ahead = ahead_count;
  *behind = behind_count;

  git_revwalk_free(walker);
  return SUCCESS;
}

/**
 * Helper: Determine if a Git repository is currently in an
 * interactive rebase state
 */
void __check_for_interactive_rebase(struct CurrentState *state) {
  char rebase_merge_path[PATH_MAX];
  char rebase_apply_path[PATH_MAX];
  snprintf(rebase_merge_path, sizeof(rebase_merge_path), "%s/.git/rebase-merge", state->repo_path);
  snprintf(rebase_apply_path, sizeof(rebase_apply_path), "%s/.git/rebase-apply", state->repo_path);

  struct stat merge_stat, apply_stat;
  state->is_rebase_in_progress = 0;
  if (stat(rebase_merge_path, &merge_stat) == 0 || stat(rebase_apply_path, &apply_stat) == 0) {
    state->is_rebase_in_progress = 1;
  }
}

/**
 * Helper: set RepoContext with repo name and return it
 */
const char * __get_repo_name(struct CurrentState *state) {
  if (state->head_ref == NULL) {
    state->repo_name = "NO_DATA";
  } else {
    state->repo_name = strrchr(state->repo_path, '/') + 1;
  }
  return state->repo_name;
}

/**
 * Helper: set RepoContext with repo branch and return it
 */
const char * __get_branch_name(struct CurrentState *state) {
  if (state->head_ref == NULL) {
    state->branch_name = "NO_DATA";
  } else {
    state->branch_name = git_reference_shorthand(state->head_ref);
  }
  return state->branch_name;
}

/**
 * Helper: Prep RepoContext with git repo info
 */
int __populate_repo_context(struct CurrentState *state, const char *path) {
  int result = FAILURE; // Default to failure

  const char *git_repository_path = NULL;
  git_repository *repo = NULL;
  git_reference *head_ref = NULL;
  const git_oid *head_oid = NULL;

  if (state->repo_path == NULL) {
    git_repository_path = __find_git_repository_path(path);
    if (strlen(git_repository_path) == 0) {
      goto cleanup; // Path not found, cleanup and exit
    }
  } else {
    git_repository_path = state->repo_path;
  }

  if (git_repository_open(&repo, git_repository_path) != 0) {
    goto cleanup; // Failed to open repository, cleanup and exit
  }

  if (git_repository_head(&head_ref, repo) != 0) {
    goto cleanup; // Failed to get repository head, cleanup and exit
  }
  head_oid = git_reference_target(head_ref);

  // Success, update state
  state->repo_path = git_repository_path;
  state->repo_obj = repo;
  state->head_ref = head_ref;
  state->head_oid = head_oid;

  return SUCCESS;

cleanup:
  if (repo != NULL) {
    git_repository_free(repo);
  }
  if (git_repository_path != NULL && state->repo_path == NULL) {
    free((void *)git_repository_path);
  }
  
  return result;
}

/**
 * Helper: Get the current Git repository's status, including staged
 * and modified changes, and conflicts.
 */
int __get_repo_status(struct CurrentState *state) {
  if (state->head_ref == NULL) return ERROR_GIT_NO_HEAD_REF;

  // First get the status-list which we'll iterate through
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
#pragma GCC diagnostic pop

  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_INCLUDE_UNTRACKED;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, state->repo_obj, &opts) != 0) {
    git_reference_free(state->head_ref);
    git_repository_free(state->repo_obj);
    free((void *) state->repo_path);
    return FAILURE_IS_NOT_GIT_REPO;
  }
  state->status_list = status_list;


  // Now iterate
  int staged_changes   = 0;
  int modified_changes = 0;
  int conflicts        = 0;
  int untracked        = 0;

  int status_count = git_status_list_entrycount(status_list);
  for (int i = 0; i < status_count; i++) {
    const git_status_entry *entry = git_status_byindex(status_list, i);
    if (entry->status == GIT_STATUS_CURRENT) continue;

    // Check for conflicts
    if (entry->status & GIT_STATUS_CONFLICTED)
      conflicts++;

    // Check for staged changes
    if (entry->status & (GIT_STATUS_INDEX_NEW      |
                         GIT_STATUS_INDEX_MODIFIED |
                         GIT_STATUS_INDEX_RENAMED  |
                         GIT_STATUS_INDEX_DELETED  |
                         GIT_STATUS_INDEX_TYPECHANGE)) {
      staged_changes++;
    }

    // Check for modified changes
    if (entry->status & (GIT_STATUS_WT_MODIFIED |
                         GIT_STATUS_WT_DELETED  |
                         GIT_STATUS_WT_RENAMED  |
                         GIT_STATUS_WT_TYPECHANGE)) {
      modified_changes++;
    }

    if (entry->status & GIT_STATUS_WT_NEW) {
      untracked++;
    }
  }

  state->staged_num    = staged_changes;
  state->modified_num  = modified_changes;
  state->untracked_num = untracked;
  state->conflict_num  = conflicts;

  __check_for_interactive_rebase(state);
  return SUCCESS;
}

/**
 * Helper: Calculate the divergence of the current Git repository from
 * its upstream branch, updating the state with information on how
 * many commits it is ahead or behind.
 */
int __get_repo_divergence(struct CurrentState *state) {
  if (state->head_ref == NULL) return ERROR_GIT_NO_HEAD_REF;

  char full_remote_branch_name[128];
  sprintf(full_remote_branch_name, "refs/remotes/origin/%s", git_reference_shorthand(state->head_ref));

  git_reference *upstream_ref = NULL;
  const git_oid *upstream_oid;
  const int retval =
    git_reference_lookup(&upstream_ref,
                         state->repo_obj,
                         full_remote_branch_name);
  if (retval != 0) {
    // If there is no upstream ref, this is a stand-alone branch
    state->has_upstream = 0;
    git_reference_free(upstream_ref);
    return FAILURE_NO_GIT_UPSTREAM;
  }

  upstream_oid = git_reference_target(upstream_ref);

  // if the upstream_oid is null, we assume it's a stand-alone branch.
  // Not certain about this. TODO: check
  if (upstream_oid == NULL) {
    state->has_upstream = 0;
    return FAILURE_NO_GIT_UPSTREAM;
  }
  state->has_upstream = 1;

  __calculate_divergence(state->repo_obj,
                        state->head_oid,
                        upstream_oid,
                        &state->ahead_num,
                        &state->behind_num);

  git_reference_free(upstream_ref);
  return SUCCESS;
}




/* ================================================== */
/* Exported functions                                 */
/* ================================================== */

/**
 * Sets up CurrentState so that they are useable.
 */
void initialise_state(struct CurrentState *state) {
  // Internal things. Uninteresting for user
  state->repo_obj                    = NULL;
  state->repo_path                   = NULL;
  state->head_ref                    = NULL;
  state->head_oid                    = NULL;
  state->status_list                 = NULL;


  // External stuff. User prolly interested in these
  state->cwd_full                    = "";
  state->cwd_basename                = "";

  state->repo_name                   = "";
  state->branch_name                 = "";

  state->is_git_repo                 = -1;
  state->has_upstream                = -1;
  state->conflict_num                = -1;
  state->is_rebase_in_progress       = -1;

  state->ahead_num                   = -1;
  state->behind_num                  = -1;
  state->staged_num                  = -1;
  state->modified_num                = -1;
  state->untracked_num               = -1;

  state->aws_token_is_valid          = -1;
  state->aws_token_remaining_hours   = -1;
  state->aws_token_remaining_minutes = -1;


  // get current working directory and basename
  static char cwd_path[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  state->cwd_full = cwd_path;

  static char wd[PATH_MAX];
  sprintf(wd, "%s", basename(cwd_path));
  state->cwd_basename = wd;
}


/**
 * Gather all git-related context.
 *
 * TODO: take return values into consideration
 */

int gather_git_context(struct CurrentState *state) {
  // in state, 0 means false, 1 means true.
  // when functions return 0, it's true, and 1 means false.
  // very confusing.
  state->is_git_repo = ! __is_git_repo(".");

  __populate_repo_context(state, ".");
  __get_repo_name(state);
  __get_branch_name(state);
  __get_repo_status(state);
  __get_repo_divergence(state);

  return ! state->is_git_repo;
}


/**
 * Check the validity of the AWS SSO login token and calculates the
 * remaining time until the token expires.
 *
 * Note that this function breaks convention by returning 1 if
 * successful(valid), to mirror the value stored in
 * state->aws_token_is_valid.
  */
int gather_aws_context(struct CurrentState *state) {
  const char *home_dir = getenv("HOME");
  if (!home_dir) return ERROR; // error

  char cache_dir[1024];
  snprintf(cache_dir, sizeof(cache_dir), "%s/.aws/sso/cache", home_dir);

  DIR *dir = opendir(cache_dir);
  if (!dir) { // cache dir doesn't exist.
    state->aws_token_is_valid = 0; // so invalid
    return 0;
  }

  struct dirent *entry;
  struct stat file_stat;
  time_t newest_mtime = 0;
  char newest_file[1024];

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) {
      char file_path[2048];
      snprintf(file_path, sizeof(file_path), "%s/%s", cache_dir, entry->d_name);

      if (stat(file_path, &file_stat) == 0) {
        if (file_stat.st_mtime > newest_mtime) {
          newest_mtime = file_stat.st_mtime;
          strncpy(newest_file, file_path, sizeof(newest_file));
        }
      }
    }
  }
  closedir(dir);

  if (newest_mtime == 0) { // no files exist in cachedir
    state->aws_token_is_valid = 0; // so invalid.
    return 0;
  }


  FILE *file = fopen(newest_file, "r");
  if (!file) return -1; // can't read - so error

  struct json_object *parsed_json, *expires_at;
  char buffer[4096];

  fread(buffer, sizeof(buffer), 1, file);
  fclose(file);

  parsed_json = json_tokener_parse(buffer);
  if (!parsed_json) return ERROR;

  if (!json_object_object_get_ex(parsed_json, "expiresAt", &expires_at)) {
    json_object_put(parsed_json);
    return ERROR; // error because invalid json
  }

  const char *expires_at_str = json_object_get_string(expires_at);
  struct tm tm;
  strptime(expires_at_str, "%Y-%m-%dT%H:%M:%SZ", &tm);
  time_t expires_at_time = timegm(&tm);

  json_object_put(parsed_json);

  time_t current_time;
  time(&current_time);

  double diff_seconds = difftime(expires_at_time, current_time);
  state->aws_token_is_valid = (diff_seconds > 0) ? 1 : 0;

  // Calculate hours and minutes
  if (state->aws_token_is_valid) {
    state->aws_token_remaining_hours = (int)(diff_seconds / 3600);
    state->aws_token_remaining_minutes = (int)((diff_seconds - (state->aws_token_remaining_hours * 3600)) / 60);
  } else {
    state->aws_token_remaining_hours = 0;
    state->aws_token_remaining_minutes = 0;
  }

  return state->aws_token_is_valid;
}

/**
 * Generate a path relative to the root of the Git repository, using
 * '+' to represent the root
 */
const char *get_cwd_from_gitrepo(struct CurrentState *state) {
  if (state->head_ref == NULL) return get_cwd_from_home(state);

  static char wd[PATH_MAX];
  size_t common_length = strspn(state->repo_path, state->cwd_full);
  if (common_length == strlen(state->cwd_full)) {
    sprintf(wd, "+/");
  }
  else {
    sprintf(wd, "+/%s", state->cwd_full + common_length + 1);
  }
  state->cwd_git_path = wd;
  return wd;
}

/**
 * Retrieve the current working directory path, replacing the home
 * directory part with '~' if applicable
 */
const char *get_cwd_from_home(struct CurrentState *state) {
  static char wd[PATH_MAX];
  char *home_path = getenv("HOME");

  if (strncmp(state->cwd_full, home_path, strlen(home_path)) == 0) {
    // Inside HOME directory
    if (strlen(state->cwd_full) == strlen(home_path)) {
      // At HOME root
      sprintf(wd, "~/");
    } else {
      // Deeper in HOME directory
      sprintf(wd, "~%s", state->cwd_full + strlen(home_path));
    }
  } else {
    // Outside HOME directory
    strcpy(wd, state->cwd_full);
  }

  state->cwd_git_path = wd;
  return wd;
}

/**
 * Memory management
 */
void cleanup_resources(struct CurrentState *state) {
  if (state->repo_obj) {
    git_repository_free(state->repo_obj);
    state->repo_obj = NULL;
  }
  if (state->repo_path) {
    free((void *) state->repo_path);
    state->repo_path = NULL;
  }
  if (state->head_ref) {
    git_reference_free(state->head_ref);
    state->head_ref = NULL;
  }

  // context-head_oid is handled internally by libgit2. Apparently.

  if (state->status_list) {
    git_status_list_free(state->status_list);
    state->status_list = NULL;
  }
}


/**
 * Shortens a filesystem path to a specified maximum width by
 * truncating the beginning of the string
 */
void shorten_path(char *original_path, int max_width) {

  // Sanity checks
  int original_path_len = strlen(original_path);
  if (original_path_len <= max_width) return;
  if (max_width < 3) {
    for (int i = 0; i < max_width; i++) original_path[i] = '.';
    original_path[3] = '\0';
    return;
  }

  // How much to shrink?
  int shrinkage = original_path_len - max_width;
  if (shrinkage <= 3) {
    shrinkage = 3; // for the ellipsis
  }
  else {
    shrinkage += 3;
  }

  // Shrink it
  char rebuild_path[original_path_len + 1];
  strcpy(rebuild_path, "...");
  strncat(rebuild_path,
          original_path + shrinkage,     // forward shrinkage chars
          original_path_len - shrinkage); // copy this many chars


  strcpy(original_path, rebuild_path);
}


/**
 * Checks if the escape sequences within a string are properly formed.
 */
int are_escape_sequences_properly_formed(const char *mystring) {
    int in_escape_sequence = 0;
    int in_bracket_sequence = 0;

    while (*mystring) {
        
        // first two conditionals to deal with opening and closing brackets

        if (*mystring == '\\' && *(mystring + 1) == '[') {
            // Found \[
            if (in_bracket_sequence) {
                fprintf(stderr, "Improperly formed - found a new opening bracket before closing the previous one\n");
                return ERROR;
            }
            in_bracket_sequence = 1;
            mystring++; // Skip the next character as it is part of the escape sequence
        } else if (*mystring == '\\' && *(mystring + 1) == ']') {
            // Found \]
            if (!in_bracket_sequence) {
                fprintf(stderr, "Improperly formed - found a closing bracket without a matching opening bracket\n");
                return ERROR;
            }
            in_bracket_sequence = 0;
            mystring++; // Skip the next character as it is part of the escape sequence
        }

        // Remaining conditionals to deal with escape sequences

        else if (*mystring == '\\' &&
                ((*(mystring + 1) == '0' && *(mystring + 2) == '3' && *(mystring + 3) == '3') ||
                 *(mystring + 1) == 'e')) {
            // Found \033 or \e, which should be within brackets
            if (!in_bracket_sequence) {
                fprintf(stderr, "Improperly formed - found an escape sequence outside of brackets\n");
                return ERROR;
            }
            in_escape_sequence = 1;
            mystring += (*mystring + 1 == 'e') ? 1 : 3; // Skip the next characters as they are part of the escape sequence
        } else if (*mystring == 'm' && in_escape_sequence) {
            // Found m - which should close an escape sequence
            if (!in_bracket_sequence) {
                fprintf(stderr, "Improperly formed - found closing of escape sequence outside of brackets\n");
                return ERROR;
            }
            in_escape_sequence = 0;
        }
        mystring++;
    }

    // If we're still in an escape sequence or in a bracket sequence at the end, it's improperly formed
    return in_escape_sequence == 0 && in_bracket_sequence == 0 ? SUCCESS : ERROR;
}


/**
 * Counts the number of visible (non-escape sequence) characters in a string.
 */
int count_visible_chars(const char *mystring) {
    int real_chars = 0;
    int in_bracket_sequence = 0;

    while (*mystring) {
        if (*mystring == '\\' && *(mystring + 1) == '[') {
            // Found the start of a bracketed sequence
            in_bracket_sequence = 1;
            mystring++; // Skip the '[' character
        } else if (in_bracket_sequence && *mystring == '\\' && *(mystring + 1) == ']') {
            // Found the end of a bracketed sequence
            in_bracket_sequence = 0;
            mystring++; // Skip the ']' character
        } else if (!in_bracket_sequence) {
            // Count the character if it's not within a bracketed sequence
            real_chars++;
        }
        mystring++;
    }

    return real_chars;
}


