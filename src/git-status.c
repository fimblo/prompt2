/*
  git-status.c

  Assortment of functions to get git status
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <git2.h>
#include <unistd.h>
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

  status->status_repo          = UP_TO_DATE;
  status->ahead                = 0;
  status->behind               = 0;

  status->status_staged        = UP_TO_DATE;
  status->staged_changes_num   = 0;

  status->status_unstaged      = UP_TO_DATE;
  status->unstaged_changes_num = 0;

  status->conflict_num         = 0;
  status->rebase_in_progress   = 0;
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


int populateRepoContext(struct RepoContext *context, const char *path) {
  const char *git_repository_path;
  git_repository *repo     = NULL;
  git_reference *head_ref  = NULL;
  const git_oid * head_oid = NULL;

  git_repository_path = __findGitRepositoryPath(path);
  if (strlen(git_repository_path) == 0) {
    free((void *) git_repository_path);
    return EXIT_DEFAULT_PROMPT;
  }

  if (git_repository_open(&repo, git_repository_path) != 0) {
    free((void *) git_repository_path);
    git_repository_free(repo);
    return EXIT_FAIL_REPO_OBJ;
  }

  if (git_repository_head(&head_ref, repo) != 0) {
    return EXIT_ABSENT_LOCAL_REF;
  }
  head_oid = git_reference_target(head_ref);

  context->repo_path = git_repository_path;  // "/path/to/projectName"
  context->repo_obj  = repo;
  context->head_ref  = head_ref;
  context->head_oid  = head_oid;
  return 1;
}

const char * getRepoName(struct RepoContext *context, struct RepoStatus *status) {
  status->repo_name = strrchr(context->repo_path, '/') + 1;
  return status->repo_name;
}

const char * getBranchName(struct RepoContext *context, struct RepoStatus *status) {
  status->branch_name = git_reference_shorthand(context->head_ref);
  return status->branch_name;
}

git_status_list * getRepoStatusList(struct RepoContext * context) {
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
    return NULL;
  }
  context->status_list = status_list;
  return status_list;
}

void getRepoStatus(git_status_list * status_list, struct RepoStatus *status) {
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

  status->staged_changes_num = staged_changes;
  status->unstaged_changes_num = unstaged_changes;
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


void getRepoDivergence(struct RepoContext *context,
                       struct RepoStatus *status) {
  char full_remote_branch_name[128];
  sprintf(full_remote_branch_name,
          "refs/remotes/origin/%s",
          git_reference_shorthand(context->head_ref));

    git_reference *upstream_ref = NULL;
    const git_oid *upstream_oid;
    const int retval = git_reference_lookup(&upstream_ref,
                                            context->repo_obj,
                                            full_remote_branch_name);
    if (retval != 0) {
      // If there is no upstream ref, this is a stand-alone branch
      status->status_repo = NO_UPSTREAM;
      git_reference_free(upstream_ref);
    }
    else {
      upstream_oid = git_reference_target(upstream_ref);

      // if the upstream_oid is null, we can't get the divergence, so
      // might as well set it to NO_DATA. Oh and btw, when there's no
      // conflict _and_ upstream_OID is NULL, then it seems we're
      // inside of an interactive rebase - when it's not useful to
      // check for divergences anyway.
      if (upstream_oid == NULL) {
        status->status_repo = NO_UPSTREAM;
      }
      else {
        __calculateDivergence(context->repo_obj,
                              context->head_oid,
                              upstream_oid,
                              &status->ahead,
                              &status->behind);
      }
    }

    // check if local and remote are the same
    if (status->status_repo == UP_TO_DATE) {
      if (git_oid_cmp(context->head_oid, upstream_oid) != 0)
        status->status_repo = MODIFIED;
    }

    git_reference_free(upstream_ref);
}

const char *getCWDFull(struct RepoStatus *status) {
  static char cwd_path[MAX_PATH_BUFFER_SIZE];
  getcwd(cwd_path, sizeof(cwd_path));
  status->cwd_path = cwd_path;
  return cwd_path;
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
