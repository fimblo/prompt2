CC = gcc
CFLAGS = -Wall -Wextra
INCLUDE_DIR = /opt/homebrew/include
LIB_DIR = /opt/homebrew/lib
LIBS = -lgit2 -ljson-c -liniparser

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

TEST_TARGET = $(BIN_DIR)/test-get-status
TEST_SOURCES = $(SRC_DIR)/get-status.c $(SRC_DIR)/test-get-status.c
TEST_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

UTILS_SOURCE = $(SRC_DIR)/prompt2-utils.c
UTILS_OBJECT = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(UTILS_SOURCE))

PROMPT2_TARGET = $(BIN_DIR)/prompt2
PROMPT2_SOURCES = $(SRC_DIR)/get-status.c $(SRC_DIR)/prompt2.c
PROMPT2_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(PROMPT2_SOURCES)) $(UTILS_OBJECT) $(TERMSTYLEGEN_OBJECT)

TERMSTYLEGEN_TARGET = $(BIN_DIR)/termstylegen
TERMSTYLEGEN_SOURCE = $(SRC_DIR)/term_attributes.c $(UTILS_SOURCE)
TERMSTYLEGEN_OBJECT = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(TERMSTYLEGEN_SOURCE))

.PHONY: termstylegen

.PHONY: all build run install install-local clean test help

all: build test

build: $(TEST_TARGET) $(PROMPT2_TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) -L$(LIB_DIR) $^ -o $@ $(LIBS)

$(PROMPT2_TARGET): $(PROMPT2_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) -L$(LIB_DIR) $^ -o $@ $(LIBS)

run: build test
	@echo "Run: $(TEST_TARGET)"
	@echo "-------------------------"
	@$(TEST_TARGET)

install: $(PROMPT2_TARGET) $(TEST_TARGET)
	install -m 755 $(PROMPT2_TARGET) $(TEST_TARGET) /usr/local/bin

install-local: $(PROMPT2_TARGET) $(TEST_TARGET)
	install -m 755 $(PROMPT2_TARGET) $(TEST_TARGET) $(HOME)/bin

clean:
	rm -f $(BUILD_DIR)/*.o $(TEST_TARGET) $(PROMPT2_TARGET) $(TERMSTYLEGEN_TARGET)

test:
	bats test


termstylegen: $(TERMSTYLEGEN_TARGET)

$(TERMSTYLEGEN_TARGET): $(TERMSTYLEGEN_OBJECT)
	@mkdir -p $(BIN_DIR)
	$(CC) -L$(LIB_DIR) $^ -o $@ $(LIBS)


help:
	@echo "Available targets:"
	@echo "  all           - Builds the executables and runs tests"
	@echo "  build         - Compiles the sources and creates the executables"
	@echo "  run           - Builds, tests, and runs the program"
	@echo "  install       - Installs the executables to /usr/local/bin"
	@echo "  install-local - Installs the executables to ~/bin"
	@echo "  clean         - Removes object files and the executables"
	@echo "  test          - Runs tests using bats"
	@echo "  help          - Displays this help information"
