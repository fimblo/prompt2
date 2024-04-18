
#ifdef __linux__
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <uthash.h>

#include "constants.h"
#include "prompt2-utils.h"



/* ========================================================
   Resources for manipulating strings
   ======================================================== */


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

int has_non_cwd_tokens(const char *str) {
  const int CWD_len = 5;

  const char *start;
  const char *end;
  while ((start = strstr(str, "@{")) != NULL) {
    end = strchr(start + 2, '}');
    if (end == NULL) {
      break;
    }
    if (end != start + CWD_len || strncmp(start + 2, "CWD", 3) != 0) {
      return SUCCESS;
    }
    str = end + 1;
  }
  return FAILURE;
}

 
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
