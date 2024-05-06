
#ifdef __linux__
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uthash.h>

#include "constants.h"
#include "prompt2-utils.h"



/* ========================================================
   Resources for manipulating strings
   ======================================================== */

/**
 * Return string with 'number_of_spaces" spaces.
 *
 * @param number_of_spaces The number of spaces to include in the string.
 * @return A pointer to a string containing the specified number of spaces.
 *         Don't forget to free this string!
 */
const char *spacefiller(int number_of_spaces) {
  if (number_of_spaces < 0) return NULL;

  char *space_str = malloc(number_of_spaces + 1);
  if (!space_str) {
    perror("malloc error in function 'filler'");
    exit(EXIT_FAILURE);
  }

  memset(space_str, ' ', number_of_spaces);
  space_str[number_of_spaces] = '\0';

  return space_str;
}


/**
 * make str lowercase
 */
char * to_lower (const char *str) {
  if (str == NULL) return NULL;

  char * lc = strdup(str);
  if (lc) {
    char *orig_lc = lc;
    while (*lc) {
      *lc = tolower((unsigned char) *lc);
      lc++;
    }
    lc = orig_lc;
  }
  return lc;
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
 * Cut off a string at length `max_width`, and replace the last
 * three characters with '...'
 */
void truncate_with_ellipsis(char *str, size_t max_width) {
  if (str && strlen(str) > max_width) {
    strcpy(&str[max_width - 3], "...");
  }
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


int has_nonexpanding_tokens(const char *str) {
  // two types of expanding widget tokens:
  // - CWD
  // - SPC
  const int token_len = 5;
  
  const char *start;
  const char *end;
  while ((start = strstr(str, "@{")) != NULL) {
    end = strchr(start + 2, '}');
    if (end == NULL) {
      break;
    }
    if ((end != start + token_len) ||
        (strncmp(start + 2, "CWD", 3) != 0
          &&
        strncmp(start + 2, "SPC", 3) != 0)) {
      return SUCCESS;
    }
    str = end + 1;
  }
  return FAILURE;
}


/**
 * Removes all occurrences of a specified token from a string.
 *
 * This function searches for the token in the string and, if found, removes it by
 * shifting the remainder of the string over the token's position. It continues to
 * search and remove subsequent occurrences of the token. The original string is
 * modified in place.
 *
 * @param str The string from which the token will be removed. Must be null-terminated.
 * @param token The token to be removed from the string. Must be null-terminated.
 */
void remove_widget_token(char *str, const char *token) {
  size_t token_len = strlen(token);
  char *token_pos;

  // Continue to search for the token until it's no longer found
  while ((token_pos = strstr(str, token)) != NULL) {
    // Move the rest of the string to overwrite the token
    memmove(token_pos, token_pos + token_len, strlen(token_pos + token_len) + 1);
  }
}


/**
 * Shortens a filesystem path to a specified maximum width by
 * truncating the beginning of the string
 */
void shorten_path(char *original_path, int max_width) {

  // Sanity checks
  int original_path_len = strlen(original_path);
  if (original_path_len <= max_width) return;
  if (max_width < 3) {
    for (int i = 0; i < max_width; i++) original_path[i] = '.';
    original_path[3] = '\0';
    return;
  }

  // How much to shrink?
  int shrinkage = original_path_len - max_width;
  if (shrinkage <= 3) {
    shrinkage = 3; // for the ellipsis
  }
  else {
    shrinkage += 3;
  }

  // Shrink it
  char rebuild_path[original_path_len + 1];
  strcpy(rebuild_path, "...");
  strncat(rebuild_path,
          original_path + shrinkage,     // forward shrinkage chars
          original_path_len - shrinkage); // copy this many chars

  strcpy(original_path, rebuild_path);
}


/**
 * Checks if the escape sequences within a string are properly formed.
 */
int are_escape_sequences_properly_formed(const char *mystring) {
  int in_escape_sequence = 0;
  int in_bracket_sequence = 0;

  while (*mystring) {

    // first two conditionals to deal with opening and closing brackets
    if (*mystring == '\\' && *(mystring + 1) == '[') {
      // Found \[
      if (in_bracket_sequence) {
        fprintf(stderr, "Improperly formed - found a new opening bracket before closing the previous one\n");
        return ERROR;
      }
      in_bracket_sequence = 1;
      mystring++; // Skip the next character as it is part of the escape sequence
    } else if (*mystring == '\\' && *(mystring + 1) == ']') {
      // Found \]
      if (!in_bracket_sequence) {
        fprintf(stderr, "Improperly formed - found a closing bracket without a matching opening bracket\n");
        return ERROR;
      }
      in_bracket_sequence = 0;
      mystring++; // Skip the next character as it is part of the escape sequence
    }

    // Remaining conditionals to deal with escape sequences
    else if (*mystring == '\\' &&
             ((*(mystring + 1) == '0' && *(mystring + 2) == '3' && *(mystring + 3) == '3') ||
              *(mystring + 1) == 'e')) {
      // Found \033 or \e, which should be within brackets
      if (!in_bracket_sequence) {
        fprintf(stderr, "Improperly formed - found an escape sequence outside of brackets\n");
        return ERROR;
      }
      in_escape_sequence = 1;
      mystring += (*mystring + 1 == 'e') ? 1 : 3; // Skip the next characters as they are part of the escape sequence
    } else if (*mystring == 'm' && in_escape_sequence) {
      // Found m - which should close an escape sequence
      if (!in_bracket_sequence) {
        fprintf(stderr, "Improperly formed - found closing of escape sequence outside of brackets\n");
        return ERROR;
      }
      in_escape_sequence = 0;
    }
    mystring++;
  }

  // If we're still in an escape sequence or in a bracket sequence at the end, it's improperly formed
  return in_escape_sequence == 0 && in_bracket_sequence == 0 ? SUCCESS : ERROR;
}


/**
 * Counts the number of visible (non-escape sequence) characters in a string.
 */
int count_visible_chars(const char *mystring) {
  int real_chars = 0;
  int in_bracket_sequence = 0;
  int in_widget_token = 0;
  while (*mystring) {
    if (!in_widget_token && *mystring == '\\' && *(mystring + 1) == '[') {
      // Found the start of a bracketed sequence
      in_bracket_sequence = 1;
      mystring++; // Skip the '[' character
    } else if (in_bracket_sequence && *mystring == '\\' && *(mystring + 1) == ']') {
      // Found the end of a bracketed sequence
      in_bracket_sequence = 0;
      mystring++; // Skip the ']' character
    } else if (!in_bracket_sequence && *mystring == '@' && *(mystring + 1) == '{') {
      // Found the start of a widget token
      in_widget_token = 1;
    } else if (in_widget_token && *mystring == '}') {
      // Found the end of a widget token
      in_widget_token = 0;
    } else if (!in_bracket_sequence && !in_widget_token) {
      // Count the character if it's not within a bracketed sequence or a widget token
      real_chars++;
    }
    mystring++;
  }

  return real_chars;
}

/* ========================================================
  Cookies

  A cookie jar is a directory:  ~/.config/prompt2-cookie-jar

  A cookie is an empty file stored in the cookie jar directory,
  where the filename itself contains the data in question.

  A cookie has a retention-time of 5 minutes; after this point,
  it will be refreshed.

  ======================================================== */

/**
 * Creates the directory ~/.config/prompt2-cookie-jar if it does not already exist.
 * Additionally, ensures that the ~/.config directory exists and creates it if necessary.
 * 
 * @return int Returns 0 on success, or -1 if an error occurred.
 */
int create_cookie_jar() {
    const char *home = getenv("HOME");
    if (!home) {
        return ERROR;
    }

    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.config", home);
    struct stat config_stat = {0};
    if (stat(config_path, &config_stat) == -1) {
        if (mkdir(config_path, 0700) == -1) {
            return ERROR;
        }
    }

    char cookie_jar_path[512];
    snprintf(cookie_jar_path, sizeof(cookie_jar_path), "%s/.config/prompt2-cookie-jar", home);
    struct stat st = {0};
    if (stat(cookie_jar_path, &st) == -1) {
        if (mkdir(cookie_jar_path, 0700) == -1) {
            return ERROR;
        }
    }

    return SUCCESS; // Successfully created or already exists
}

// widget_name: aws
// content: 2024-03-12T12:22:33Z
// results in a file: ~/.config/prompt2-cookie-jar/aws--2024-03-12T12~22~33Z
// note that colons should be auto-replaced with the tilde, stupid macos
// finally, filename is updated with the full path to the file in question
int save_cookie(const char *widget_name, const char *content, const char *filename);

int delete_cookie_by_filename(const char *filename);
int delete_widget_cookies(const char *widget_name);

int find_cookie(const char *widget_name, const char *content, const char *filename);


/* ========================================================
   Other resources
   ======================================================== */

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
    return 0;
  }
  return (int) w.ws_col;
}
