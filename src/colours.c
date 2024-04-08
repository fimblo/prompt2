#ifdef __linux__
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#include <uthash.h>

#include "colours.h"
#include "prompt2-utils.h"

struct Colour {
  const char *name;
  int red;
  int green;
  int blue;
  UT_hash_handle hh;
} ;
static struct HashMap *colour_map = NULL;

