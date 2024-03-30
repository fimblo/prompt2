#ifdef __linux__
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#include <ctype.h>

#include "prompt2-utils.h"


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
