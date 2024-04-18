#ifndef PROMPT2_UTILS_H
#define PROMPT2_UTILS_H

#include <uthash.h>


/**
 * Return lowercase version of const char *str
 */
char *to_lower(const char *str);


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


/**
 * Checks if the given string contains any widget tokens that do not
 * exactly match "@{CWD}".
 *
 * @param str The input string to check for widget tokens.
 *
 * @return Returns SUCCESS if there is at least one non-CWD widget token;
 *   otherwise, FAILURE.
 */
int has_non_cwd_tokens(const char *str);


/* ========================================================
   Resources for manipulating strings
   ======================================================== */

/**
 * helper. Get terminal width.
 */
int term_width();


#endif //PROMPT2_UTILS_H
