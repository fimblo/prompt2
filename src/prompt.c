#include <git2.h>
#include "get-status.h"
#include "uthash.h"
#include "stdio.h"


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
  i->replacement = replacement;
  HASH_ADD_KEYPTR(hh, instructions, i->command, strlen(i->command), i);
}

// Function to find an entry in the hash table
const char *find_replacement(const char *command) {
  instruction_t *i;
  HASH_FIND_STR(instructions, command, i);
  return i ? i->replacement : NULL;
}


int main(void) {
  git_libgit2_init();
  struct CurrentState state;
  initialiseState(&state);
  gatherGitContext(&state);

  state.repo_path = findGitRepositoryPath(".");
  if (strlen(state.repo_path) == 0) {
    free((void *) state.repo_path);
    printf("$ ");
    return 0;
  }

  gatherAWSContext(&state);
  const char *undigestedPrompt = getenv("GP2_PROMPT") ?: "<@{Repo.name}> @{CWD.home_path}\n$ \n";

  const char* cwd_from_gitrepo = getCWDFromGitRepo(&state);
  const char* cwd_from_home = getCWDFromHome(&state);
  add_instruction("CWD.full",                     state.cwd_full);
  add_instruction("CWD.basename",                 state.cwd_basename);
  add_instruction("CWD.git_path",                 cwd_from_gitrepo);
  add_instruction("CWD.home_path",                cwd_from_home);

  add_instruction("Repo.is_git_repo",             state.is_git_repo);
  add_instruction("Repo.name",                    state.repo_name);
  add_instruction("Repo.branch.name",             state.branch_name);
  add_instruction("Repo.rebase_active",           state.rebase_in_progress);
  add_instruction("Repo.conflict.num",            state.conflict_num);

  add_instruction("Diverge.status",               state_names[state.status_repo]);
  add_instruction("Diverge.ahead",                state.ahead);
  add_instruction("Diverge.behind",               state.behind);

  add_instruction("Staged.status",                state_names[state.status_staged]);
  add_instruction("Staged.num",                   state.staged_changes_num);
  add_instruction("Unstaged.status",              state_names[state.status_unstaged]);
  add_instruction("Unstaged.num",                 state.unstaged_changes_num);

  add_instruction("AWS.token_is_valid",           state.aws_token_is_valid);
  add_instruction("AWS.token_remaining_hours",    state.aws_token_remaining_hours);
  add_instruction("AWS.token_remaining_minutes",  state.aws_token_remaining_minutes);




  char digestedPrompt[1024] = {0};
  const char *ptr = undigestedPrompt;
  char command[256];
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
  
  // Output the digested prompt
  printf("%s", digestedPrompt);

  
  cleanupResources(&state);
  git_libgit2_shutdown();
  instruction_t *current_entry, *tmp;
    HASH_ITER(hh, instructions, current_entry, tmp) {
    HASH_DEL(instructions, current_entry);
    free(current_entry);
  }


  return 0;
}

