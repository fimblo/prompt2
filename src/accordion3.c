#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define MAX_PATH 4096

void shortenPath(char *path) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int maxWidth = (w.ws_col / 2) - 2;

    char originalPath[MAX_PATH];
    strncpy(originalPath, path, MAX_PATH - 1);
    originalPath[MAX_PATH - 1] = '\0';

    /*
      Preserve the first part of the path
      Examples:
      ~/tmp/hello            -> preserve '~/tmp/'
      /home/fimblo/tmp/hello -> preserve '/home/fimblo/'

      This since the first bit of the path helps us understand where
      we are.
    */
    
    char preservedPart[MAX_PATH] = {0};
    char *secondSlash = strchr(originalPath + 1, '/');
    char *remainder = originalPath;
    if (secondSlash) {
        char *thirdSlash = strchr(secondSlash + 1, '/');
        if (thirdSlash) {
            int preservedLength = thirdSlash - originalPath + 1;
            if (preservedLength < MAX_PATH) {
                strncpy(preservedPart, originalPath, preservedLength);
                preservedPart[preservedLength] = '\0';
                remainder += preservedLength; // Adjust the remainder pointer
            }
        }
    }

    // Shorten the remaining part of the path
    int len = strlen(remainder);
    if (len > maxWidth) {
        char tempPath[MAX_PATH];
        char *lastSlash = strrchr(remainder, '/');
        if (!lastSlash || lastSlash == remainder) {
            strncpy(tempPath, remainder, maxWidth - 3); // Truncate if no slash found
            tempPath[maxWidth - 3] = '\0';
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
