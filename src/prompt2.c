/*
 * prompt2
 *
 * Generate a dynamic shell prompt based on the environment context.
 *
 * This program reads configuration from an INI file and constructs a
 * shell prompt that reflects the current state of the environment.
 *
 * A "Widget" is a component of the prompt that represents a specific
 * aspect of the environment, such as the current git branch,
 * repository status, or working directory. Widgets are dynamic; they
 * can be active or inactive based on the state they represent. For
 * instance, the "Repo.name" widget is active when the current
 * directory is within a git repository.
 *
 * Widgets are configurable in terms of text and colour, with separate
 * settings for their active and inactive states. For example, the
 * "Repo.name" widget configuration might look like this:
 *   - string_active: The format string used when the widget is active
 *     (e.g., displaying the git branch name). eg: "<%s>"
 *   - string_inactive: The format string used when the widget is
 *     inactive (e.g., an empty string or placeholder text). eg: "-"
 *   - colour_on: The colour code applied to the active widget text.
 *   - colour_off: The colour code applied to the inactive widget text.
 *
 * Widgets are assembled into a prompt using the configuration
 * variables `git_prompt` and `default_prompt`. These variables define
 * the structure of the prompt, specifying where each widget should
 * appear. Widgets are denoted in these strings by their placeholders,
 * which follow the pattern `@{widget_name}`. During prompt
 * generation, these placeholders are replaced with the actual content
 * of the corresponding widgets, formatted according to their
 * active/inactive state and the environment context.
 *
 * The final prompt is a composition of these widgets, providing a
 * rich, context-aware command line experience.
 */

#include <errno.h>
#include <git2.h>
#include <stdio.h>
#include <uthash.h>
#ifdef __unix__
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>
#include <linux/limits.h>
#elif __APPLE__
#include <dictionary.h>
#include <iniparser.h>
#include <sys/syslimits.h>
#else
#error "Unknown or unsupported OS"
#endif

#include "constants.h"
#include "get-status.h"
#include "prompt2-utils.h"

/**
   Max length of any widget token in the config file variable
   `git_prompt`
*/
#define WIDGET_TOKEN_MAX_LEN        256

/**
   Max length in characters of a widget in the resulting prompt
*/
#define WIDGET_MAX_LEN         256

/**
   Width of terminal if I can't get it from ioctl
*/
#define DEFAULT_TERMINAL_WIDTH 80

/**
   Max number of widgets types
*/
#define DICTIONARY_MAX_SIZE    64

/**
   Max length of a section in an ini file
*/
#define INI_SECTION_MAX_SIZE   64
#define INI_SECTION_WIDGET_DEFAULT  "widget_default"


/**
   Struct to contain configuration for a single widget
*/
struct WidgetConfig {
  char *string_active;
  char *string_inactive;
  char *colour_on;
  char *colour_off;
  int max_width;
};

/**
   Struct to contain non-widget configuration
*/
struct ConfigRoot {
  char *              default_prompt;
  char *              default_prompt_cwd_type;
  char *              git_prompt;
  char *              git_prompt_zero;
  char *              git_prompt_cwd_type;
  struct WidgetConfig defaults;

  // horrid way to ensure to free these if necessary
  int dynamic_default_prompt;
  int dynamic_git_prompt;
  int dynamic_widget_config;
};

/**
   Hash table to store all widget configs
   (except the default widget which is in the ConfigRoot struct)
*/
struct WidgetConfigMap {
  char                *name;
  struct WidgetConfig  config;
  UT_hash_handle       hh; // makes this structure hashable
};
struct WidgetConfigMap *configurations = NULL;


/**
 * helper function for debugging
*/
void __print_debug_widget_config(struct WidgetConfig wc) {
  char * reset = "\\[\\033[0m\\]";
  printf("string_active: '%s'\n", wc.string_active);
  printf("string_inactive: '%s'\n", wc.string_inactive);
  printf("colour_on: '%s'%s\n", wc.colour_on, reset);
  printf("colour_off: '%s'%s\n", wc.colour_off, reset);
  printf("max_width: %d\n", wc.max_width);
}


/**
 * Retrieves a widget configuration from the hash table by name.
 *
 * This function searches the hash table for a widget configuration
 * with the specified name. If found, it returns a pointer to the
 * WidgetConfig struct; otherwise, it returns NULL.
 *
 * @param name The name of the widget to retrieve.
 * @return     A pointer to the WidgetConfig struct if found, or NULL
 *             if not found.
 */
struct WidgetConfig *get_widget(const char *name) {
  struct WidgetConfigMap *s;

  HASH_FIND_STR(configurations, name, s);
  if (s) {
    return &s->config;
  }
  return NULL;
}


/**
 * Saves the widget in the hash table for later retrieval.
 *
 * This function stores a WidgetConfig in a hash table, allowing it to
 * be retrieved later by name. If a widget with the same name already
 * exists, its configuration is updated.
 *
 * @param name The name of the widget to save.
 * @param widget_config The WidgetConfig struct containing the
 *                      widget's configuration.
 */
void save_widget(const char *name, struct WidgetConfig widget_config) {
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


/**
 * Transfers INI file section into a WidgetConfig struct.
 *
 * This function populates a WidgetConfig struct with values from a
 * specified INI file section. If a value is not found in the INI
 * file, it uses the provided default values.
 *
 * @param ini The dictionary representing the INI file.
 * @param section The section of the INI file to read.
 * @param widget_config The WidgetConfig struct to populate.
 * @param defaults The default values to use if a value is not found
 *                 in the INI file.
 */
void create_widget(dictionary *ini,
                                 const char *section,
                                 struct WidgetConfig *widget_config,
                                 const struct WidgetConfig *defaults) {
  const char *default_string_active   = defaults ? defaults->string_active : "";
  const char *default_string_inactive = defaults ? defaults->string_inactive : "";
  const char *default_colour_on       = defaults ? defaults->colour_on : "";
  const char *default_colour_off      = defaults ? defaults->colour_off : "";
  const int   default_max_width       = defaults ? defaults->max_width : WIDGET_MAX_LEN;

  char key[INI_SECTION_MAX_SIZE];
  snprintf(key, sizeof(key), "%s:string_active", section);
  widget_config->string_active = strdup(iniparser_getstring(ini, key, default_string_active));
  snprintf(key, sizeof(key), "%s:string_inactive", section);
  widget_config->string_inactive = strdup(iniparser_getstring(ini, key, default_string_inactive));
  snprintf(key, sizeof(key), "%s:colour_on", section);
  widget_config->colour_on = strdup(iniparser_getstring(ini, key, default_colour_on));
  snprintf(key, sizeof(key), "%s:colour_off", section);
  widget_config->colour_off = strdup(iniparser_getstring(ini, key, default_colour_off));
  snprintf(key, sizeof(key), "%s:max_width", section);
  widget_config->max_width = iniparser_getint(ini, key, default_max_width);
}


/**
 * Sets the default values for all fields in the configuration
 * structure.
 *
 * This function initializes the configuration structure with default
 * values. Any field set in the configuration file will override the
 * corresponding default value.
 *
 * @param config The configuration structure to initialize with
 *               default values.
 */
void set_config_defaults(struct ConfigRoot *config) {
  // Set default prompt defaults  
  config->default_prompt = "\\W $ ";
  config->default_prompt_cwd_type = "home";

  // Set git prompt defaults
  config->git_prompt = "G: \\W $ ";
  config->git_prompt_zero = "Z: \\W $ ";
  config->git_prompt_cwd_type = "home";

  // Set widget defaults
  config->defaults.string_active   = "%s";
  config->defaults.string_inactive = "%s";
  config->defaults.colour_on       = "";
  config->defaults.colour_off      = "";
  config->defaults.max_width       = WIDGET_MAX_LEN;

  config->dynamic_default_prompt = 0;
  config->dynamic_git_prompt     = 0;
  config->dynamic_widget_config  = 0;
}



/**
 * Handles the configuration of the prompt by loading settings from an
 * INI file.
 *
 * This function sets default values for the configuration and then
 * attempts to load and override these values from a specified INI
 * file. If no file path is provided, it searches for a default
 * configuration file in the current directory or the user's home
 * directory.
 *
 * @param config The configuration structure to populate.
 * @param config_file_path The path to the INI file to load. If NULL,
 *                         a default file is searched for.
 * @return SUCCESS unless something goes wrong
 */
int handle_configuration(struct ConfigRoot *config, const char *config_file_path) {
  // Set all default values first
  set_config_defaults(config);

  char selected_config_file[PATH_MAX];
  if (config_file_path == NULL) {
    // Find INI file either in . or home
    char *default_config_filename = ".prompt2_config.ini";
    const char *config_dirs[] = {".", getenv("HOME")};
    int found = 0;
    for (long unsigned int i = 0; i < sizeof(config_dirs)/sizeof(config_dirs[0]); i++) {
      snprintf(selected_config_file, PATH_MAX, "%s/%s", config_dirs[i], default_config_filename);
      if (access(selected_config_file, R_OK) == 0) {
        found = 1;
        break;
      }
    }
    if (!found) return ERROR_DEFAULT_INI_FILE_NOT_FOUND;
  }
  else {
    if (access(config_file_path, R_OK) != 0) {
      return ERROR_CUSTOM_INI_FILE_NOT_FOUND;
    }
    sprintf(selected_config_file, "%s", config_file_path);
  }


  // Load INI file
  dictionary *ini = iniparser_load(selected_config_file);
  if (ini == NULL) return ERROR_INVALID_INI_FILE;

  // get prompt-related config
  if (iniparser_find_entry(ini, "PROMPT.DEFAULT") == 1) {
    config->default_prompt          = strdup(iniparser_getstring(ini, "PROMPT.DEFAULT:prompt",     config->default_prompt));
    config->default_prompt_cwd_type = strdup(iniparser_getstring(ini, "PROMPT.DEFAULT:cwd_type",   config->default_prompt_cwd_type));
    config->dynamic_default_prompt = 1;
  }

  if (iniparser_find_entry(ini, "PROMPT.GIT") == 1) {
    config->git_prompt          = strdup(iniparser_getstring(ini, "PROMPT.GIT:prompt",     config->git_prompt));
    config->git_prompt_zero     = strdup(iniparser_getstring(ini, "PROMPT.GIT:special",    config->git_prompt_zero));
    config->git_prompt_cwd_type = strdup(iniparser_getstring(ini, "PROMPT.GIT:cwd_type",   config->git_prompt_cwd_type));
    config->dynamic_git_prompt = 1;
  }

  // Set default widget to fall back on
  if (iniparser_find_entry(ini, INI_SECTION_WIDGET_DEFAULT) == 1) {
    create_widget(ini, INI_SECTION_WIDGET_DEFAULT, &config->defaults, &config->defaults);
    config->dynamic_widget_config = 1;
  }

  // Read each ini section
  for (int i = 0; i < iniparser_getnsec(ini); i++) {
    const char * section = iniparser_getsecname(ini, i);
    if (strcmp(section, INI_SECTION_WIDGET_DEFAULT) == 0) continue;

    struct WidgetConfig wc = { NULL, NULL, NULL, NULL, 0 };
    create_widget(ini, section, &wc, &config->defaults);
    save_widget(section, wc);
  }

  // Free the dictionary
  iniparser_freedict(ini);
  return SUCCESS;
}

/**
 * Maps widget tokens to their corresponding values based on the
 * current state.
 *
 * This function populates a dictionary with key-value pairs where the
 * keys are widget tokens and the values are derived from the current
 * state of the environment.
 *
 * @param dict The dictionary to populate with widget tokens and their values.
 * @param state The current state of the environment from which values are derived.
 */
void map_wtoken_to_state(dictionary *dict, struct CurrentState *state) {

  char itoa_buf[ITOA_BUFFER_SIZE]; // to store numbers as strings

  dictionary_set(dict, "sys.username",            state->username);
  dictionary_set(dict, "sys.hostname",            state->hostname);
  dictionary_set(dict, "sys.promptchar",          state->uid ? "$": "#");

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      (int) state->uid);
  dictionary_set(dict, "sys.uid",            itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      (int) state->gid);
  dictionary_set(dict, "sys.gid",            itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_git_repo);
  dictionary_set(dict, "repo.is_git_repo",   itoa_buf);

  dictionary_set(dict, "repo.name",              state->repo_name);
  dictionary_set(dict, "repo.branch_name",       state->branch_name);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->is_rebase_in_progress);
  dictionary_set(dict, "repo.rebase_active", itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->conflict_num);
  dictionary_set(dict, "repo.conflicts",     itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->has_upstream);
  dictionary_set(dict, "repo.has_upstream",  itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->ahead_num);
  dictionary_set(dict, "repo.ahead",         itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->behind_num);
  dictionary_set(dict, "repo.behind",        itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->staged_num);
  dictionary_set(dict, "repo.staged",        itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->modified_num);
  dictionary_set(dict, "repo.modified",      itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->untracked_num);
  dictionary_set(dict, "repo.untracked",     itoa_buf);

  snprintf(itoa_buf, sizeof(itoa_buf), "%d",      state->aws_token_is_valid);
  dictionary_set(dict, "aws.token_is_valid", itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",               state->aws_token_remaining_hours);
  dictionary_set(dict, "aws.token_remaining_hours",   itoa_buf);
  snprintf(itoa_buf, sizeof(itoa_buf), "%d",               state->aws_token_remaining_minutes);
  dictionary_set(dict, "aws.token_remaining_minutes", itoa_buf);
}


/**
 * Determines if a widget is active based on its token and value.
 *
 * Widgets can be active or inactive depending on the type of widget
 * and its value.
 * - String-type widgets are inactive if the value is an empty string.
 * - Number-type widgets are inactive if the value is zero or negative.
 * - Special cases are handled for specific widgets like
     `aws.token_remaining_hours` and `aws.token_remaining_minutes`.
 *
 * @param wtoken The widget token to check.
 * @param value The value associated with the widget token.
 * @return 1 if the widget is active, 0 if inactive.
 */
int is_widget_active(const char * wtoken, const char *value) {
  char * wtoken_lc = to_lower(wtoken);

  /*
    Widgets can be active or inactive.

    To discern which of the two states a widget should be in, we can
    look at the value parameter. Currently, there are three ways to
    check, depending on the type of widget.

    TYPE_STRING widgets are inactive if value is the empty string.
    TYPE_TOGGLE widgets are inactive if value is zero or negative.

    The third type of widget are special cases and are treated
    specially.

    For example, the widget 'Repo.is_git_repo' is active when the user
    is standing in a directory which is a git repo (not "0"), and
    inactive otherwise ("0").

    | WIDGET                         | inactive     | active      |
    | ------------------------------ | ------------ | ----------- |
    | `sys.username`                 | empty string | string      |
    | `sys.hostname`                 | empty string | string      |
    | `cwd.full`                     | empty string | string      |
    | `cwd.basename`                 | empty string | string      |
    | `cwd.git_path`                 | empty string | string      |
    | `cwd.home_path`                | empty string | string      |
    | `repo.name`                    | empty string | string      |
    | `repo.branch_name`             | empty string | string      |
    | `sys.uid`                      | <1           | otherwise   |
    | `sys.gid`                      | <1           | otherwise   |
    | `repo.is_git_repo`             | <1           | otherwise   |
    | `repo.rebase_active`           | <1           | otherwise   |
    | `repo.conflicts`               | <1           | otherwise   |
    | `repo.has_upstream`            | <1           | otherwise   |
    | `repo.ahead`                   | <1           | otherwise   |
    | `repo.behind`                  | <1           | otherwise   |
    | `repo.staged`                  | <1           | otherwise   |
    | `repo.modified`                | <1           | otherwise   |
    | `repo.untracked`               | <1           | otherwise   |
    | `aws.token_is_valid`           | <1           | otherwise   |
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
    const char *wtoken;
    WidgetType type;
  };


  // Define the widget type entries
  const struct WidgetTypeTable widget_type_table[] = {
    { "sys.username",       TYPE_STRING },
    { "sys.hostname",       TYPE_STRING },
    { "cwd",                TYPE_STRING },
    { "repo.name",          TYPE_STRING },
    { "repo.branch_name",   TYPE_STRING },

    { "sys.uid",            TYPE_TOGGLE },
    { "sys.gid",            TYPE_TOGGLE },
    { "sys.promptchar",     TYPE_TOGGLE },
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
    if (strcmp(wtoken_lc, widget_type_table[i].wtoken) == 0) {
      type = widget_type_table[i].type;
      break;
    }
  }
  if (type == TYPE_UNKNOWN) {
    return 0;
  }

  int is_active = 0;
  if (type == TYPE_STRING && value[0] != '\0') is_active = 1;
  else if (type == TYPE_TOGGLE && atoi(value) > 0) is_active = 1;
  else if (strcmp(wtoken_lc, "aws.token_remaining_hours") == 0) {
    if (atoi(value) <= 0) is_active = 1;
  }
  else if (strcmp(wtoken_lc, "aws.token_remaining_minutes") == 0) {
    if (atoi(value) <= 10) is_active = 1;
  }
  return is_active;
}


const char *format_widget(const char *name, const char *value, int is_active, struct WidgetConfig *defaults) {
  const char *lower_name = to_lower(name);
  struct WidgetConfig *wc = get_widget(lower_name);
  if (!wc) {
    wc = defaults;
  }
  // Format the value
  char padded_value[4];
  char *value_to_format;
  if (strcmp(lower_name, "aws.token_remaining_minutes") == 0 && strlen(value) == 1) {
    snprintf(padded_value, sizeof(padded_value), "0%s", value);
    value_to_format = padded_value;
  }
  else {
    value_to_format = (char *) value;
  }

  if (strlen(value_to_format) > (size_t) wc->max_width) {
    if (strcmp(lower_name, "cwd") == 0) {
      shorten_path(value_to_format, wc->max_width);
    }
    else {
      truncate_with_ellipsis(value_to_format, (size_t) wc->max_width);
    }
  }

  char widget[WIDGET_MAX_LEN];
  const char *format_string = is_active ? wc->string_active : wc->string_inactive;
  snprintf(widget, sizeof(widget), format_string, value_to_format);
  
  // Wrap resulting string in colours
  const char *colour_string = is_active ? wc->colour_on : wc->colour_off;

  // define reset if colours were added in the preceding step
  const char *reset_term_colours = "";
  if (strstr(colour_string, "\\[\\033[") != NULL || strstr(colour_string, "\\[\\e[") != NULL) {
    reset_term_colours = "\\[\\033[0m\\]";
  }
  char coloured_widget[WIDGET_MAX_LEN];
  snprintf(coloured_widget, sizeof(widget), "%s%s%s", colour_string, widget, reset_term_colours);

  return strdup(coloured_widget);
}


/**
 * Parses a given input prompt string, replacing any embedded widget
 * tokens with their corresponding values.
 *
 * This function scans the input string for embedded widget tokens,
 * which are denoted by the syntax `@{token}`. When a widget token is
 * found, it is replaced by the widget. If the token is unknown, it is
 * left unchanged in the output. The function ensures that the length
 * of the resulting prompt does not exceed `PROMPT_MAX_LEN`.
 *
 * @param unparsed_prompt The input prompt string containing
 *        embedded widget tokens to be parsed.
 *
 * @return A dynamically allocated string containing the digested
 *         prompt. If the resulting prompt would exceed
 *         `PROMPT_MAX_LEN`, a predefined error message is returned
 *         instead. The caller is responsible for freeing the returned
 *         string.
 */
const char *parse_prompt(const char *unparsed_prompt,
                         dictionary *wtoken_state_map,
                         struct WidgetConfig *defaults) {
  char prompt[PROMPT_MAX_LEN] = "";
  const char *ptr = unparsed_prompt;
  char wtoken[WIDGET_TOKEN_MAX_LEN];
  int index = 0;
  int inside_wtoken = 0;

  while (*ptr) {
    if (*ptr == '@' && *(ptr + 1) == '{') {
      inside_wtoken = 1;
      index = 0;
      ptr += 2; // Skip past the '@{'
    } else if (*ptr == '}' && inside_wtoken) {
      inside_wtoken = 0;
      wtoken[index] = '\0'; // Null-terminate the widget token

      // Look up the widget token and append its value to prompt
      const char *replacement = dictionary_get(wtoken_state_map, (const char *) to_lower(wtoken), NULL);
      if (replacement) {
        int is_active = is_widget_active(wtoken, replacement);
        const char *formatted_replacement = format_widget(wtoken, replacement, is_active, defaults);
        if(safe_strcat(prompt, formatted_replacement, PROMPT_MAX_LEN) == FAILURE) { goto error; }
      } else {
        // Token not found: put the widget token back, as is
        if(safe_strcat(prompt, "@{", PROMPT_MAX_LEN) == FAILURE) { goto error; }
        if(safe_strcat(prompt, wtoken, PROMPT_MAX_LEN) == FAILURE) { goto error; }
        if(safe_strcat(prompt, "}", PROMPT_MAX_LEN) == FAILURE) { goto error; }
      }
      ptr++; // Move past the '}'
    } else if (inside_wtoken) {
      // We are inside a widget token, accumulate characters
      wtoken[index++] = *ptr++;
    } else {
      // We are outside a widget token, copy character directly to prompt
      char str[2] = {*ptr++, '\0'};
      if(safe_strcat(prompt, str, PROMPT_MAX_LEN) == FAILURE) { goto error; }
    }
  }

  return strdup(prompt);

 error:
  return "PROMPT TOO LONG $ ";
}


int main(int argc, char *argv[]) {
  struct CurrentState state;
  struct ConfigRoot config;
  dictionary *wtoken_state_map = dictionary_new(DICTIONARY_MAX_SIZE);

  git_libgit2_init();
  initialise_state(&state);

  char *config_file_path = (argc > 1) ? argv[1] : NULL;
  int retval = handle_configuration(&config, config_file_path);
  if (retval != SUCCESS) {
    if (retval == ERROR_CUSTOM_INI_FILE_NOT_FOUND) {
      printf("USER-SPECIFIED INI FILE '%s' NOT FOUND\n$ ", config_file_path);
    }
    if (retval == ERROR_DEFAULT_INI_FILE_NOT_FOUND) {
      printf("INI FILE '.prompt2_config.ini' NOT FOUND IN $HOME OR '.'\n$ ");
    }
    if (retval == ERROR_INVALID_INI_FILE) {
      printf("INVALID INI FILE\n$ ");
    }
    return ERROR;
  }

  if (are_escape_sequences_properly_formed(config.default_prompt) != SUCCESS) {
    printf("MALFORMED DEFAULT_PROMPT $ ");
    return ERROR;
  }
  if (are_escape_sequences_properly_formed(config.git_prompt) != SUCCESS) {
    printf("MALFORMED GIT_PROMPT $ ");
    return ERROR;
  }


  /*
    Ok, now that the error checking is done, let's gather some info on the environment
  */
  gather_system_context(&state);
  gather_aws_context(&state);
  int is_git_repo = gather_git_context(&state);
  

  /*
    .. and figure out which prompt config to select

    There are three prompts to choose from:
    - git prompt: for use in normal git repos
    - zero git prompt: for use in newly initialized git repos, with no
      commits as of yet.
    - non-git-repo: the default to use in all other directories
  */
  char * selected_prompt;
  char * selected_cwd_type;
  if (is_git_repo == SUCCESS_IS_GIT_REPO) {
    if (state.ahead_num  == -1 && state.behind_num   == -1 &&
        state.staged_num == -1 && state.modified_num == -1) {
      selected_prompt = strdup(config.git_prompt_zero);
    }
    else {
      selected_prompt = strdup(config.git_prompt);
    }
    selected_cwd_type = strdup(config.git_prompt_cwd_type);
  }
  else if (is_git_repo == FAILURE_IS_NOT_GIT_REPO) {
    selected_prompt = strdup(config.default_prompt);
    selected_cwd_type = strdup(config.default_prompt_cwd_type);
  }
  else {
    printf("UNDEFINED STATE $ ");
    return ERROR;
  }

  
  // Connect states to widgets
  map_wtoken_to_state(wtoken_state_map, &state);

  // for tokenization on \n to work, we need to replace the string "\n" with a newline character.
  const char * unparsed_prompt = replace_literal_newlines(selected_prompt);
  free(selected_prompt);
  const char *prompt = parse_prompt(unparsed_prompt, wtoken_state_map, &config.defaults);
  int terminal_width = term_width() ?: DEFAULT_TERMINAL_WIDTH;

  char temp_prompt[PROMPT_MAX_LEN] = "";
  char *tokenized_prompt = strdup(prompt);
  char *line = strtok(tokenized_prompt, "\n");

  while (line != NULL) {
    // check if there are any widget tokens which aren't the expanding type
    if (has_nonexpanding_tokens(line) == SUCCESS) {
      line = (char *) parse_prompt(line, wtoken_state_map, &config.defaults);
    }

    // Expanding type 1:
    // if there is a CWD widget token, shorten the CWD to fit the
    // terminal (if it's long) then re-parse the line
    if (strstr(line, "@{CWD}")) {
      char* cwd = get_cwd(&state, selected_cwd_type);
      int cwd_length = strlen(cwd);
      //int WIDGET_TOKEN_CWD_LEN = 6; // length of "@{CWD}"
      int visible_prompt_length = cwd_length + count_visible_chars(line); //- WIDGET_TOKEN_CWD_LEN;

      if (visible_prompt_length > terminal_width) {
        int max_width = cwd_length - (visible_prompt_length - terminal_width);
        shorten_path(cwd, max_width);
      }
      dictionary_set(wtoken_state_map, "cwd", cwd);
      line = (char *) parse_prompt(line, wtoken_state_map, &config.defaults); // Re-parse the current line
    }

    // Expanding type 2:
    // if there are any SPC tokens, and if the prompt is smaller
    // than the terminal, then replace it with filler text (whitespace)
    //int WIDGET_TOKEN_SPC_LEN = 6; // length of "@{SPC}"
    int visible_prompt_length = count_visible_chars(line); //- WIDGET_TOKEN_SPC_LEN;
    if (strstr(line, "@{SPC}")) {
      if (visible_prompt_length < terminal_width) {
        int number_of_spaces = terminal_width - visible_prompt_length;
        const char *filler = spacefiller(number_of_spaces);
        dictionary_set(wtoken_state_map, "spc", strdup(filler));
        free((void *)filler);
        line = (char *) parse_prompt(line, wtoken_state_map, &config.defaults); // Re-parse the current line
      }
      else {
        remove_widget_token(line, "@{SPC}");
      }
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

  prompt = strdup(temp_prompt);
  free(tokenized_prompt);

  // Finally, print the prompt
  printf("%s", prompt);
  free((char *) prompt);

  
  /*
    Time to free up memory
  */
  if (config.dynamic_default_prompt) {
    free(config.default_prompt);
    free(config.default_prompt_cwd_type);
  }
  if (config.dynamic_git_prompt) {
    free(config.git_prompt);
    free(config.git_prompt_zero);
    free(config.git_prompt_cwd_type);
  }
  if (config.dynamic_widget_config) {
    free(config.defaults.string_active);
    free(config.defaults.string_inactive);
    free(config.defaults.colour_on);
    free(config.defaults.colour_off);
  }

  struct WidgetConfigMap *current2, *tmp2;
  HASH_ITER(hh, configurations, current2, tmp2) {
    HASH_DEL(configurations, current2);
    free(current2->config.string_active);
    free(current2->config.string_inactive);
    free(current2->config.colour_on);
    free(current2->config.colour_off);
    free(current2->name);
    free(current2);
  }

  cleanup_resources(&state);
  git_libgit2_shutdown();
  dictionary_del(wtoken_state_map);


  return 0;
}
