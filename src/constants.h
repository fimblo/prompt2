/**
 * This file contains constants and enums which are shared across all
 * the files in this directory. You can find constants/enums which are
 * specific to one file in that file. There are exceptions to this,
 * but for them, it can be argued that they are general enough to be
 * contained here.
 */
#ifndef ENUM_H
#define ENUM_H

enum generic_return_values {
  SUCCESS =  0,
  FAILURE =  1,
  ERROR   = -1,
};

enum file_related_return_values {
  ERROR_CUSTOM_INI_FILE_NOT_FOUND = -10,
  ERROR_DEFAULT_INI_FILE_NOT_FOUND = -11,
  ERROR_INVALID_INI_FILE = -12,
};



//note: Each of these max sizes are "famous last words" haha

/**
 *  The prompt can be 4 * PATH_MAX (4 * 4096)
 *
 *  This should be enough for many multi-line prompts with an absurd number of
 *  many escape characters
 */
#define PROMPT_MAX_LEN 16384


/**
 * How much space to reserve for a string with numbers in it.
 */
#define ITOA_BUFFER_SIZE       8


/**
 * This is how long a path this program can accept.
 */
#define PATH_MAX               4096

/**
 * The maximum length of a short string
*/
#define SHORT_STRING           16

#endif // ENUM_H
