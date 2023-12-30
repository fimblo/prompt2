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

  char originalPath[MAX_PATH];
  strncpy(originalPath, path, MAX_PATH - 1); // copy up to MAX_PATH-1 characters of path into originalPath
  originalPath[MAX_PATH - 1] = '\0'; // if path was larger than originalPath, null-terminate it.

  /*
    Preserve the first part of the path
    Examples:
    ~/tmp/hello            -> preserve '~/tmp/'
    /home/fimblo/tmp/hello -> preserve '/home/fimblo/'

    This since the first bit of the path helps us understand where
    we are, we don't want to truncate that.
  */


  /*
    TODO
    Later, check if there is a tilde or plus in the zeroth position.
    If there is, copy it and use the remainder for the rest of the
    calculations. Then prepend it later when returning.
  */

  /*
    all examples in the margin below assume this path:
    0    0    1    1    2    2    3    3    4    4    5    5    6    
    0    5    0    5    0    5    0    5    0    5    0    5    0
    /home/fimblo/personal/writing/story_10/chapter_18/section_30   len 60    
    /home/fimblo/                                                  len 13
    personal/writing/story_10/chapter_18/section_30                len 47
  */


  char preservedPart[MAX_PATH] = {0};
  char *secondSlash = strchr(originalPath + 1, '/'); // '/home/' 
  char *remainder = originalPath;
  if (secondSlash) {
    char *thirdSlash = strchr(secondSlash + 1, '/'); // 'fimblo/'
    if (thirdSlash) {
      int preservedLength = thirdSlash - originalPath + 1;  // 13
      strncpy(preservedPart, originalPath, preservedLength);
      preservedPart[preservedLength] = '\0';
      remainder += preservedLength; // 'personal/writing/story_10/chapter_18/section_3'
    }
  }

  // Shorten the remaining part of the path
  len = strlen(remainder); // 47
  if (len > maxWidth) {
    char tempPath[MAX_PATH];
    char *lastSlash = strrchr(remainder, '/');

    // Special case: If the rest of the path is one long directory
    // name, or if it is only a slash at the beginning
    if (!lastSlash || lastSlash == remainder) {
      // truncate to fit this limit, and add space for '...'
      strncpy(tempPath, remainder, maxWidth - 3);
      tempPath[maxWidth - 3] = '\0';
      // tempPath will have 'personal/writing/story_10/cha'
      
    } else {
      int lastSegmentLen = strlen(lastSlash);
      if (lastSegmentLen > maxWidth - 3) {
        strncpy(tempPath, "...", 3);
        strncpy(tempPath + 3, lastSlash, maxWidth - 4);
        tempPath[maxWidth - 1] = '\0';
      } else {
        int frontLength = maxWidth - lastSegmentLen - 3; // Length available at the front
        strncpy(tempPath, "...", 3);
        memmove(tempPath + 3, remainder + len - lastSegmentLen - frontLength, frontLength);
        strcpy(tempPath + maxWidth - lastSegmentLen, lastSlash); // Use strcpy to include the null terminator
      }
    }

    // Concatenate the preserved part and the shortened part
    strncpy(path, preservedPart, MAX_PATH - 1);
    strncat(path, tempPath, MAX_PATH - strlen(path) - 1);
    path[MAX_PATH - 1] = '\0';
  } else {
    // If no shortening needed, concatenate the preserved part and remainder
    strncpy(path, preservedPart, MAX_PATH - 1);
    strncat(path, remainder, MAX_PATH - strlen(path) - 1);
    path[MAX_PATH - 1] = '\0';
  }
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
  printf("%s $ ", path);

  return 0;
}
