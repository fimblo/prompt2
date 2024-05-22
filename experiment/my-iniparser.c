
#include <errno.h>
#include <stdio.h>

#ifdef __unix__
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>
#include <linux/limits.h>
#elif __APPLE__
#include <dictionary.h>
#include <iniparser.h>
#include <sys/syslimits.h>
#include <unistd.h> // for access()
#else
#error "Unknown or unsupported OS"
#endif



char* replace_literal_newlines(const char* input) {
  int inputLen = strlen(input);

  char* result = malloc(inputLen + 1); // +1 for the null terminator
  if (!result) {
    perror("REPLACE LITERAL NEWLINES FAILURE $ ");
    exit(EXIT_FAILURE);
  }

  const char* current = input;
  char* output = result;
  while (*current) {
    if (*current == '\\' && *(current + 1) == 'n') {
      *output++ = '\n';
      current += 2;
    } else {
      *output++ = *current++;
    }
  }
  *output = '\0'; // Null-terminate

  return result;
}



int main(int argc, char *argv[]) {
  char *config_file_path = (argc > 1) ? argv[1] : NULL;
  if (! config_file_path) {
    printf("arg1: ini file\n");
    exit(1);
  }


  dictionary *ini = iniparser_load(config_file_path);
  if (ini == NULL) return 1;
  
  const char *var1;
  const char *var2;

  if (iniparser_find_entry(ini, "SECTION") != 1) {
    printf("Missing SECTION in ini-file.\n");
    exit(1);
  }

  var1 = iniparser_getstring(ini, "SECTION:var1", NULL);
  var2 = iniparser_getstring(ini, "SECTION:var2", NULL);

  if (var1 && var2) {
    printf("var1: '%s'\n", replace_literal_newlines(var1));
    printf("var2: '%s'\n", replace_literal_newlines(var2));
  }
  else {
    printf("missing sections var1 and/or var2\n");    
  }
  return 0;
}