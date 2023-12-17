CC = gcc
CFLAGS = -Wall -Wextra
INCLUDE_DIR = /opt/homebrew/include
LIB_DIR = /opt/homebrew/lib
LIBS = -lgit2

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/test-functions
SOURCES = $(SRC_DIR)/git-status.c $(SRC_DIR)/test-functions.c
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all build run install install-local clean test help

all: build test

build: $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) -L$(LIB_DIR) $^ -o $@ $(LIBS)

run: build test
	@echo "Run: $(TARGET)"
	@echo "-------------------------"
	@$(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin

install-local: $(TARGET)
	install -m 755 $(TARGET) $(HOME)/bin

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)

test:
	bats test

help:
	@echo "Available targets:"
	@echo "  all           - Builds the program and runs tests"
	@echo "  build         - Compiles the sources and creates the executable"
	@echo "  run           - Builds, tests, and runs the program"
	@echo "  install       - Installs the executable to /usr/local/bin"
	@echo "  install-local - Installs the executable to ~/bin"
	@echo "  clean         - Removes object files and the executable"
	@echo "  test          - Runs tests using bats"
	@echo "  help          - Displays this help information"
