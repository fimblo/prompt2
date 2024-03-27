#include <errno.h>
#include <git2.h>
#include <iniparser/iniparser.h>
//#include <limits.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <uthash.h>

#include "get-status.h"

#define COMMAND_MAX_LEN  256
#define PATH_MAX 4096


struct CommandMap {
  const char *command;      // key
  const char *replacement;  // value
  UT_hash_handle hh;        // makes this structure hashable
} ;
struct CommandMap *instructions = NULL;

// Function to add entries to the hash table
void add_command(const char *command, const char *replacement) {
  struct CommandMap *i = malloc(sizeof(struct CommandMap));
  if (i == NULL) {
    printf("HASH INSERT FAIL (malloc) $ ");
    exit(EXIT_FAILURE);
  }
  i->command = command;
  i->replacement = strdup(replacement);
  if (i->replacement == NULL) {
    free(i);
    printf("HASH INSERT FAIL (null value) $ ");
    exit(EXIT_FAILURE);
  }
  HASH_ADD_KEYPTR(hh, instructions, i->command, strlen(i->command), i);
}

// Function to find an entry in the hash table
const char *lookup_command(const char *command) {
  struct CommandMap *i;
  HASH_FIND_STR(instructions, command, i);
  return i ? i->replacement : NULL;
}

void assign_instructions(struct CurrentState *state) {

  char itoa_buf[32]; // to store numbers as strings

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_git_repo);
  add_command("Repo.is_git_repo", itoa_buf);

  add_command("Repo.name",                        state->repo_name);
  add_command("Repo.branch_name",                 state->branch_name);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_rebase_in_progress);
  add_command("Repo.rebase_active", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->conflict_num);
  add_command("Repo.conflicts", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->has_upstream);
  add_command("Repo.has_upstream", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->ahead_num);
  add_command("Repo.ahead", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->behind_num);
  add_command("Repo.behind", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->staged_num);
  add_command("Repo.staged", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->modified_num);
  add_command("Repo.modified", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->untracked_num);
  add_command("Repo.untracked", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->aws_token_is_valid);
  add_command("AWS.token_is_valid", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",                state->aws_token_remaining_hours);
  add_command("AWS.token_remaining_hours", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",                state->aws_token_remaining_minutes);
  add_command("AWS.token_remaining_minutes", itoa_buf);
}


/**
 * Helper function.
 * Concatenates a string to git_prompt if the resulting length is within bounds.
 *
 */
int safe_strcat(char *target_string, const char *addition, int max_len) {
  if (strlen(target_string) + strlen(addition) >= (size_t) max_len) {
    return FAILURE;
  }
  strcat(target_string, addition);
  return SUCCESS;
}


/**
 * Replaces occurrences of the two-character sequence '\' followed by 'n'
 * with the actual newline character '\n', in a given string.
 *
 * @param input The input string containing literal "\n" sequences.
 * @return A new string with "\n" sequences replaced by actual newline characters.
 *         The caller is responsible for freeing this string.
 */
char* replace_literal_newlines(const char* input) {
  int inputLen = strlen(input);

  char* result = malloc(inputLen + 1); // +1 for the null terminator
  if (!result) {
    perror("REPLACE LITERAL NEWLINES FAILURE $ ");
    exit(EXIT_FAILURE);
  }

  const char* current = input;
  char* output = result;
  while (*current) {
    if (*current == '\\' && *(current + 1) == 'n') {
      *output++ = '\n';
      current += 2;
    } else {
      *output++ = *current++;
    }
  }
  *output = '\0'; // Null-terminate

  return result;
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
      const char *replacement = lookup_command(command);
      if (replacement) {
        if(safe_strcat(git_prompt, replacement, PROMPT_MAX_LEN) == FAILURE) { goto error; }
      } else {
        // Command not found, append the original command
        if(safe_strcat(git_prompt, "@{", PROMPT_MAX_LEN) == FAILURE) { goto error; }
        if(safe_strcat(git_prompt, command, PROMPT_MAX_LEN) == FAILURE) { goto error; }
        if(safe_strcat(git_prompt, "}", PROMPT_MAX_LEN) == FAILURE) { goto error; }
      }
      ptr++; // Move past the '}'
    } else if (in_command) {
      // We are inside a command, accumulate characters
      command[command_index++] = *ptr++;
    } else {
      // We are outside a command, copy character directly to git_prompt
      char str[2] = {*ptr++, '\0'};
      if(safe_strcat(git_prompt, str, PROMPT_MAX_LEN) == FAILURE) { goto error; }
    }
  }

  return strdup(git_prompt);

 error:
  return "PROMPT TOO LONG $ ";
}

/**
 * helper. Get terminal width.
 *
 * Note that I check stderr and not stdout, since stderr is less
 * likely to be piped or redirected.
 */
int term_width() {
  struct winsize w;
  if (ioctl(STDERR_FILENO, TIOCGWINSZ, &w) == -1) {
    perror("ioctl error");
    return -1;
  }
  return (int) w.ws_col;
}


char * get_cwd(struct CurrentState *state, const char *cwd_type) {
  char *cwd_path;

  if (strcmp(cwd_type, "full") == 0) {
    cwd_path = (char *)state->cwd_full;
  }
  else if (strcmp(cwd_type, "basename") == 0) {
    cwd_path = (char *)state->cwd_basename;
  }
  else if (strcmp(cwd_type, "git") == 0) {
    cwd_path = (char *)get_cwd_from_gitrepo(state);
  }
  else {
    cwd_path = (char *)get_cwd_from_home(state);
  }
  return cwd_path;
}

void truncate_with_ellipsis(char *str, size_t max_width) {
  if (str && strlen(str) > max_width) {
    strcpy(&str[max_width - 3], "...");
  }
}

struct Config {
  const char * cwd_type;
  size_t branch_max_width;
};

void initialise_config(struct Config *config) {
  config->cwd_type = "home";
  config->branch_max_width = (size_t) 40;
}

/**
 * read from config file, save to and return config struct
 */
int read_config(struct Config *config) {
  // Set default values
  initialise_config(config);

  // Find INI file
  char *config_file_name = ".prompt2_config.ini";
  char config_file_path[PATH_MAX];
  const char *config_dirs[] = {".", getenv("HOME")};
  int found = 0;
  for (long unsigned int i = 0; i < sizeof(config_dirs)/sizeof(config_dirs[0]); i++) {
    sprintf(config_file_path, "%s/%s", config_dirs[i], config_file_name);
    if (access(config_file_path, R_OK) == 0) {
      found = 1;
      break;
    }
  }
  if (!found) {
    // use default values since no config found
    return ERROR;
  }

  // Load INI file
  dictionary *ini = iniparser_load(config_file_path);
  if (ini == NULL) {
    // do nothing. We will use the default config
    return ERROR;
  }

  // Set config struct from ini file
  char b[128];
  sprintf(b, "%d", (int) config->branch_max_width);
  config->branch_max_width = (size_t) atoi(iniparser_getstring(ini, "GENERIC:branch_max_width", b));
  config->cwd_type = strdup(iniparser_getstring(ini, "GENERIC:cwd_type", config->cwd_type));


  // Free the dictionary
  iniparser_freedict(ini);

  return SUCCESS;
}


int main(void) {
  struct CurrentState state;
  struct Config config;

  //  Get environment variables and check them for inconsistencies
  const char *plain_prompt   = getenv("GP2_NON_GIT_PROMPT") ?: "\\W$ ";
  const char *gp2_git_prompt = getenv("GP2_GIT_PROMPT")     ?: "<@{Repo.name}> @{CWD} $ ";

  if (are_escape_sequences_properly_formed(plain_prompt) != SUCCESS) {
    printf("MALFORMED GP2_NON_GIT_PROMPT $ ");
    return ERROR;
  }
  if (are_escape_sequences_properly_formed(gp2_git_prompt) != SUCCESS) {
    printf("MALFORMED GP2_GIT_PROMPT $ ");
    return ERROR;
  }
  // todo: add quality check for gp2 instructions


  git_libgit2_init();
  initialise_state(&state);
  read_config(&config);

  if (gather_git_context(&state) == FAILURE_IS_NOT_GIT_REPO) {
    printf("%s", plain_prompt);
    return SUCCESS;
  }
  gather_aws_context(&state);


  // Limit branch name string length
  truncate_with_ellipsis((char *) state.branch_name, config.branch_max_width);


  /*
    parse_prompt will replace all instruction strings with their
    values - except for the CWD instruction.
  */
  assign_instructions(&state);

  // for tokenization on \n to work, we need to replace the string "\n" with a newline character.
  const char * unparsed_git_prompt = replace_literal_newlines(gp2_git_prompt);
  const char *git_prompt = parse_prompt(unparsed_git_prompt);

  /*
    We'll deal with this here - after all the other instructions have
    been applied, since we want to ensure that the current working
    directory path will fit in the terminal width - for each line in the prompt.
  */
  int terminal_width = term_width() ?: 80;


  char temp_prompt[PROMPT_MAX_LEN] = "";
  char *tokenized_prompt = strdup(git_prompt);
  char *line = strtok(tokenized_prompt, "\n");

  while (line != NULL) {
    if (strstr(line, "@{CWD}")) {
      char* cwd = get_cwd(&state, config.cwd_type);
      int cwd_length = strlen(cwd);
      int visible_prompt_length = cwd_length + count_visible_chars(line) - 6; // len("@{CWD}") = 6

      if (visible_prompt_length > terminal_width) {
        int max_width = cwd_length - (visible_prompt_length - terminal_width);
        shorten_path(cwd, max_width);
      }
      add_command("CWD",  cwd);
      line = (char *) parse_prompt(line); // Re-parse the current line
    }

    // Append the processed line to temp_prompt
    if (strlen(temp_prompt) + strlen(line) < PROMPT_MAX_LEN - 1) {
      strcat(temp_prompt, line);
      strcat(temp_prompt, "\n"); // Re-add the newline character
    } else {
      printf("PROMPT TOO LONG $ ");
      return ERROR;
    }

    line = strtok(NULL, "\n"); // Get the next line
  }

  git_prompt = strdup(temp_prompt);
  free(tokenized_prompt); // Free the duplicated string used for tokenization

  // Finally, print the git prompt
  printf("%s", git_prompt);


  cleanup_resources(&state);
  git_libgit2_shutdown();
  struct CommandMap *current_entry, *tmp;
  HASH_ITER(hh, instructions, current_entry, tmp) {
    HASH_DEL(instructions, current_entry);
    free((char*)current_entry->replacement);
    free(current_entry);
  }

  return 0;
}
