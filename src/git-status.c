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

#include "git-status.h"

const char *state_names[ENUM_SIZE] = {
  "NO_DATA",
  "NO_UPSTREAM",
  "UP_TO_DATE",
  "MODIFIED"
};


void setDefaultValues(struct RepoContext *context, struct CurrentState *state) {
  // Internal things. Uninteresting for user
  context->repo_obj                  = NULL;
  context->repo_path                 = NULL;
  context->head_ref                  = NULL;
  context->head_oid                  = NULL;
  context->status_list               = NULL;


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

  state->aws_token_is_valid          = 0;
  state->aws_token_remaining_hours   = -1;
  state->aws_token_remaining_minutes = -1;
}


// helper function. not exported
const char *__findGitRepositoryPath(const char *path) {
  git_buf repo_path = { 0 };
  int error = git_repository_discover(&repo_path, path, 0, NULL);

  if (error == 0) {
    char *last_slash = strstr(repo_path.ptr, "/.git/");
    if (last_slash) {
      *last_slash = '\0';  // Null-terminate the string at the last slash
    }
    char *result = strdup(repo_path.ptr);  // Duplicate the path before freeing the buffer
    git_buf_free(&repo_path);
    return result;
  }

  // Check if we've reached the file system root
  if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0) {
    return strdup("");
  }

  // Move to the parent directory
  char *last_slash = strrchr(path, '/');
  if (last_slash) {
    char parent_path[last_slash - path + 1];
    strncpy(parent_path, path, last_slash - path);
    parent_path[last_slash - path] = '\0';

    return __findGitRepositoryPath(parent_path);
  }
  else {
    return strdup("");
  }
}

// helper function. not exported
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




// return 0 if unable to open git repo
// return 1 if git repo was opened
int populateRepoContext(struct RepoContext *context, const char *path) {
  const char *git_repository_path;
  git_repository *repo     = NULL;
  git_reference *head_ref  = NULL;
  const git_oid * head_oid = NULL;

  git_repository_path = __findGitRepositoryPath(path);
  if (strlen(git_repository_path) == 0) {
    free((void *) git_repository_path);
    return 0;
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

  context->repo_path = git_repository_path;  // "/path/to/projectName"
  context->repo_obj  = repo;
  context->head_ref  = head_ref;
  context->head_oid  = head_oid;
  return 1;
}

const char * getRepoName(struct RepoContext *context, struct CurrentState *state) {
  if (context->head_ref == NULL) return state_names[NO_DATA];
  state->repo_name = strrchr(context->repo_path, '/') + 1;
  return state->repo_name;
}

const char * getBranchName(struct RepoContext *context, struct CurrentState *state) {
  if (context->head_ref == NULL) return state_names[NO_DATA];
  state->branch_name = git_reference_shorthand(context->head_ref);
  return state->branch_name;
}

// 0 if fail to get repo status
// 1 if success
int getRepoStatus(struct RepoContext *context, struct CurrentState *state) {
  if (context->head_ref == NULL) return 0;

  // First get the status-list which we'll iterate through
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
#pragma GCC diagnostic pop

  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, context->repo_obj, &opts) != 0) {
    git_reference_free(context->head_ref);
    git_repository_free(context->repo_obj);
    free((void *) context->repo_path);
    return 0;
  }
  context->status_list = status_list;


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

  return 1;
}

// return 0 if no upstream
// and 1 if successful
int getRepoDivergence(struct RepoContext *context,
                       struct CurrentState *state) {
  if (context->head_ref == NULL) return 0;

  char full_remote_branch_name[128];
  sprintf(full_remote_branch_name, "refs/remotes/origin/%s", git_reference_shorthand(context->head_ref));

  git_reference *upstream_ref = NULL;
  const git_oid *upstream_oid;
  const int retval =
    git_reference_lookup(&upstream_ref,
                         context->repo_obj,
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

  __calculateDivergence(context->repo_obj,
                        context->head_oid,
                        upstream_oid,
                        &state->ahead,
                        &state->behind);

  if (state->ahead + state->behind == 0) {
    state->status_repo = UP_TO_DATE;
  }
  else {
    state->status_repo = MODIFIED;
  }

  /* if (git_oid_cmp(context->head_oid, upstream_oid) != 0) */
  /*   state->status_repo = MODIFIED; */

  git_reference_free(upstream_ref);
  return 1;
}


// 1 if we are in ann interactive rebase
// 0 otherwise
int checkForInteractiveRebase(struct RepoContext *context, struct CurrentState *state) {
  char rebaseMergePath[PATH_MAX];
  char rebaseApplyPath[PATH_MAX];
  snprintf(rebaseMergePath, sizeof(rebaseMergePath), "%s/.git/rebase-merge", context->repo_path);
  snprintf(rebaseApplyPath, sizeof(rebaseApplyPath), "%s/.git/rebase-apply", context->repo_path);

  struct stat mergeStat, applyStat;
  if (stat(rebaseMergePath, &mergeStat) == 0 || stat(rebaseApplyPath, &applyStat) == 0) {
    state->rebase_in_progress = 1;
    return 1;
  }
  return 0;
}

// return -1 if unable to get token expiry time
// 0 if token exists and is invalid
// 1 if token exists and is valid
int getAWSContext(struct CurrentState *state) {
  const char *home_dir = getenv("HOME");
  if (!home_dir) return 0;

  char cache_dir[1024];
  snprintf(cache_dir, sizeof(cache_dir), "%s/.aws/sso/cache", home_dir);

  DIR *dir = opendir(cache_dir);
  if (!dir) return 0;

  struct dirent *entry;
  struct stat file_stat;
  time_t newest_mtime = 0;
  char newest_file[1024];

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) {
      char file_path[1024];
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

  if (newest_mtime == 0) return -1;

  FILE *file = fopen(newest_file, "r");
  if (!file) return -1;

  struct json_object *parsed_json, *expires_at;
  char buffer[4096];

  fread(buffer, sizeof(buffer), 1, file);
  fclose(file);

  parsed_json = json_tokener_parse(buffer);
  if (!parsed_json) return -1;

  if (!json_object_object_get_ex(parsed_json, "expiresAt", &expires_at)) {
    json_object_put(parsed_json);
    return -1;
  }

  const char *expires_at_str = json_object_get_string(expires_at);
  struct tm tm;
  strptime(expires_at_str, "%Y-%m-%dT%H:%M:%SZ", &tm);
  time_t expires_at_time = mktime(&tm);

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


const char *getCWDFull(struct CurrentState *state) {
  static char cwd_path[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  state->cwd_full = cwd_path;
  return cwd_path;
}

const char *getCWDBasename(struct CurrentState *state) {
  static char cwd_path[PATH_MAX];
  static char wd[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  sprintf(wd, "%s", basename(cwd_path));
  state->cwd_basename = wd;
  return wd;
}


const char *getCWDFromGitRepo(struct RepoContext *context, struct CurrentState *state) {
  if (context->head_ref == NULL) return strdup("NO_DATA");

  static char cwd_path[PATH_MAX];
  static char wd[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  size_t common_length = strspn(context->repo_path, cwd_path);
  if (common_length == strlen(cwd_path)) {
    sprintf(wd, "+/");
  }
  else {
    sprintf(wd, "+/%s", cwd_path + common_length + 1);
  }
  state->cwd_git_path = wd;
  return wd;
}

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

void cleanupResources(struct RepoContext *context) {
  if (context->repo_obj) {
    git_repository_free(context->repo_obj);
    context->repo_obj = NULL;
  }
  if (context->repo_path) {
    free((void *) context->repo_path);
    context->repo_path = NULL;
  }
  if (context->head_ref) {
    git_reference_free(context->head_ref);
    context->head_ref = NULL;
  }

  // context-head_oid is handled internally by libgit2. Apparently.

  if (context->status_list) {
    git_status_list_free(context->status_list);
    context->status_list = NULL;
  }
}

