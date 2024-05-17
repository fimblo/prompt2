#ifndef TERM_ATTRIBUTES_H
#define TERM_ATTRIBUTES_H

#include <iniparser/dictionary.h>

/**
 * Structure to hold an escape code mapping.
 */
struct EscapeCode {
  const char *name;
  const char *code;
};


/**
 * Creates and initializes the escape code dictionary.
 * 
 * This function allocates memory for a dictionary and populates it
 * with all the predefined escape codes for terminal color and style
 * settings.
 * 
 * @return A pointer to the newly created dictionary
 */
dictionary *create_escape_code_dict();


/**
 * Frees the memory allocated for the dictionary.
 * 
 * @param dict A pointer to the dictionary to be freed
 */
void free_escape_code_dict(dictionary *dict);


/**
 * Generates the escape sequence for a given combination of escape
 * codes.
 * 
 * @param escape_code_dict The dictionary containing the escape codes
 * @param combo The combination of escape codes in the format "part1;part2;part3"
 * @return The escape sequence string
 */
const char *get_escape_combo(dictionary *escape_code_dict, const char* combo);



const char *replace_attribute_tokens(const char *string, dictionary *attribute_dict);

#endif /* TERM_ATTRIBUTES_H */
