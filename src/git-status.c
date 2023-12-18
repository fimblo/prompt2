/*
  git-status.c

  Assortment of functions to get git status
*/
#include <git2.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "git-status.h"

const char *state_names[ENUM_SIZE] = {
    "NO_DATA",
    "NO_UPSTREAM",
    "UP_TO_DATE",
    "MODIFIED"
};


void initializeRepoContext(struct RepoContext *context) {
  context->repo_obj     = NULL;
  context->repo_path    = NULL;
  context->head_ref     = NULL;
  context->head_oid     = NULL;
  context->status_list  = NULL;
}

void initializeRepoStatus(struct RepoStatus *status) {
  status->repo_name    = NULL;
  status->branch_name  = NULL;

  status->status_repo          = NO_DATA;
  status->ahead                = -1;
  status->behind               = -1;

  status->status_staged        = NO_DATA;
  status->staged_changes_num   = -1;

  status->status_unstaged      = NO_DATA;
  status->unstaged_changes_num = -1;
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

const char * getRepoName(struct RepoContext *context, struct RepoStatus *status) {
  if (context->head_ref == NULL) return state_names[NO_DATA];
  status->repo_name = strrchr(context->repo_path, '/') + 1;
  return status->repo_name;
}

const char * getBranchName(struct RepoContext *context, struct RepoStatus *status) {
  if (context->head_ref == NULL) return state_names[NO_DATA];
  status->branch_name = git_reference_shorthand(context->head_ref);
  return status->branch_name;
}

// 0 if fail to get repo status
// 1 if success
int getRepoStatus(struct RepoContext *context, struct RepoStatus *status) {
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

  int status_count = git_status_list_entrycount(status_list);
  for (int i = 0; i < status_count; i++) {
    const git_status_entry *entry = git_status_byindex(status_list, i);
    if (entry->status == GIT_STATUS_CURRENT) continue;

    // Check for conflicts
    if (entry->status & GIT_STATUS_CONFLICTED)
      status->conflict_num++;

    // Check for staged changes
    if (entry->status & (GIT_STATUS_INDEX_NEW      |
                         GIT_STATUS_INDEX_MODIFIED |
                         GIT_STATUS_INDEX_RENAMED  |
                         GIT_STATUS_INDEX_DELETED  |
                         GIT_STATUS_INDEX_TYPECHANGE)) {
      status->status_staged = MODIFIED;
      staged_changes++;
    }

    // Check for unstaged changes
    if (entry->status & (GIT_STATUS_WT_MODIFIED |
                         GIT_STATUS_WT_DELETED  |
                         GIT_STATUS_WT_RENAMED  |
                         GIT_STATUS_WT_TYPECHANGE)) {
      status->status_unstaged = MODIFIED;
      unstaged_changes++;
    }
  }

  if (staged_changes == 0)   {
    status->status_staged   = UP_TO_DATE;
  }
  if (unstaged_changes == 0) {
    status->status_unstaged = UP_TO_DATE;
  }

  status->staged_changes_num = staged_changes;
  status->unstaged_changes_num = unstaged_changes;

  return 1;
}


int getRepoDivergence(struct RepoContext *context,
                       struct RepoStatus *status) {
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
    status->status_repo = NO_UPSTREAM;
    git_reference_free(upstream_ref);
    return 0;
  }

  upstream_oid = git_reference_target(upstream_ref);

  // if the upstream_oid is null, we assume it's a stand-alone branch.
  // Not certain about this. TODO: check
  if (upstream_oid == NULL) {
    status->status_repo = NO_UPSTREAM;
    return 0;
  }

  __calculateDivergence(context->repo_obj,
                        context->head_oid,
                        upstream_oid,
                        &status->ahead,
                        &status->behind);

  if (status->ahead + status->behind == 0) {
    status->status_repo = UP_TO_DATE;
  }
  else {
    status->status_repo = MODIFIED;
  }

  /* if (git_oid_cmp(context->head_oid, upstream_oid) != 0) */
  /*   status->status_repo = MODIFIED; */

  git_reference_free(upstream_ref);
  return 1;
}

const char *getCWDFull(struct RepoStatus *status) {
  static char cwd_path[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  status->cwd_full = cwd_path;
  return cwd_path;
}

const char *getCWDBasename(struct RepoStatus *status) {
  static char cwd_path[PATH_MAX];
  static char wd[PATH_MAX];
  getcwd(cwd_path, sizeof(cwd_path));
  sprintf(wd, "%s", basename(cwd_path));
  status->cwd_basename = wd;
  return wd;
}

const char *getCWDFromGitRepo(struct RepoContext *context, struct RepoStatus *status) {
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
  status->cwd_git_path = wd;
  return wd;
}

const char *getCWDFromHome(struct RepoStatus *status) {
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

  status->cwd_git_path = wd;
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
