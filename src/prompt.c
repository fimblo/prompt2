#include <git2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "get-status.h"

char *substitute(const char *text, const char *search, const char *replacement);




int main(void) {

  /*
  1. [x] check first in the code if we are in a gitrepo (cd .. look for .git)
  2. [x] If no, then print non-git prompt and exit
  3. [x] initialize git and run all info-gathering functions and update state
  4. [ ] check if ~/.aws/.. exists. If no, return
  5. [ ] run getAWSContext()

  Consider making all functions have the same signature.
  Consider merging CurrentState and RepoContext into one struct
     

  */
  git_libgit2_init();
  struct CurrentState state;
  initialiseState(&state);
  gatherGitContext(&state);

  state.repo_path = findGitRepositoryPath(".");
  printf("'%s'\n", state.repo_path);
  if (strlen(state.repo_path) == 0) {
    free((void *) state.repo_path);
    printf("$ ");
    return 0;
  }



  const char *undigestedPrompt = getenv("GP2_PROMPT") ?: "@{CWD.full}:@{Repo.name}\n$ \n";

  const char *instructions[][2] = {
    { "@{CWD.full}",  getCWDFull(&state)  },
    { "@{Repo.name}", state.repo_name     },
  };


  /*
    Later, instead of walking through the prompt for every
    instruction, instead walk through prompt and populate a new prompt
    for the instructions we find. This will be much less inefficient
   */
  char *prompt = strdup(undigestedPrompt);
  for (unsigned long i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
    prompt = substitute(prompt, instructions[i][0], instructions[i][1]);
  }

  printf("%s", prompt);
  free(prompt);

  cleanupResources(&state);
  git_libgit2_shutdown();
  return 0;
}



/**
 * Helper function that performs a string substitution operation.
 * It searches for occurrences of a 'search' string within a 'text'
 * string and replaces them with a 'replacement' string.
 *
 * @param text        The original string where substitutions should
 *                    be made.
 * @param search      The substring to look for within the 'text'.
 * @param replacement The string to replace 'search' with.
 * @return Returns a new string with all instances of 'search'
 *         replaced by 'replacement'.
 */
char *substitute(const char *text, const char *search, const char *replacement) {
  char *message = strdup(text);
  char *found = strstr(message, search);

  while (found) {
    size_t prefix_length = found - message;
    size_t suffix_length = strlen(found + strlen(search));

    size_t new_length = prefix_length + strlen(replacement) + suffix_length + 1;
    char *temp = malloc(new_length);  // Allocate temporary buffer for the new string

    strncpy(temp, message, prefix_length);
    temp[prefix_length] = '\0';

    strcat(temp, replacement);

    // If there's still a suffix, copy it to the new string
    if (suffix_length > 0) {
      strcat(temp, found + strlen(search));
    }

    free(message);  // Free the old 'message'
    message = temp;  // Set 'message' to the new string

    found = strstr(message, search);  // Search for the next occurrence
  }

  return message;
}
