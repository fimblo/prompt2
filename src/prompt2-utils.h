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


/* ========================================================
    Resources for manipulating the command hash table
   ======================================================== */

/**
 * Make the string `str` lowercase
*/
void to_lower (char *str);


/**
 * Helper function.
 * Concatenates a string to git_prompt if the resulting length is within bounds.
 *
 */
int safe_strcat(char *target_string, const char *addition, int max_len);


/**
 * Cut off a string at length `max_width`, and replace the last
 * three characters with '...'
*/
void truncate_with_ellipsis(char *str, size_t max_width);


/**
 * Replaces occurrences of the two-character sequence '\' followed by 'n'
 * with the actual newline character '\n', in a given string.
 *
 * @param input The input string containing literal "\n" sequences.
 * @return A new string with "\n" sequences replaced by actual newline characters.
 *         The caller is responsible for freeing this string.
 */
char* replace_literal_newlines(const char* input);


/* ========================================================
    Resources for manipulating strings
   ======================================================== */

/**
 * helper. Get terminal width.
 */
int term_width();


#endif //PROMPT2_UTILS_H
