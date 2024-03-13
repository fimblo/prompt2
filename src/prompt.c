#include <git2.h>
#include <stdio.h>
#include <uthash.h>

#include "get-status.h"

#define COMMAND_MAX_LEN  256

typedef struct {
  const char *command;      // key
  const char *replacement;  // value
  UT_hash_handle hh;        // makes this structure hashable
} instruction_t;
instruction_t *instructions = NULL;

// Function to add entries to the hash table
void add_instruction(const char *command, const char *replacement) {
  instruction_t *i = malloc(sizeof(instruction_t));
  if (i == NULL) {
    // handle malloc failure
    exit(-1);
  }
  i->command = command;
  i->replacement = strdup(replacement);
  if (i->replacement == NULL) {
    free(i);
    exit(-1);
  }
  HASH_ADD_KEYPTR(hh, instructions, i->command, strlen(i->command), i);
}

// Function to find an entry in the hash table
const char *find_replacement(const char *command) {
  instruction_t *i;
  HASH_FIND_STR(instructions, command, i);
  return i ? i->replacement : NULL;
}

void addDefaultInstructions(struct CurrentState *state) {
  const char* cwd_from_gitrepo = getCWDFromGitRepo(state);
  const char* cwd_from_home = getCWDFromHome(state);

  char buf[32];

  add_instruction("CWD.full",                     state->cwd_full);
  add_instruction("CWD.basename",                 state->cwd_basename);
  add_instruction("CWD.git_path",                 cwd_from_gitrepo);
  add_instruction("CWD.home_path",                cwd_from_home);

  
  snprintf(buf, sizeof(buf), "%d",                state->is_git_repo);
  add_instruction("Repo.is_git_repo", buf);

  add_instruction("Repo.name",                    state->repo_name);
  add_instruction("Repo.branch_name",             state->branch_name);

  snprintf(buf, sizeof(buf), "%d",                state->is_rebase_in_progress);
  add_instruction("Repo.rebase_active", buf);

  snprintf(buf, sizeof(buf), "%d",                state->conflict_num);
  add_instruction("Repo.conflicts", buf);

  snprintf(buf, sizeof(buf), "%d",                state->has_upstream);
  add_instruction("Repo.has_upstream", buf);
  snprintf(buf, sizeof(buf), "%d",                state->ahead_num);
  add_instruction("Repo.ahead", buf);
  snprintf(buf, sizeof(buf), "%d",                state->behind_num);
  add_instruction("Repo.behind", buf);

  snprintf(buf, sizeof(buf), "%d",                state->staged_num);
  add_instruction("Repo.staged", buf);
  snprintf(buf, sizeof(buf), "%d",                state->modified_num);
  add_instruction("Repo.modified", buf);
  snprintf(buf, sizeof(buf), "%d",                state->untracked_num);
  add_instruction("Repo.untracked", buf);

  snprintf(buf, sizeof(buf), "%d",                state->aws_token_is_valid);
  add_instruction("AWS.token_is_valid", buf);
  snprintf(buf, sizeof(buf), "%d",                state->aws_token_remaining_hours);
  add_instruction("AWS.token_remaining_hours", buf);
  snprintf(buf, sizeof(buf), "%d",                state->aws_token_remaining_minutes);
  add_instruction("AWS.token_remaining_minutes", buf);
}


const char *parsePrompt(const char *undigestedPrompt) {
  char digestedPrompt[PROMPT_MAX_LEN] = "";
  const char *ptr = undigestedPrompt;
  char command[COMMAND_MAX_LEN];
  int commandIndex = 0;
  int inCommand = 0;

  while (*ptr) {
    if (*ptr == '@' && *(ptr + 1) == '{') {
      inCommand = 1;
      commandIndex = 0;
      ptr += 2; // Skip past the '@{'
    } else if (*ptr == '}' && inCommand) {
      inCommand = 0;
      command[commandIndex] = '\0'; // Null-terminate the command string
  
      // Look up the command and append its value to digestedPrompt
      const char *replacement = find_replacement(command);
      if (replacement) {
        strcat(digestedPrompt, replacement);
      } else {
        // Command not found, append the original command
        strcat(digestedPrompt, "@{");
        strcat(digestedPrompt, command);
        strcat(digestedPrompt, "}");
      }
      ptr++; // Move past the '}'
    } else if (inCommand) {
      // We are inside a command, accumulate characters
      command[commandIndex++] = *ptr++;
    } else {
      // We are outside a command, copy character directly to digestedPrompt
      char str[2] = {*ptr++, '\0'};
      strcat(digestedPrompt, str);
    }
  }

  return strdup(digestedPrompt);
}


int main(void) {
  struct CurrentState state;

  git_libgit2_init();
  initialiseState(&state);

  if (gatherGitContext(&state) != 0) {
    const char *nonGitPrompt = getenv("GP2_NON_GIT_PROMPT") ?: "\\W$ ";
    printf("%s", nonGitPrompt);
    return 0;
  }

  gatherAWSContext(&state);
  addDefaultInstructions(&state);


  const char *undigestedPrompt = getenv("GP2_GIT_PROMPT") ?: "<@{Repo.name}> @{CWD.home_path}\n$ ";
  const char *digestedPrompt = parsePrompt(undigestedPrompt);
  printf("%s", digestedPrompt);

  
  cleanupResources(&state);
  git_libgit2_shutdown();
  instruction_t *current_entry, *tmp;
    HASH_ITER(hh, instructions, current_entry, tmp) {
    HASH_DEL(instructions, current_entry);
    free((char*)current_entry->replacement);
    free(current_entry);
  }

  return 0;
}

