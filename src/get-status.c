/*
  git-status.c

  Assortment of functions to get git status
*/
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

const char *state_names[ENUM_SIZE] = {
  "NO_DATA",
  "NO_UPSTREAM",
  "UP_TO_DATE",
  "MODIFIED"
};


/* ================================================== */
/* Helper functions                                   */
/* ================================================== */


/**
 * Helper: Figure out divergence between local and upstream branches.
 */
int __calculateDivergence(git_repository *repo,
                          const git_oid *local_oid,
                          const git_oid *upstream_oid,
                          int *ahead,
                          int *behind) {
  int aheadCount = 0;
  int behindCount = 0;
  git_oid id;

  // init walker
  git_revwalk *walker = NULL;
  if (git_revwalk_new(&walker, repo) != 0) {
    return -1;
  }

  // count number of commits ahead
  if (git_revwalk_push(walker, local_oid)    != 0 ||  // set where I want to start
      git_revwalk_hide(walker, upstream_oid) != 0) {  // set where the walk ends (exclusive)
    git_revwalk_free(walker);
    return -2;
  }
  while (git_revwalk_next(&id, walker) == 0) aheadCount++;

  // count number of commits behind
  git_revwalk_reset(walker);
  if (git_revwalk_push(walker, upstream_oid) != 0 || // set where I want to start
      git_revwalk_hide(walker, local_oid)    != 0) { // set where the walk ends (exclusive)
    git_revwalk_free(walker);
    return -3;
  }
  while (git_revwalk_next(&id, walker) == 0) behindCount++;

  *ahead = aheadCount;
  *behind = behindCount;

  git_revwalk_free(walker);
  return 0;
}

/**
 * Helper: Determine if a Git repository is currently in an
 * interactive rebase state
 */
int __checkForInteractiveRebase(struct CurrentState *state) {
  char rebaseMergePath[PATH_MAX];
  char rebaseApplyPath[PATH_MAX];
  snprintf(rebaseMergePath, sizeof(rebaseMergePath), "%s/.git/rebase-merge", state->repo_path);
  snprintf(rebaseApplyPath, sizeof(rebaseApplyPath), "%s/.git/rebase-apply", state->repo_path);

  struct stat mergeStat, applyStat;
  if (stat(rebaseMergePath, &mergeStat) == 0 || stat(rebaseApplyPath, &applyStat) == 0) {
    state->rebase_in_progress = 1;
    return 1;
  }
  return 0;
}

/**
 * Helper: set RepoContext with repo name and return it
 */
const char * __getRepoName(struct CurrentState *state) {
  if (state->head_ref == NULL) return state_names[NO_DATA];
  state->repo_name = strrchr(state->repo_path, '/') + 1;
  return state->repo_name;
}

/**
 * Helper: set RepoContext with repo branch and return it
 */
const char * __getBranchName(struct CurrentState *state) {
  if (state->head_ref == NULL) return state_names[NO_DATA];
  state->branch_name = git_reference_shorthand(state->head_ref);
  return state->branch_name;
}

/**
 * Helper: Prep RepoContext with git repo info
 */
int __populateRepoContext(struct CurrentState *state, const char *path) {
  const char *git_repository_path;
  git_repository *repo     = NULL;
  git_reference *head_ref  = NULL;
  const git_oid * head_oid = NULL;

  if (state->repo_path == NULL) {
    git_repository_path = findGitRepositoryPath(path);

    if (strlen(git_repository_path) == 0) {
      free((void *) git_repository_path);
      return 0;
    }
  }
  else {
    git_repository_path = state->repo_path;
  }


  if (git_repository_open(&repo, git_repository_path) != 0) {
    free((void *) git_repository_path);
    git_repository_free(repo);
    return 0;
  }

  if (git_repository_head(&head_ref, repo) != 0) {
    return 0;
  }
  head_oid = git_reference_target(head_ref);

  state->repo_path = git_repository_path;  // "/path/to/projectName"
  state->repo_obj  = repo;
  state->head_ref  = head_ref;
  state->head_oid  = head_oid;
  
  return 1;
}

/**
 * Helper: Get the current Git repository's status, including staged
 * and unstaged changes, and conflicts.
 */
int __getRepoStatus(struct CurrentState *state) {
  if (state->head_ref == NULL) return 0;

  // First get the status-list which we'll iterate through
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
#pragma GCC diagnostic pop

  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, state->repo_obj, &opts) != 0) {
    git_reference_free(state->head_ref);
    git_repository_free(state->repo_obj);
    free((void *) state->repo_path);
    return 0;
  }
  state->status_list = status_list;


  // Now iterate
  int staged_changes   = 0;
  int unstaged_changes = 0;
  int conflicts        = 0;

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
      state->status_staged = MODIFIED;
      staged_changes++;
    }

    // Check for unstaged changes
    if (entry->status & (GIT_STATUS_WT_MODIFIED |
                         GIT_STATUS_WT_DELETED  |
                         GIT_STATUS_WT_RENAMED  |
                         GIT_STATUS_WT_TYPECHANGE)) {
      state->status_unstaged = MODIFIED;
      unstaged_changes++;
    }
  }

  if (staged_changes == 0)   {
    state->status_staged   = UP_TO_DATE;
  }
  if (unstaged_changes == 0) {
    state->status_unstaged = UP_TO_DATE;
  }

  state->staged_changes_num = staged_changes;
  state->unstaged_changes_num = unstaged_changes;
  state->conflict_num = conflicts;


  __checkForInteractiveRebase(state);
  return 1;
}

/**
 * Helper: Calculate the divergence of the current Git repository from
 * its upstream branch, updating the state with information on how
 * many commits it is ahead or behind.
 */
int __getRepoDivergence(struct CurrentState *state) {
  if (state->head_ref == NULL) return 0;

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
    state->status_repo = NO_UPSTREAM;
    git_reference_free(upstream_ref);
    return 0;
  }

  upstream_oid = git_reference_target(upstream_ref);

  // if the upstream_oid is null, we assume it's a stand-alone branch.
  // Not certain about this. TODO: check
  if (upstream_oid == NULL) {
    state->status_repo = NO_UPSTREAM;
    return 0;
  }

  __calculateDivergence(state->repo_obj,
                        state->head_oid,
                        upstream_oid,
                        &state->ahead,
                        &state->behind);

  if (state->ahead + state->behind == 0) {
    state->status_repo = UP_TO_DATE;
  }
  else {
    state->status_repo = MODIFIED;
  }

  /* if (git_oid_cmp(state->head_oid, upstream_oid) != 0) */
  /*   state->status_repo = MODIFIED; */

  git_reference_free(upstream_ref);
  return 1;
}




/* ================================================== */
/* Exported functions                                 */
/* ================================================== */

/**
 * Sets up CurrentState so that they are useable.
 */
void initialiseState(struct CurrentState *state) {
  // Internal things. Uninteresting for user
  state->repo_obj                  = NULL;
  state->repo_path                 = NULL;
  state->head_ref                  = NULL;
  state->head_oid                  = NULL;
  state->status_list               = NULL;


  // External stuff. User prolly interested in these
  state->repo_name                   = NULL;
  state->branch_name                 = NULL;

  state->status_repo                 = NO_DATA;
  state->ahead                       = -1;
  state->behind                      = -1;

  state->status_staged               = NO_DATA;
  state->staged_changes_num          = -1;

  state->status_unstaged             = NO_DATA;
  state->unstaged_changes_num        = -1;

  state->conflict_num                = -1;
  state->rebase_in_progress          = 0;

  state->aws_token_is_valid          = -1;
  state->aws_token_remaining_hours   = -1;
  state->aws_token_remaining_minutes = -1;
}

/**
 * Given a path, returns root of git repo or empty string
 */
const char *findGitRepositoryPath(const char *path) {
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
 * Gather all git-related context.
 */

int gatherGitContext(struct CurrentState *state) {
  __populateRepoContext(state, ".");
  __getRepoName(state);
  __getBranchName(state);
  __getRepoStatus(state);
  __getRepoDivergence(state);

  return 1;
}


/**
 * Check the validity of the AWS SSO login token and calculates the
 * remaining time until the token expires
 */
int gatherAWSContext(struct CurrentState *state) {
  const char *home_dir = getenv("HOME");
  if (!home_dir) return -1; // error

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
  if (!parsed_json) return -1;

  if (!json_object_object_get_ex(parsed_json, "expiresAt", &expires_at)) {
    json_object_put(parsed_json);
    return -1; // error because invalid json
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
 * Retrieve the full path of the current working directory
 */
const char *getCWDFull(struct CurrentState *state) {
  static char cwd_path[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  state->cwd_full = cwd_path;
  return cwd_path;
}

/**
 * Obtain the basename (the last component) of the current working directory
 */
const char *getCWDBasename(struct CurrentState *state) {
  static char cwd_path[PATH_MAX];
  static char wd[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  sprintf(wd, "%s", basename(cwd_path));
  state->cwd_basename = wd;
  return wd;
}

/**
 * Generate a path relative to the root of the Git repository, using
 * '+' to represent the root
 */
const char *getCWDFromGitRepo(struct CurrentState *state) {
  if (state->head_ref == NULL) return strdup("NO_DATA");

  static char cwd_path[PATH_MAX];
  static char wd[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  size_t common_length = strspn(state->repo_path, cwd_path);
  if (common_length == strlen(cwd_path)) {
    sprintf(wd, "+/");
  }
  else {
    sprintf(wd, "+/%s", cwd_path + common_length + 1);
  }
  state->cwd_git_path = wd;
  return wd;
}

/**
 * Retrieve the current working directory path, replacing the home
 * directory part with '~' if applicable
 */
const char *getCWDFromHome(struct CurrentState *state) {
  static char cwd_path[PATH_MAX];
  static char wd[PATH_MAX];
  char *home_path = getenv("HOME");

  getcwd(cwd_path, sizeof(cwd_path));
  if (strncmp(cwd_path, home_path, strlen(home_path)) == 0) {
    // Inside HOME directory
    if (strlen(cwd_path) == strlen(home_path)) {
      // At HOME root
      sprintf(wd, "~/");
    } else {
      // Deeper in HOME directory
      sprintf(wd, "~%s", cwd_path + strlen(home_path));
    }
  } else {
    // Outside HOME directory
    strcpy(wd, cwd_path);
  }

  state->cwd_git_path = wd;
  return wd;
}

/**
 * Memory management
 */
void cleanupResources(struct CurrentState *state) {
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
void pathTruncateSimple(char *originalPath, int maxWidth) {

  // Sanity checks
  int originalPathLen = strlen(originalPath);
  if (originalPathLen <= maxWidth) return;
  if (maxWidth < 3) {
    for (int i = 0; i < maxWidth; i++) originalPath[i] = '.';
    originalPath[3] = '\0';
    return;
  }

  // How much to shrink?
  int shrinkage = originalPathLen - maxWidth;
  if (shrinkage <= 3) {
    shrinkage = 3; // for the ellipsis
  }
  else {
    shrinkage += 3;
  }

  // Shrink it
  char rebuildPath[originalPathLen + 1];
  strcpy(rebuildPath, "...");
  strncat(rebuildPath,
          originalPath + shrinkage,     // forward shrinkage chars
          originalPathLen - shrinkage); // copy this many chars


  strcpy(originalPath, rebuildPath);
}

/**
 * Shortens a filesystem path to a specified maximum width by
 * abbreviating intermediate directories while keeping the last
 * directory in full.
 */
void pathTruncateAccordion(char *originalPath, int maxWidth) {

  // Sanity checks
  int originalPathLen = strlen(originalPath);
  if (originalPathLen <= maxWidth) return;
  if (maxWidth < 3) {
    for (int i = 0; i < maxWidth; i++) originalPath[i] = '.';
    originalPath[3] = '\0';
    return;
  }


  // tmp vars as my workbench, these will be messed with
  char tmpPath[originalPathLen];
  int  tmpPathLength = originalPathLen;

  // As I step through the directories, I place dirs (short or long)
  // here after I decide what to do.
  char rebuildPath[originalPathLen];
  memset(rebuildPath, '\0', originalPathLen);

  // store the first char if it's a special char.
  if (originalPath[0] == '~' || originalPath[0] == '+') {
    rebuildPath[0] = originalPath[0];
    strcpy(tmpPath, originalPath + 1);
  } else {
    strcpy(tmpPath, originalPath);
  }

  // use tokenizer to walk through the dir levels
  char  lastToken[originalPathLen];  // for the dir the user is standing in, we want to preserve that
  int   shortDirWasUsed;
  char *token = strtok(tmpPath, "/");
  while (token != NULL) {
    int shortDirLength = 3; // +1 for slash, +1 for terminator
    char shortDir[shortDirLength];
    sprintf(shortDir, "/%c", token[0]);

    int longDirLength = strlen(token) + 2; // +1 for slash, +1 for terminator ill be back
    char longDir[longDirLength];
    snprintf(longDir, (size_t) longDirLength, "/%s", token);

    int shinkage = longDirLength - shortDirLength;
    if (tmpPathLength >= maxWidth) {
      tmpPathLength = tmpPathLength - shinkage;
      strcat(rebuildPath, shortDir);
      shortDirWasUsed = 1;
    }
    else {
      strcat(rebuildPath, longDir);
      shortDirWasUsed = 0;
    }

    strcpy(lastToken, token);
    token = strtok(NULL, "/");
  }

  /*
    If the last directory added to rebuildPath was a shortDir, replace
    it with the full directory name, since I want that behaviour
  */
  if (shortDirWasUsed) {
    char *lastSlash = strrchr(rebuildPath, '/');
    if (lastSlash != NULL) {
      strcpy(lastSlash + 1, lastToken);
    }
  }


  strcpy(originalPath, rebuildPath);
}
