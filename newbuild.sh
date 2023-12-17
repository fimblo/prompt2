#!/bin/bash

INCLUDE_DIR=/opt/homebrew/include
LIB_DIR=/opt/homebrew/lib

# Setup/Clean
mkdir -p build
rm build/*o

echo Building...
gcc -Wall -Wextra -I$INCLUDE_DIR -c src/git-status.c -o build/git-status.o
gcc -Wall -Wextra -I$INCLUDE_DIR -c src/test-functions.c -o build/test-functions.o
gcc -L$LIB_DIR build/git-status.o build/test-functions.o -o bin/test-functions -lgit2

echo Run: build/test-functions
echo -------------------------
bin/test-functions
