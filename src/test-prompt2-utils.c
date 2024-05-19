/*
 *  test-prompt2-utils.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "prompt2-utils.h"


int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <function_name> <arg1> [arg2]\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *function_name = argv[1];

  if (strcmp(function_name, "trim") == 0) {
    if (argc != 3) {
      fprintf(stderr, "trim function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    char *result = trim(argv[2]);
    printf("%s\n", result);
    free(result);
  }


  else if (strcmp(function_name, "spacefiller") == 0) {
    if (argc != 3) {
      fprintf(stderr, "spacefiller function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    int num_spaces = atoi(argv[2]);
    const char *result = spacefiller(num_spaces);
    printf("%s\n", result);
    free((void *)result);
  }


  else if (strcmp(function_name, "to_lower") == 0) {
    if (argc != 3) {
      fprintf(stderr, "to_lower function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    char *result = to_lower(argv[2]);
    printf("%s\n", result);
    free(result);
  }


  else if (strcmp(function_name, "safe_strcat") == 0) {
    if (argc != 5) {
      fprintf(stderr, "safe_strcat function requires 3 arguments.\n");
      return EXIT_FAILURE;
    }
    char target_string[256]; // Assuming a buffer size for demonstration
    strncpy(target_string, argv[2], sizeof(target_string) - 1);
    target_string[sizeof(target_string) - 1] = '\0'; // Ensure null-termination
    int max_len = atoi(argv[4]);
    int result = safe_strcat(target_string, argv[3], max_len);
    if (result == SUCCESS) {
      printf("%s\n", target_string);
    } else {
      printf("Concatenation failed due to length restrictions.\n");
      return EXIT_FAILURE;
    }
  }


  else if (strcmp(function_name, "truncate_with_ellipsis") == 0) {
    if (argc != 4) {
      fprintf(stderr, "truncate_with_ellipsis function requires 2 arguments.\n");
      return EXIT_FAILURE;
    }
    char str[256]; // Assuming a buffer size for demonstration
    strncpy(str, argv[2], sizeof(str) - 1);
    str[sizeof(str) - 1] = '\0'; // Ensure null-termination
    size_t max_width = (size_t)atoi(argv[3]);
    truncate_with_ellipsis(str, max_width);
    printf("%s\n", str);
  }


  else if (strcmp(function_name, "replace_literal_newlines") == 0) {
    if (argc != 3) {
      fprintf(stderr, "replace_literal_newlines function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    char *result = replace_literal_newlines(argv[2]);
    printf("%s\n", result);
    free(result);
  }


  else if (strcmp(function_name, "has_nonexpanding_tokens") == 0) {
    if (argc != 3) {
      fprintf(stderr, "has_nonexpanding_tokens function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    int result = has_nonexpanding_tokens(argv[2]);
    printf("%d\n", result);
  }


  else if (strcmp(function_name, "remove_widget_token") == 0) {
    if (argc != 4) {
      fprintf(stderr, "remove_widget_token function requires 2 arguments.\n");
      return EXIT_FAILURE;
    }
    char str[256]; // Assuming a buffer size for demonstration
    strncpy(str, argv[2], sizeof(str) - 1);
    str[sizeof(str) - 1] = '\0'; // Ensure null-termination
    remove_widget_token(str, argv[3]);
    printf("%s\n", str);
  }


  else if (strcmp(function_name, "shorten_path") == 0) {
    if (argc != 4) {
      fprintf(stderr, "shorten_path function requires 2 arguments.\n");
      return EXIT_FAILURE;
    }
    char path[256]; // Assuming a buffer size for demonstration
    strncpy(path, argv[2], sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0'; // Ensure null-termination
    int max_width = atoi(argv[3]);
    shorten_path(path, max_width);
    printf("%s\n", path);
  }


  else if (strcmp(function_name, "are_escape_sequences_properly_formed") == 0) {
    if (argc != 3) {
      fprintf(stderr, "are_escape_sequences_properly_formed function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    int result = are_escape_sequences_properly_formed(argv[2]);
    printf("%d\n", result);
  }


  else if (strcmp(function_name, "count_visible_chars") == 0) {
    if (argc != 3) {
      fprintf(stderr, "count_visible_chars function requires 1 argument.\n");
      return EXIT_FAILURE;
    }
    int count = count_visible_chars(argv[2]);
    printf("%d\n", count);
  }


  else if (strcmp(function_name, "term_width") == 0) {
    if (argc != 2) {
      fprintf(stderr, "term_width function does not require arguments.\n");
      return EXIT_FAILURE;
    }
    int width = term_width();
    printf("%d\n", width);
  }


  else {
    fprintf(stderr, "Function '%s' not recognized or not supported for testing.\n", function_name);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
