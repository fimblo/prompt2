#ifndef PROMPT2_UTILS_H
#define PROMPT2_UTILS_H

#include <uthash.h>

enum generic_return_values {
  SUCCESS =  0,
  FAILURE =  1,
  ERROR   = -1,
};


/* ========================================================
    Resources for manipulating hash tables
   ======================================================== */

struct TextHashMap {
  const char *key;      // key
  const char *value;  // value
  UT_hash_handle hh;        // makes this structure hashable
} ;
void text_hash_add(struct TextHashMap **instructions,
                 const char *command,
                 const char *replacement);
const char *text_hash_lookup(struct TextHashMap **instructions,
                           const char *command);
void free_instructions(struct TextHashMap **instructions);


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
