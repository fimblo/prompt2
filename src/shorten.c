#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
//#include <sys/ioctl.h>

#define MAX_PATH 4096

/*
    all examples in the margin below assume this path:
    0    0    1    1    2    2    3    3    4    4    5    5    6    
    0    5    0    5    0    5    0    5    0    5    0    5    0
    /home/fimblo/personal/writing/story_10/chapter_18/section_30   len 60    
    /home/fimblo/                                                  len 13
    personal/writing/story_10/chapter_18/section_30                len 47
*/

void truncatePath(char *originalPath, int maxWidth) {

  // Sanity checks
  int originalPathLen = strlen(originalPath);
  if (originalPathLen <= maxWidth) return;
  if (maxWidth < 3) {
    for (int i = 0; i < maxWidth; i++) originalPath[i] = '.';
    originalPath[3] = '\0';
    return;
  }

  // How much to shrink?
  int shrinkage = originalPathLen - maxWidth;
  if (shrinkage <= 3) {
    shrinkage = 3; // for the ellipsis
  }
  else {
    shrinkage += 3;
  }

  // Shrink it
  char rebuildPath[originalPathLen + 1];
  strcpy(rebuildPath, "...");
  strncat(rebuildPath,
          originalPath + shrinkage,     // forward shrinkage chars
          originalPathLen - shrinkage); // copy this many chars


  strcpy(originalPath, rebuildPath);
}





void accordionPath(char *originalPath, int maxWidth) {

  // Sanity checks
  int originalPathLen = strlen(originalPath);
  if (originalPathLen <= maxWidth) return;
  if (maxWidth < 3) {
    for (int i = 0; i < maxWidth; i++) originalPath[i] = '.';
    originalPath[3] = '\0';
    return;
  }

  
  // tmp vars as my workbench, these will be messed with
  char tmpPath[originalPathLen];
  int  tmpPathLength = originalPathLen;

  // As I step through the directories, I place dirs (short or long)
  // here after I decide what to do.
  char rebuildPath[originalPathLen];
  memset(rebuildPath, '\0', originalPathLen);

  // store the first char if it's a special char.
  if (originalPath[0] == '~' || originalPath[0] == '+') {
    rebuildPath[0] = originalPath[0];
    strcpy(tmpPath, originalPath + 1);
  } else {
    strcpy(tmpPath, originalPath);
  }

  // use tokenizer to walk through the dir levels
  char  lastToken[originalPathLen];  // for the dir the user is standing in, we want to preserve that
  int   shortDirWasUsed;
  char *token = strtok(tmpPath, "/"); 
  while (token != NULL) { 
    int shortDirLength = 3; // +1 for slash, +1 for terminator
    char shortDir[shortDirLength];
    sprintf(shortDir, "/%c", token[0]);

    int longDirLength = strlen(token) + 2; // +1 for slash, +1 for terminator ill be back
    char longDir[longDirLength];
    snprintf(longDir, (size_t) longDirLength, "/%s", token);

    int shinkage = longDirLength - shortDirLength;
    if (tmpPathLength >= maxWidth) {
      tmpPathLength = tmpPathLength - shinkage;
      strcat(rebuildPath, shortDir);
      shortDirWasUsed = 1;
    }
    else {
      strcat(rebuildPath, longDir);
      shortDirWasUsed = 0;
    }
    
    strcpy(lastToken, token);
    token = strtok(NULL, "/"); 
  }

   /*
    If the last directory added to rebuildPath was a shortDir, replace
    it with the full directory name, since I want that behaviour
   */
  if (shortDirWasUsed) {
    char *lastSlash = strrchr(rebuildPath, '/');
    if (lastSlash != NULL) {
        strcpy(lastSlash + 1, lastToken);
    }
  }


  strcpy(originalPath, rebuildPath);
}









int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <originalPath>\n", argv[0]);
    return 1;
  }

  char path1[MAX_PATH];
  strncpy(path1, argv[1], MAX_PATH - 1);
  path1[MAX_PATH - 1] = '\0';
  accordionPath(path1, atoi(argv[2]));
  truncatePath(path1, atoi(argv[2]));
  printf("%s\n", path1);

  char path2[MAX_PATH];
  strncpy(path2, argv[1], MAX_PATH - 1);
  path2[MAX_PATH - 1] = '\0';
  truncatePath(path2, atoi(argv[2]));
  printf("%s\n", path2);

  return 0;
}
