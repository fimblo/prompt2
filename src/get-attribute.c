
#include <ctype.h>
#include <iniparser/dictionary.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attributes.h"
#include "prompt2-utils.h"
#include "term-attributes.h"

void usage() {
  printf("Usage:\n");
  printf(" get-attribute [option] <comma-separated attributes>\n");
  printf("\n");
  printf("Description:\n");
  printf(" get-attribute is a tool to retrieve terminal escape codes for various\n text styles and colors.\n");
  printf("\n");
  printf("Options:\n");
  printf(" --list-all          List all available attributes.\n");
  printf(" --list-styles       List all available text styles.\n");
  printf(" --list-col-8bit     List all available 8-bit colors.\n");
  printf(" --list-col-24bit    Instructions to create full RGB colors.\n");
  printf(" --list-col-names    List all available color names.\n");
  printf(" --list-reset        List all reset escape sequences.\n");
  printf(" --usage, --help, -h Show this usage information.\n");
  printf("\n");
  printf("Example: to get the terminal escape code '\\[\\e[1;34;47m\\]', run:\n");
  printf(" get-attribute 'bold;fg blue;bg white'\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage();
    exit(1);
  }
  //  printf("arg: %s\n", argv[1]);
  int exit_status=-1;
  
  const char *category = argv[1];

  if (strcmp(category, "--list-all") == 0) {
    for (size_t i = 0; attributes[i].name != NULL; i++) {
        printf("%s\n", attributes[i].name);
    }
    exit_status = 0;
  }
  else if (strcmp(category, "--list-styles") == 0) {
    for (size_t i = 9; i < 17; i++) {
        printf("%s\n", attributes[i].name);
    }
    exit_status = 0;
  }
  else if (strcmp(category, "--list-col-8bit") == 0) {
    // List 8-bit colors
    for (size_t i = 17; i < 81; i++) {
        printf("%s\n", attributes[i].name);
    }
    exit_status = 0;
  }
  else if (strcmp(category, "--list-col-24bit") == 0) {
    printf("Create your own full RGB colour by passing a string like so:\n");
    printf("  '{fg|bg}-rgb-{r};{g};{b}'\n\n");
    printf("where r, g, and b are in decimal notation. For example:\n");
    printf("  'fg-rgb-0;0;255' will make the foreground text blue.\n");
    exit_status = 0;
  }
  else if (strcmp(category, "--list-col-names") == 0) {
    // List xorg colors
    for (size_t i = 81; attributes[i].name != NULL; i++) {
        printf("%s\n", attributes[i].name);
    }
    exit_status = 0;
  }
  else if (strcmp(category, "--list-reset") == 0) {
    // List reset escape sequences
    for (size_t i = 0; i < 9; i++) {
        printf("%s\n", attributes[i].name);
    }
    
    exit_status = 0;
  }
  else if (strcmp(category, "--usage") == 0 ||
           strcmp(category, "--help")  == 0 ||
           strcmp(category, "-h") == 0) {
    // List usage
    usage();
    exit_status = 0;
  }
  else {
    dictionary *attribute_dict = create_attribute_dict();

    const char *seq = get_attribute_combo(attribute_dict, category);
    if (seq) {
      printf("%s\n", seq);
      exit_status=0;
    }


    // Free the dictionary
    free_attribute_dict(attribute_dict);
  }
  exit(exit_status);
}

