#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define MAX_PATH 4096

void shortenPath(char *path) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  int maxWidth = (w.ws_col / 3) - 2; // Path should occupy roughly 1/3 of terminal

  int len = strlen(path);
  if (len <= maxWidth) {
    return;
  }

  // store the first char if it's a special char.
  char specialChar = '\0';
  char cleanPath[strlen(path)];
  if (path[0] == '~' || path[0] == '+') {
    specialChar = path[0];
    strcpy(cleanPath, path + 1);
  } else {
    strcpy(cleanPath, path);
  }


  /*
    If there is no second slash, then we have a ginormous directory
   name, probably in the homedir. Truncate the end and we're done
  */
  char firstDir[strlen(cleanPath) + 1];
  char *secondSlash = strchr(cleanPath + 1, '/');
  if (secondSlash == NULL) {
    int truncateLength = maxWidth - 3 - (specialChar != '\0' ? 1 : 0);
    cleanPath[truncateLength] = '\0';
    strcat(cleanPath, "...");

    if (specialChar != '\0') {
      strcpy(path + 1, cleanPath);
    } else {
      strcpy(path, cleanPath);
    }
    return;
  }

  
  /*
    Now we know that cleanPath has at least one directory level in it.
    Let's save the first one, since it's helpful for spacial
    navigation purposes.
   */
  int lengthOfFirstDir = secondSlash - cleanPath + 1;
  strncpy(firstDir, cleanPath, lengthOfFirstDir);
  firstDir[lengthOfFirstDir] = '\0';


  char remainderPath[strlen(cleanPath) - lengthOfFirstDir + 1];
  strcpy(remainderPath, cleanPath + lengthOfFirstDir);
  
  int remainderMaxWidth = maxWidth - ( lengthOfFirstDir + (specialChar != '\0' ? 1 : 0) );
  
  /*
    Let's save the last directory name as well.
  */
  char *lastSlash = strrchr(remainderPath, '/');
  char lastDir[strlen(remainderPath) + 1];

  if (lastSlash == NULL) {
    // If there's no last slash, the entire remainderPath is the last directory

    // truncate remainderPath
    int truncateLength = remainderMaxWidth - 3;
    remainderPath[truncateLength] = '\0';
    strcat(remainderPath, "...");

    // rebuild path and return
    path[0] = '\0';
    if (specialChar != '\0') {
      path[0] = specialChar;
      path[1] = '\0';
      strcat(path, firstDir);
    } else {
      strcpy(path, firstDir);
    }
    strcat(path, remainderPath);
    return;
  }
  strcpy(lastDir, lastSlash + 1);
  *lastSlash = '\0'; // remainderPath is now shorter

  
  /*
    Now we tokenise all the in-between directory levels
   */
  int remainderLength = strlen(remainderPath);
  char accordionPath[remainderLength + 1];
  accordionPath[0] = '\0';

  char *token = strtok(remainderPath, "/"); 
  while (token != NULL) { 
    if (remainderLength - (int) strlen(token) + 2 >= remainderMaxWidth) {
      char firstChars[3]; 
      firstChars[0] = token[0];
      firstChars[1] = '/';
      firstChars[2] = '\0';
      strcat(accordionPath, firstChars);
      remainderLength = remainderLength - (strlen(token) + 2);
    }
    else {
      char tokenWithSlash[strlen(token) + 2];
      tokenWithSlash[0] = '\0';
      strcat(tokenWithSlash, token);
      strcat(tokenWithSlash, "/");
      strcat(accordionPath, tokenWithSlash);
    }
    token = strtok(NULL, "/"); 
  }
  

  // rebuild path and return
  path[0] = '\0';
  if (specialChar != '\0') {
    path[0] = specialChar;
    path[1] = '\0';
    strcat(path, firstDir);
  } else {
    strcpy(path, firstDir);
  }
  strcat(path, accordionPath);
  return;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <path>\n", argv[0]);
    return 1;
  }

  char path[MAX_PATH];
  strncpy(path, argv[1], MAX_PATH - 1);
  path[MAX_PATH - 1] = '\0';

  shortenPath(path);
  printf("'%s'\n", path);

  return 0;
}
