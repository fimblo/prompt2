# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Is

`prompt2` is a fast (~10-13ms) bash prompt generator written in C. It reads an INI config file, gathers system/git/AWS context, processes widget tokens, applies terminal colors, and writes the final prompt to stdout. There is also a companion CLI tool `get-attribute` for querying terminal escape sequences.

## Build and Test Commands

**Dependencies (Ubuntu/Debian):**
```bash
sudo apt-get install build-essential git libgit2-dev libiniparser-dev libjson-c-dev uthash-dev bats
```

**Dependencies (macOS):**
```bash
brew install libgit2 json-c uthash iniparser bats-core bash
```

**Common make targets:**
```bash
make          # build executables and run tests
make build    # compile only
make test     # run BATS test suite
make clean    # remove object files and executables
make install-local  # install to ~/bin
make tarball  # create distributable tarball
```

**Running a single test file:**
```bash
bats test/test-get-status.bats
bats test/test-prompt2-utils.bats
bats test/test-term-attributes.bats
```

**Test the prompt interactively in the current shell:**
```bash
source config/set-prompt.ini.sh
```

## Architecture

The project compiles to two binaries: `prompt2` (main prompt generator) and `get-attribute` (escape sequence query tool), plus several test binaries.

**Data flow:**
```
INI config → prompt2.c reads config & widget definitions
                  ↓
           get-status.c populates CurrentState (git, system, AWS)
                  ↓
           Widget tokens @{widget_name} replaced with real values
                  ↓
           term-attributes.c applies ANSI color/style escape sequences
                  ↓
           Final prompt string written to stdout
```

**Key source files:**

- `src/prompt2.c` — Entry point. Reads INI config, manages widget configuration via uthash hash tables, assembles the final prompt string.
- `src/get-status.c` — Gathers all runtime context: git repo detection/branch/divergence (via libgit2), username/hostname/UID, AWS SSO token validity.
- `src/prompt2-utils.c` — String/path utilities: trimming, truncation with ellipsis, path shortening, terminal width detection, escape sequence validation, widget token parsing.
- `src/term-attributes.c` — Converts human-readable attribute names (e.g. `"fg red bold"`) to ANSI escape codes.
- `src/attributes.c` — Database of ~700+ named colors/styles mapped to ANSI codes, including xorg rgb.txt color names.
- `src/get-attribute.c` — Standalone CLI that queries `term-attributes.c` and prints the escape sequence for a given attribute name.

**Header files define the main data structures:**
- `src/get-status.h` — `CurrentState` struct (all gathered context)
- `src/constants.h` — Shared enums and size constants
- `src/attributes.h` — Attribute struct definition

## Configuration

Users place an INI file at `~/.prompt2_config.ini`. The config has two sections: `[PROMPT]` for non-git contexts and `[PROMPT.GIT]` for git repos. Widget tokens `@{widget_name}` in the prompt string are replaced at runtime. See `config/dot.prompt2_config.ini` for the canonical example and `Customisation.md` for widget documentation.

## Conventions

- Follows [Conventional Commits](https://www.conventionalcommits.org/) for commit messages.
- Compiler flags: `-Wall -Wextra` — keep warnings clean.
- Tests use BATS (Bash Automated Testing System) v1.5.0+; helper functions live in `test/test_helper_functions.bash`.
- CI runs on Ubuntu via `.github/workflows/build-and-test.yml`.
