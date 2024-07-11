# Instructions when adding instructions to build a new binary:
# - Add the name of the new binary to the list BINARIES
# - Add a Link section for the binary.

# Initial Configuration
CC = gcc
CFLAGS = -Wall -Wextra
INCLUDE_DIR = /opt/homebrew/include
LIB_DIR = /opt/homebrew/lib
LIBS = -lgit2 -ljson-c -liniparser

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Find all C source files and define object file paths
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Binaries to build
BINARIES = $(BIN_DIR)/prompt2 $(BIN_DIR)/get-term-esc-sequence $(BIN_DIR)/test-get-status $(BIN_DIR)/test-prompt2-utils $(BIN_DIR)/test-term-attributes

# Phony Targets
.PHONY: all clean build install-local test

# Main Targets
all: build test

# Build Targets
build: $(BINARIES)

# Compile Source Files to Object Files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "\nCompiling $< to $@"
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Link prompt2
$(BIN_DIR)/prompt2: $(BUILD_DIR)/prompt2.o $(BUILD_DIR)/prompt2-utils.o $(BUILD_DIR)/term-attributes.o $(BUILD_DIR)/get-status.o $(BUILD_DIR)/attributes.o 
	@echo "\nLinking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -L$(LIB_DIR) -o $@ $(LIBS)

# Link get-term-esc-sequence
$(BIN_DIR)/get-term-esc-sequence: $(BUILD_DIR)/get-term-esc-sequence.o $(BUILD_DIR)/prompt2-utils.o $(BUILD_DIR)/term-attributes.o $(BUILD_DIR)/attributes.o 
	@echo "\nLinking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -L$(LIB_DIR) -o $@ $(LIBS)

# Link test-get-status
$(BIN_DIR)/test-get-status: $(BUILD_DIR)/test-get-status.o $(BUILD_DIR)/get-status.o
	@echo "\nLinking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -L$(LIB_DIR) -o $@ $(LIBS)

# Link test-prompt2-utils
$(BIN_DIR)/test-prompt2-utils: $(BUILD_DIR)/test-prompt2-utils.o $(BUILD_DIR)/prompt2-utils.o
	@echo "\nLinking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -L$(LIB_DIR) -o $@ $(LIBS)

# Link test-term-attributes
$(BIN_DIR)/test-term-attributes: $(BUILD_DIR)/test-term-attributes.o $(BUILD_DIR)/term-attributes.o $(BUILD_DIR)/prompt2-utils.o $(BUILD_DIR)/attributes.o 
	@echo "\nLinking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -L$(LIB_DIR) -o $@ $(LIBS)


# Clean Target
clean:
	@echo "Cleaning..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Install Target
install-local: $(BINARIES)
	@echo "Installing binaries to $(HOME)/bin"
	@mkdir -p $(HOME)/bin
	install -m 755 $^ $(HOME)/bin

# Test Target
test:
	 bats test

# Help Target
help:
	@echo "Available targets:"
	@echo "  all           - Builds the executables and runs tests"
	@echo "  build         - Compiles the sources and creates the executables"
	@echo "  clean         - Removes object files and the executables"
	@echo "  install-local - Installs the executables to ~/bin"
	@echo "  test          - Runs tests using bats or another testing framework"
	@echo "  help          - Displays this help information"