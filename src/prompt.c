#include <git2.h>
#include <stdio.h>
#include <uthash.h>

#include "get-status.h"

#define COMMAND_MAX_LEN  256


// todo: check file for camelCase
  

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

  char itoa_buf[32]; // to store numbers as strings

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
 * Concatenates a string to git_prompt if the resulting length is within bounds.
 *
  */
int my_strcat(char *git_prompt, const char *addition) {
    if (strlen(git_prompt) + strlen(addition) >= PROMPT_MAX_LEN) {
        return FAILURE; // The resulting string would be too long
    }
    strcat(git_prompt, addition); // Safe to concatenate
    return SUCCESS;
}

/**
 * Parses a given input prompt string, replacing any embedded
 * commands with their corresponding values.
 *
 * This function scans the input string for embedded commands, which
 * are denoted by the syntax `@{command}`. Each command is looked up
 * in a global hash table of instructions, and if found, the command
 * is replaced by its associated value in the output string. If a
 * command is not found, it is left unchanged in the output. The
 * function ensures that the length of the digested prompt does not
 * exceed `PROMPT_MAX_LEN`.
 *
 * @param unparsed_git_prompt The input prompt string containing
 *        embedded commands to be parsed.
 *
 * @return A dynamically allocated string containing the digested
 *         prompt. If the resulting prompt would exceed
 *         `PROMPT_MAX_LEN`, a predefined error message is returned
 *         instead. The caller is responsible for freeing the returned
 *         string.
 */
const char *parse_prompt(const char *unparsed_git_prompt) {
  char git_prompt[PROMPT_MAX_LEN] = "";
  const char *ptr = unparsed_git_prompt;
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

      // Look up the command and append its value to git_prompt
      const char *replacement = find_replacement(command);
      if (replacement) {
        if(my_strcat(git_prompt, replacement) == FAILURE) { goto error; }
      } else {
        // Command not found, append the original command
        if(my_strcat(git_prompt, "@{") == FAILURE) { goto error; }
        if(my_strcat(git_prompt, command) == FAILURE) { goto error; }
        if(my_strcat(git_prompt, "}") == FAILURE) { goto error; }
      }
      ptr++; // Move past the '}'
    } else if (in_command) {
      // We are inside a command, accumulate characters
      command[command_index++] = *ptr++;
    } else {
      // We are outside a command, copy character directly to git_prompt
      char str[2] = {*ptr++, '\0'};
      if(my_strcat(git_prompt, str) == FAILURE) { goto error; }
    }
  }

  return strdup(git_prompt);

  error:
    return "PROMPT TOO LONG $ ";
}


int main(void) {
  struct CurrentState state;

  const char *nonGitPrompt        = getenv("GP2_NON_GIT_PROMPT") ?: "\\W$ ";
  const char *unparsed_git_prompt = getenv("GP2_GIT_PROMPT")     ?: "<@{Repo.name}> @{CWD} $ ";

  if (are_escape_sequences_properly_formed(nonGitPrompt) != SUCCESS) {
    printf("MALFORMED GP2_GIT_PROMPT $ ");
    return ERROR;
  }
  if (are_escape_sequences_properly_formed(unparsed_git_prompt) != SUCCESS) {
    printf("MALFORMED GP2_NON_GIT_PROMPT $ ");
    return ERROR;
  }



  git_libgit2_init();
  initialise_state(&state);

  if (gather_git_context(&state) == FAILURE_IS_NOT_GIT_REPO) {
    printf("%s", nonGitPrompt);
    return SUCCESS;
  }
  gather_aws_context(&state);


  /*
    parse_prompt will replace all instruction strings with their
    values - except for the CWD instruction.
  */
  add_default_instructions(&state);
  const char *git_prompt = parse_prompt(unparsed_git_prompt);
  
  /*
    We'll deal with this here - after all the other instructions have
    been applied, since we want to ensure that the current working
    directory path will fit in the terminal width.
  */
  if (strstr(git_prompt, "@{CWD}")) {
    char* cwd = (char*) get_cwd_from_home(&state);
    int cwd_length = strlen(cwd);
    int visible_prompt_length = cwd_length + count_visible_chars(git_prompt) - 6; // len("@{CWD}") = 6
    int terminal_width = 80;
    
    if (visible_prompt_length > terminal_width) {
      int max_width = cwd_length - (visible_prompt_length - terminal_width);
      path_truncate_simple(cwd, max_width);
    }
    add_instruction("CWD",  cwd);
    git_prompt = parse_prompt(git_prompt);
  }


  // Finally, print the git prompt
  printf("%s", git_prompt);


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

