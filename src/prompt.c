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

void add_default_instructions(struct CurrentState *state) {
  const char* cwd_from_gitrepo = get_cwd_from_gitrepo(state);
  const char* cwd_from_home = get_cwd_from_home(state);

  char itoa_buf[32]; // to store numbers as strings

  add_instruction("CWD.full",                     state->cwd_full);
  add_instruction("CWD.basename",                 state->cwd_basename);
  add_instruction("CWD.git_path",                 cwd_from_gitrepo);
  add_instruction("CWD.home_path",                cwd_from_home);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_git_repo);
  add_instruction("Repo.is_git_repo", itoa_buf);

  add_instruction("Repo.name",                    state->repo_name);
  add_instruction("Repo.branch_name",             state->branch_name);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_rebase_in_progress);
  add_instruction("Repo.rebase_active", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->conflict_num);
  add_instruction("Repo.conflicts", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->has_upstream);
  add_instruction("Repo.has_upstream", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->ahead_num);
  add_instruction("Repo.ahead", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->behind_num);
  add_instruction("Repo.behind", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->staged_num);
  add_instruction("Repo.staged", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->modified_num);
  add_instruction("Repo.modified", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->untracked_num);
  add_instruction("Repo.untracked", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->aws_token_is_valid);
  add_instruction("AWS.token_is_valid", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",                state->aws_token_remaining_hours);
  add_instruction("AWS.token_remaining_hours", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",                state->aws_token_remaining_minutes);
  add_instruction("AWS.token_remaining_minutes", itoa_buf);
}


/**
 * Helper function.
 * Concatenates a string to digested_prompt if the resulting length is within bounds.
 * 
  */
int my_strcat(char *digested_prompt, const char *addition) {
    if (strlen(digested_prompt) + strlen(addition) >= PROMPT_MAX_LEN) {
        return FAILURE; // The resulting string would be too long
    }
    strcat(digested_prompt, addition); // Safe to concatenate
    return SUCCESS;
}

const char *parse_prompt(const char *undigested_prompt) {
  char digested_prompt[PROMPT_MAX_LEN] = "";
  const char *ptr = undigested_prompt;
  char command[COMMAND_MAX_LEN];
  int command_index = 0;
  int in_command = 0;

  while (*ptr) {
    if (*ptr == '@' && *(ptr + 1) == '{') {
      in_command = 1;
      command_index = 0;
      ptr += 2; // Skip past the '@{'
    } else if (*ptr == '}' && in_command) {
      in_command = 0;
      command[command_index] = '\0'; // Null-terminate the command string
  
      // Look up the command and append its value to digested_prompt
      const char *replacement = find_replacement(command);
      if (replacement) {
        if(my_strcat(digested_prompt, replacement) == FAILURE) { goto error; }
      } else {
        // Command not found, append the original command
        if(my_strcat(digested_prompt, "@{") == FAILURE) { goto error; }
        if(my_strcat(digested_prompt, command) == FAILURE) { goto error; }
        if(my_strcat(digested_prompt, "}") == FAILURE) { goto error; }
      }
      ptr++; // Move past the '}'
    } else if (in_command) {
      // We are inside a command, accumulate characters
      command[command_index++] = *ptr++;
    } else {
      // We are outside a command, copy character directly to digested_prompt
      char str[2] = {*ptr++, '\0'};
      if(my_strcat(digested_prompt, str) == FAILURE) { goto error; }
    }
  }

  return strdup(digested_prompt);

  error:
    return "PROMPT TOO LONG $ ";  
}


int main(void) {
  struct CurrentState state;

  git_libgit2_init();
  initialise_state(&state);

  if (gather_git_context(&state) != 0) {
    const char *nonGitPrompt = getenv("GP2_NON_GIT_PROMPT") ?: "\\W$ ";
    printf("%s", nonGitPrompt);
    return 0;
  }

  gather_aws_context(&state);
  add_default_instructions(&state);


  const char *undigested_prompt = getenv("GP2_GIT_PROMPT") ?: "<@{Repo.name}> @{CWD.home_path}\n$ ";
  const char *digested_prompt = parse_prompt(undigested_prompt);
  printf("%s", digested_prompt);

  
  cleanup_resources(&state);
  git_libgit2_shutdown();
  instruction_t *current_entry, *tmp;
    HASH_ITER(hh, instructions, current_entry, tmp) {
    HASH_DEL(instructions, current_entry);
    free((char*)current_entry->replacement);
    free(current_entry);
  }

  return 0;
}

