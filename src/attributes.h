#ifndef COLOR_ATTRIBUTES_H
#define COLOR_ATTRIBUTES_H

#include <stddef.h>

/**
 * Structure to hold an escape code mapping.
 */
struct EscapeCode {
  const char *name;
  const char *code;
};

extern struct EscapeCode attributes[];


#endif // COLOR_ATTRIBUTES_H
