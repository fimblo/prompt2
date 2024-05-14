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
 * Converts a string to lowercase.
 *
 * This function creates a new string that is a lowercase version of
 * the input string. The caller is responsible for freeing the
 * returned string.
 *
 * @param str The input string to convert to lowercase.
 * @return    A new string with all characters converted to lowercase,
 *            or NULL if the input is NULL.
 */
char *to_lower(const char *str);


/**
 * Safely concatenates a string to the target string if the resulting
 * length is within bounds.
 *
 * This function appends the addition string to the target string,
 * ensuring that the total length does not exceed the specified
 * maximum length. If the concatenation would exceed the maximum
 * length, the function returns FAILURE without modifying the target
 * string.
 *
 * @param target_string The target string to which the addition will
 *                 be appended.
 * @param addition The string to append to the target string.
 * @param max_len  The maximum allowed length of the resulting string.
 * @return SUCCESS if the concatenation is successful, FAILURE if it
 *                 would exceed max_len.
 */
int safe_strcat(char *target_string, const char *addition, int max_len);


/**
 * Truncates a string to a specified maximum width and appends an
 * ellipsis.
 *
 * This function shortens the input string to the specified maximum
 * width and replaces the last three characters with '...'. If the
 * string is shorter than or equal to the maximum width, it remains
 * unchanged.
 *
 * @param str       The string to truncate. Must be null-terminated.
 * @param max_width The maximum allowed width of the string,
 *                  including the ellipsis.
 */
void truncate_with_ellipsis(char *str, size_t max_width);


/**
 * Replaces occurrences of the two-character sequence '\' followed by
 * 'n' with the actual newline character '\n', in a given string.
 *
 * @param input The input string containing literal "\n" sequences.
 * @return      A new string with "\n" sequences replaced by actual newline
 *              characters. The caller is responsible for freeing this
 *              string.
 */
char* replace_literal_newlines(const char* input);


/**
 * Checks if a string contains any non-expanding widget tokens.
 *
 * This function searches for specific widget tokens (e.g., @{CWD} and
 * @{SPC}) in the input string. If it finds any tokens that do not
 * match these specific types, it returns SUCCESS. Otherwise, it
 * returns FAILURE.
 *
 * @param str       The input string to check for non-expanding widget
 *                  tokens.
 * @return SUCCESS  if non-expanding tokens are found, FAILURE otherwise.
 */
int has_nonexpanding_tokens(const char *str);


/**
 * Removes all occurrences of a specified token from a string.
 *
 * This function searches for the token in the string and, if found,
 * removes it by shifting the remainder of the string over the token's
 * position. It continues to search and remove subsequent occurrences
 * of the token. The original string is modified in place.
 *
 * @param str The string from which the token will be removed. Must be
 *            null-terminated.
 * @param token The token to be removed from the string. Must be
 *              null-terminated.
 */
void remove_widget_token(char *str, const char *token);


/**
 * Shortens a filesystem path to a specified maximum width by
 * truncating the beginning of the string.
 *
 * This function shortens the input path to the specified maximum
 * width and prepends an ellipsis ('...'). If the path is shorter than
 * or equal to the maximum width, it remains unchanged.
 *
 * @param original_path The filesystem path to shorten. Must be
 *                      null-terminated.
 * @param max_width The maximum allowed width of the path, including
 *                  the ellipsis.
 */
void shorten_path(char *original_path, int max_width);


/**
 * Checks if the escape sequences within a string are properly formed.
 *
 * This function validates that escape sequences in the input string
 * are correctly formed. It ensures that bracketed sequences and
 * escape sequences are properly opened and closed.
 *
 * @param mystring The input string to check for properly formed
 *                 escape sequences.
 * @return SUCCESS if all escape sequences are properly formed, ERROR
 *                 otherwise.
 */
int are_escape_sequences_properly_formed(const char *mystring);


/**
 * Counts the number of visible (non-escape sequence) characters in a
 * string.
 *
 * This function iterates through the input string and counts
 * characters that are not part of escape sequences or widget tokens,
 * providing the total number of visible characters.
 *
 * @param mystring The input string to count visible characters in.
 * @return The number of visible characters in the input string.
 */
int count_visible_chars(const char *mystring);


/* ========================================================
   Resources for manipulating strings
   ======================================================== */

/**
 * Gets the width of the terminal.
 *
 * This function retrieves the current width of the terminal window in
 * columns. It checks the width of stderr, as stderr is less likely to
 * be piped or redirected.
 *
 * @return The width of the terminal in columns, or 0 if an error occurs.
 */
int term_width();


#endif //PROMPT2_UTILS_H
