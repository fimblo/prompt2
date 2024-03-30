#ifndef PROMPT2_UTILS_H
#define PROMPT2_UTILS_H

#include <uthash.h>


/* ========================================================
    Resources for manipulating the command hash table
   ======================================================== */

struct CommandMap {
  const char *command;      // key
  const char *replacement;  // value
  UT_hash_handle hh;        // makes this structure hashable
} ;
void add_command(struct CommandMap **instructions,
                 const char *command,
                 const char *replacement);
const char *lookup_command(struct CommandMap **instructions,
                           const char *command);
void free_instructions(struct CommandMap **instructions);

/**
 * make str lowercase
*/
void to_lower (char *str);


#endif //PROMPT2_UTILS_H
