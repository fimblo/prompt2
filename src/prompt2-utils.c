#ifdef __linux__
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <uthash.h>

#include "prompt2-utils.h"


/* ========================================================
    Resources for manipulating the command hash table
   ======================================================== */


// Function to add entries to the hash table
void add_command(struct CommandMap **instructions,
                 const char *command,
                 const char *replacement) {
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
  HASH_ADD_KEYPTR(hh, *instructions, i->command, strlen(i->command), i);
}

// Function to find an entry in the hash table
const char *lookup_command(struct CommandMap **instructions,
                           const char *command) {
  struct CommandMap *i;
  char *command_lowercase = strdup(command);
  to_lower(command_lowercase);
  HASH_FIND_STR(*instructions, command_lowercase, i);
  return i ? i->replacement : NULL;
}

void free_instructions(struct CommandMap **instructions) {
  struct CommandMap *current1, *tmp1;
  HASH_ITER(hh, *instructions, current1, tmp1) {
    HASH_DEL(*instructions, current1);
    free((char*)current1->replacement);
    free(current1);
  }
}

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
