#include <errno.h>
#include <git2.h>
//#include <limits.h>
#include <stdio.h>
#include <uthash.h>
#ifdef __unix__
#include <iniparser/iniparser.h>
#elif __APPLE__
#include <iniparser.h>
#else
#error "Unknown or unsupported OS"
#endif


#include "get-status.h"
#include "prompt2-utils.h"

#define COMMAND_MAX_LEN        256
#define WIDGET_MAX_LEN         256
#define INI_SECTION_MAX_SIZE   64
#define ITOA_BUFFER_SIZE       8
#define DEFAULT_TERMINAL_WIDTH 80
#define BRANCH_MAX_WIDTH       128
#define PATH_MAX               4096
#define INI_SECTION_DEFAULT    "default"
#define INI_SECTION_GENERIC    "generic"


// struct to contain configuration for a widget
struct WidgetConfig {
  char *string_active;
  char *string_inactive;
  char *colour_on;
  char *colour_off;
};

// Struct to contain non-widget configuration
//
struct ConfigRoot {
  char *              cwd_type;
  size_t              branch_max_width;
  struct WidgetConfig defaults;
};

// struct to store all widget configes (except default)
struct WidgetConfigMap {
  char                *name;
  struct WidgetConfig  config;
  UT_hash_handle       hh; // makes this structure hashable
};
struct WidgetConfigMap *configurations = NULL;


void print_debug_widget_config(struct WidgetConfig wc) {
  char * reset = "\\[\\033[0m\\]";
  printf("string_active: '%s'\n", wc.string_active);
  printf("string_inactive: '%s'\n", wc.string_inactive);
  printf("colour_on: '%s'%s\n", wc.colour_on, reset);
  printf("colour_off: '%s'%s\n", wc.colour_off, reset);
}


// Function to find a widget configuration
struct WidgetConfig *get_widget_config(const char *name) {
  struct WidgetConfigMap *s;

  HASH_FIND_STR(configurations, name, s);
  if (s) {
    return &s->config;
  }
  return NULL;
}


// Function to add or update a widget configuration
void upsert_widget_config(const char *name, struct WidgetConfig widget_config) {
  struct WidgetConfigMap *s;

  HASH_FIND_STR(configurations, name, s);
  if (s == NULL) {
    s = (struct WidgetConfigMap *)malloc(sizeof(struct WidgetConfigMap));
    s->name = strdup(name);
    HASH_ADD_KEYPTR(hh, configurations, s->name, strlen(s->name), s);
  }
  // Update the configuration
  s->config = widget_config;
}


// transfer INI file section into a WidgetConfig struct
// if this fails, use the default values.
void populate_widget_from_config(dictionary *ini,
                                 const char *section,
                                 struct WidgetConfig *widget_config,
                                 const struct WidgetConfig *defaults) {
  char key[INI_SECTION_MAX_SIZE];
  const char *default_string_active   = defaults ? defaults->string_active : "";
  const char *default_string_inactive = defaults ? defaults->string_inactive : "";
  const char *default_colour_on       = defaults ? defaults->colour_on : "";
  const char *default_colour_off      = defaults ? defaults->colour_off : "";

  snprintf(key, sizeof(key), "%s:string_active", section);
  widget_config->string_active = strdup(iniparser_getstring(ini, key, default_string_active));
  snprintf(key, sizeof(key), "%s:string_inactive", section);
  widget_config->string_inactive = strdup(iniparser_getstring(ini, key, default_string_inactive));
  snprintf(key, sizeof(key), "%s:colour_on", section);
  widget_config->colour_on = strdup(iniparser_getstring(ini, key, default_colour_on));
  snprintf(key, sizeof(key), "%s:colour_off", section);
  widget_config->colour_off = strdup(iniparser_getstring(ini, key, default_colour_off));
}


/**
 * read from config file, save to and return config struct
 */
int read_ini_config(struct ConfigRoot *config) {
  // Set hard-coded default values
  config->cwd_type = "home";
  config->branch_max_width = (size_t) 40;

  // Find INI file either in . or home
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
  if (!found) goto default_config;

  // Load INI file
  dictionary *ini = iniparser_load(config_file_path);
  if (ini == NULL) goto default_config;

  // Set config struct from ini file
  config->cwd_type = strdup(iniparser_getstring(ini, "GENERIC:cwd_type", config->cwd_type));
  char bmw_tmp[BRANCH_MAX_WIDTH];
  sprintf(bmw_tmp, "%d", (int) config->branch_max_width);
  config->branch_max_width = (size_t) atoi(iniparser_getstring(ini, "GENERIC:branch_max_width", bmw_tmp));

  // Set widget defaults
  populate_widget_from_config(ini, INI_SECTION_DEFAULT, &config->defaults, NULL);

  // Read each ini section  
  for (int i = 0; i < iniparser_getnsec(ini); i++) {
    const char * section = iniparser_getsecname(ini, i);
    if (strcmp(section, INI_SECTION_DEFAULT) == 0) continue;
    if (strcmp(section, INI_SECTION_GENERIC) == 0) continue;

    struct WidgetConfig wc = { NULL, NULL, NULL, NULL };
    populate_widget_from_config(ini, section, &wc, &config->defaults);
    upsert_widget_config(section, wc);
  }

  // Free the dictionary
  iniparser_freedict(ini);
  return SUCCESS;

  // If there is no config file, just go with these plain defaults
  default_config:
    config->defaults.string_active   = "%s";
    config->defaults.string_inactive = "%s";
    config->defaults.colour_on       = "";
    config->defaults.colour_off      = "";
    return ERROR;
}


void setup_instruction_map(struct CurrentState *state, struct CommandMap **instructions) {

  char itoa_buf[ITOA_BUFFER_SIZE]; // to store numbers as strings

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_git_repo);
  add_command(instructions, "repo.is_git_repo",   itoa_buf);

  add_command(instructions, "repo.name",          state->repo_name);
  add_command(instructions, "repo.branch_name",   state->branch_name);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_rebase_in_progress);
  add_command(instructions, "repo.rebase_active", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->conflict_num);
  add_command(instructions, "repo.conflicts",     itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->has_upstream);
  add_command(instructions, "repo.has_upstream",  itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->ahead_num);
  add_command(instructions, "repo.ahead",         itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->behind_num);
  add_command(instructions, "repo.behind",        itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->staged_num);
  add_command(instructions, "repo.staged",        itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->modified_num);
  add_command(instructions, "repo.modified",      itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->untracked_num);
  add_command(instructions, "repo.untracked",     itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->aws_token_is_valid);
  add_command(instructions, "aws.token_is_valid", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",               state->aws_token_remaining_hours);
  add_command(instructions, "aws.token_remaining_hours",   itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",               state->aws_token_remaining_minutes);
  add_command(instructions, "aws.token_remaining_minutes", itoa_buf);
}


// return 0 if false, 1 if true
int is_widget_active(const char * name, const char *value) {
  char * name_lowercase = strdup(name);
  to_lower(name_lowercase);
  /*
    Widgets can be active or inactive.

    To discern which of the two states a widget should be in, we can
    look at the value parameter. Currently, there are three ways to
    check, depending on the type of widget.

    TYPE_STRING widgets are inactive if value is the empty string.
    TYPE_TOGGLE widgets are inactive if value is "0" (not int. string)
    
    The third type of widget are special cases and are treated
    specially.

    For example, the widget 'Repo.is_git_repo' is active when the user
    is standing in a directory which is a git repo (not "0"), and
    inactive otherwise ("0").
    
    | WIDGET                         | inactive     | active      |
    | ------------------------------ | ------------ | ----------- |
    | `cwd.full`                     | empty string | string      |
    | `cwd.basename`                 | empty string | string      |
    | `cwd.git_path`                 | empty string | string      |
    | `cwd.home_path`                | empty string | string      |
    | `repo.name`                    | empty string | string      |
    | `repo.branch_name`             | empty string | string      |
    | `repo.is_git_repo`             | "0"          | otherwise   |
    | `repo.rebase_active`           | "0"          | otherwise   |
    | `repo.conflicts`               | "0"          | otherwise   |
    | `repo.has_upstream`            | "0"          | otherwise   |
    | `repo.ahead`                   | "0"          | otherwise   |
    | `repo.behind`                  | "0"          | otherwise   |
    | `repo.staged`                  | "0"          | otherwise   |
    | `repo.modified`                | "0"          | otherwise   |
    | `repo.untracked`               | "0"          | otherwise   |
    | `aws.token_is_valid`           | "0"          | otherwise   |
    | `aws.token_remaining_hours`    | >0           | <=0         |
    | `aws.token_remaining_minutes`  | >10          | <=10        |

    The code below checks if the widget should be set to active or
    inactive.
  */

  typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_STRING,
    TYPE_TOGGLE
  } WidgetType;

  struct WidgetTypeTable {
    const char *name;
    WidgetType type;
  };


  // Define the widget type entries
  const struct WidgetTypeTable widget_type_table[] = {
    { "cwd",                TYPE_STRING },
    { "repo.name",          TYPE_STRING },
    { "repo.branch_name",   TYPE_STRING },

    { "repo.is_git_repo",   TYPE_TOGGLE },
    { "repo.rebase_active", TYPE_TOGGLE },
    { "repo.conflicts",     TYPE_TOGGLE },
    { "repo.has_upstream",  TYPE_TOGGLE },
    { "repo.ahead",         TYPE_TOGGLE },
    { "repo.behind",        TYPE_TOGGLE },
    { "repo.staged",        TYPE_TOGGLE },
    { "repo.modified",      TYPE_TOGGLE },
    { "repo.untracked",     TYPE_TOGGLE },
    { "aws.token_is_valid", TYPE_TOGGLE }
  };

  int type = TYPE_UNKNOWN;
  for (size_t i = 0; i < sizeof(widget_type_table) / sizeof(widget_type_table[0]); i++) {
    if (strcmp(name_lowercase, widget_type_table[i].name) == 0) {
      type = widget_type_table[i].type;
      break;
    }
  }
  if (type == TYPE_UNKNOWN) {
    return 0;
  }

  int is_active = 0;
  if (type == TYPE_STRING && value[0] != '\0') is_active = 1;
  else if (type == TYPE_TOGGLE && strcmp(value, "0") != 0) is_active = 1;
  else if (strcmp(name_lowercase, "aws.token_remaining_hours") == 0) {
    if (atoi(value) <= 0) is_active = 1;
  }
  else if (strcmp(name_lowercase, "aws.token_remaining_minutes") == 0) {
    if (atoi(value) <= 10) is_active = 1;
  }
  return is_active;
}


const char *format_widget(const char *name, const char *value, int is_active, struct WidgetConfig *defaults) {
  char *name_lowercase = strdup(name);
  to_lower(name_lowercase);

  struct WidgetConfig *wc = get_widget_config(name_lowercase);
  if (!wc) {
    wc = defaults;
  }
  // Format the value
  char widget[WIDGET_MAX_LEN];
  const char *format_string = is_active ? wc->string_active : wc->string_inactive;
  snprintf(widget, sizeof(widget), format_string, value);

  // Wrap resulting string in colours (if needed)
  const char *colour_string = is_active ? wc->colour_on : wc->colour_off;
  const char *reset_term_colours = "";

  if (strstr(colour_string, "\\[\\033[") != NULL || strstr(colour_string, "\\[\\e[") != NULL) {
    reset_term_colours = "\\[\\033[0m\\]";
  }
  snprintf(widget, sizeof(widget), "%s%s%s", colour_string, value, reset_term_colours);
 
  return strdup(widget);
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
const char *parse_prompt(const char *unparsed_git_prompt,
                         struct CommandMap *instructions,
                         struct WidgetConfig *defaults) {
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
      const char *replacement = lookup_command(&instructions, command);
      if (replacement) {
        int is_active = is_widget_active(command, replacement);
        const char *formatted_replacement = format_widget(command, replacement, is_active, defaults);
        if(safe_strcat(git_prompt, formatted_replacement, PROMPT_MAX_LEN) == FAILURE) { goto error; }
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


int main(void) {
  struct CurrentState state;
  struct ConfigRoot config;
  struct CommandMap *instructions = NULL;
  
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
  read_ini_config(&config);

  if (gather_git_context(&state) == FAILURE_IS_NOT_GIT_REPO) {
    printf("%s", plain_prompt);
    return SUCCESS;
  }
  gather_aws_context(&state);

  // Limit branch name string length
  truncate_with_ellipsis((char *) state.branch_name, config.branch_max_width);

  add_command(&instructions, "key", "value");

  /*
    parse_prompt will replace all instruction strings with their
    values - except for the CWD instruction.
  */
  setup_instruction_map(&state, &instructions);
  
  // for tokenization on \n to work, we need to replace the string "\n" with a newline character.
  const char * unparsed_git_prompt = replace_literal_newlines(gp2_git_prompt);
  const char *git_prompt = parse_prompt(unparsed_git_prompt, instructions, &config.defaults);
  int terminal_width = term_width() ?: DEFAULT_TERMINAL_WIDTH;
  
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
      add_command(&instructions, "cwd",  cwd); // lowercase
      line = (char *) parse_prompt(line, instructions, &config.defaults); // Re-parse the current line
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

  // clean/free memory
  cleanup_resources(&state);
  git_libgit2_shutdown();

  struct WidgetConfigMap *current2, *tmp2;
  HASH_ITER(hh, configurations, current2, tmp2) {
    HASH_DEL(configurations, current2);
    free(current2->name);
    free(current2);
  }

  return 0;
}
