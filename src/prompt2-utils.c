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

#include "prompt2-utils.h"


/* ========================================================
    Resources for manipulating hash tables
   ======================================================== */


// Function to add entries to the hash table
void text_hash_add(struct TextHashMap **hash_map,
                 const char *key,
                 const char *value) {
  struct TextHashMap *i = malloc(sizeof(struct TextHashMap));
  if (i == NULL) {
    printf("HASH INSERT FAIL (malloc) $ ");
    exit(EXIT_FAILURE);
  }
  i->key = key;
  i->value = strdup(value);
  if (i->value == NULL) {
    free(i);
    printf("HASH INSERT FAIL (null value) $ ");
    exit(EXIT_FAILURE);
  }
  HASH_ADD_KEYPTR(hh, *hash_map, i->key, strlen(i->key), i);
}

// Function to find an entry in the hash table
const char *text_hash_lookup(struct TextHashMap **hash_map,
                        const char *key) {
  struct TextHashMap *i;
  char *key_lowercase = strdup(key);
  to_lower(key_lowercase);
  HASH_FIND_STR(*hash_map, key_lowercase, i);
  return i ? i->value : NULL;
}

void free_instructions(struct TextHashMap **hash_map) {
  struct TextHashMap *current, *tmp1;
  HASH_ITER(hh, *hash_map, current, tmp1) {
    HASH_DEL(*hash_map, current);
    free((char*)current->value);
    free(current);
  }
}

/* ========================================================
    Resources for manipulating strings
   ======================================================== */


/**
 * make str lowercase
*/
void to_lower (char *str) {
  if (str) {
    while (*str) {
      *str = tolower((unsigned char) *str);
      str++;
    }
  }
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
    return -1;
  }
  return (int) w.ws_col;
}