#ifndef PROMPT2_UTILS_H
#define PROMPT2_UTILS_H

#include <uthash.h>


/**
 * Return string with 'number_of_spaces" spaces.
 *
 * @param number_of_spaces The number of spaces to include in the string.
 * @return A pointer to a string containing the specified number of spaces.
 *         Don't forget to free this string!
 */
const char *spacefiller(int number_of_spaces);


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
 * exactly match "@{CWD}" or "@{SPC}", which both are tokens which can
 * have a variable length, decided after all the other widgets.
 *
 * @param str The input string to check for widget tokens.
 *
 * @return Returns SUCCESS if there is at least one non-expanding
 *   widget token; otherwise, FAILURE.
 */
int has_nonexpanding_tokens(const char *str);


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
void remove_widget_token(char *str, const char *token);


/* ========================================================
   Resources for manipulating strings
   ======================================================== */

/**
 * helper. Get terminal width.
 */
int term_width();


#endif //PROMPT2_UTILS_H
