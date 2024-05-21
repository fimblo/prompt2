#ifdef __linux__
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __unix__
#include <iniparser/dictionary.h>
#elif __APPLE__
#include <dictionary.h>
#else
#error "Unknown or unsupported OS"
#endif
#include "constants.h"
#include "attributes.h"
#include "prompt2-utils.h"
#include "term-attributes.h"



/**
 * Creates and initialises the escape code dictionary
 * 
 * This function allocates memory for a dictionary and populates it
 * with all the predefined escape codes for terminal colour and style
 * settings.
 * 
 * @return A pointer to the newly created dictionary
*/
dictionary *create_attribute_dict() {
  dictionary *dict = dictionary_new((size_t) 2048);
  for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); i++) {
    dictionary_set(dict, attributes[i].name, attributes[i].code);
  }
  return dict;
}


/**
 * Frees the memory allocated for the attribute dictionary
 * 
 * @param dict A pointer to the dictionary to be freed
*/
void free_attribute_dict(dictionary *dict) {
  dictionary_del(dict);
}


void _join_sequence(char *result, size_t result_size, char *sequences[], size_t count) {
  size_t pos = 0;
  for (size_t i = 0; i < count; i++) {
    int written = snprintf(result + pos,
                           result_size - pos,
                           "%s%s",
                           sequences[i],
                           (i < count - 1) ? ";" : "");
    if (written < 0) {
      result = strdup("Error in sequence");
      break;
    }
    if ((size_t)written >= result_size - pos) {
      // truncation
      result = strdup("Sequence truncated");
    }
    pos += written;
  }
}

/**
 * Retrieves the escape code for a style/colour attribute combo
 * 
 * This function searches the attribute dictionary for the escape
 * sequence corresponding to the specified combo of text styles and
 * colours.
 * 
 * @param attribute_dict A pointer to the dictionary of attributes
 * @param combo A comma-separated string of names of styles and
 *              colours
 * @return a const char pointer to the escape sequence, or NULL in the
 *         case of an error
*/
const char * get_attribute_combo(dictionary *attribute_dict, const char* combo) {
  // combo is "part1,part2,part3"
  char *str = strdup(combo);
  if (str == NULL) {
    return NULL;
  }

  char *token;
  char *rest = str;
  char *sequences[16]; // TODO: magic number
  int i = 0;
  while ((token = strtok_r(rest, ",", &rest))) {
    char *trimmed_token = trim(token);
    const char * code = dictionary_get(attribute_dict,
                                       (const char*) to_lower((const char*) trimmed_token),
                                       NULL);
    free(trimmed_token);
    if (code == NULL) {
      return NULL;
    }
    sequences[i++] = (char *) code;
  }

  char attribute_sequence[32*16]; // TODO: magic number
  _join_sequence(attribute_sequence, sizeof(attribute_sequence), sequences, i);
  char combo_result[sizeof(attribute_sequence)+8];
  snprintf(combo_result, sizeof(combo_result), "\\[\\e[%sm\\]", attribute_sequence );
  return (const char *) strdup(combo_result);
}

const char *replace_attribute_tokens(const char *string, dictionary *attribute_dict) {
    // Estimate the size of the result string
    size_t result_size = PROMPT_MAX_LEN; 
    char *result = malloc(result_size);
    if (!result) return string; // Failed to allocate memory, return string as-is

    const char *current = string;
    char *result_ptr = result;

    while (*current) {
        if (strncmp(current, "%{", 2) == 0) {
            char *end = strstr(current, "}");
            if (!end) break; // No closing brace found, break out of the loop

            size_t attr_len = end - current - 2;
            char *attr = strndup(current + 2, attr_len);

            const char *escape_seq;
            if (strlen(attr) == 0) {
              escape_seq = "\\[\\e[0m\\]";
            }
            else {
              escape_seq = get_attribute_combo(attribute_dict, attr);
            }

            if (escape_seq) {
              // Copy escape sequence to result
              strcpy(result_ptr, escape_seq);
              result_ptr += strlen(escape_seq);
            }

            free(attr);
            current = end + 1; // Move past the processed token
        } else {
            *result_ptr++ = *current++; // Copy current character and move to the next
        }
    }

    *result_ptr = '\0'; // Null-terminate the result string
    return result;
}
