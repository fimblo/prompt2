/*
 *  test-term-attributes.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term-attributes.h"

int test_create_and_free_dict() {
  free_attribute_dict(create_attribute_dict());
  return 0;
}


int test_create_attribute_dict(char *key, char *value) {
  dictionary *dict = create_attribute_dict();
  dictionary_set(dict, key, value);
  const char *retrieved_value  = dictionary_get(dict, key, NULL);

  int retval = 1;
  if (retrieved_value) {
    printf("%s->%s\n", key, retrieved_value);
    retval = 0;
  }

  free_attribute_dict(dict);
  return retval;
}


int test_get_attribute_combo(char *combo) {
  dictionary *dict = create_attribute_dict();
  const char *result = get_attribute_combo(dict, combo);

  int retval = 1;
  if (result) {
    printf("%s\n", result);
    free((void *)result);
    retval = 0;
  }

  free_attribute_dict(dict);
  return retval;
}


int test_replace_attribute_tokens(char *string) {
  dictionary *dict = create_attribute_dict();
  const char *result = replace_attribute_tokens(string, dict);

  int retval = 1;
  if (result) {
    printf("%s\n", result);
    free((void *)result);
    retval = 0;
  }

  free_attribute_dict(dict);
  return retval;
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <function_name> <arg1> [arg2]\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *function_name = argv[1];

  if (strcmp(function_name, "create_attribute_dict") == 0) {
    if (argc != 4) {
      fprintf(stderr, "create_attribute_dict function requires 2 arguments.\n");
      return EXIT_FAILURE;
    }
    return test_create_attribute_dict(argv[2], argv[3]);
  }

  else if (strcmp(function_name, "get_attribute_combo") == 0) {
    if (argc != 3) {
      fprintf(stderr, "get_attribute_combo function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    return test_get_attribute_combo(argv[2]);
  }

  else if (strcmp(function_name, "replace_attribute_tokens") == 0) {
    if (argc != 3) {
      fprintf(stderr, "replace_attribute_tokens function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    return test_replace_attribute_tokens(argv[2]);
  }

  else {
    fprintf(stderr, "Function '%s' not recognized or not supported for testing.\n", function_name);
    return EXIT_FAILURE;
  }
}
