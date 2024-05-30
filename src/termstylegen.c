
#include <ctype.h>
#include <iniparser/dictionary.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prompt2-utils.h"

#include "term-attributes.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage:\n");
    printf(" get-terminal-escape-sequence <plaintext style and colour>\n");
    printf("\n");
    printf("Example: to get the terminal escape code '\\[\\e[1;34;47m\\]', run:\n");
    printf(" get-terminal-escape-sequence 'bold;fg blue;bg white'\n");
    exit(1);
  }
  //  printf("arg: %s\n", argv[1]);
  
  dictionary *attribute_dict = create_attribute_dict();

  int exit_status=-1;
  const char *seq = get_attribute_combo(attribute_dict, argv[1]);
  if (seq) {
    printf("%s\n", seq);
    exit_status=0;
  }


  // Free the dictionary
  free_attribute_dict(attribute_dict);
  exit(exit_status);
}

