#!/usr/bin/env bash
# record-demo.sh — scripted terminal demo of prompt2 in action
#
# Requirements:
#   brew install asciinema   # records the session
#   brew install agg         # optional: converts the recording to a GIF
#
# Usage:
#   asciinema rec demo.cast -c 'bash scripts/record-demo.sh'
#   agg demo.cast demo.gif                  # produces an animated GIF
#   agg --fps 15 demo.cast demo.gif         # slower, easier to follow

set -uo pipefail

PROMPT2="${HOME}/bin/prompt2"
CONFIG="${HOME}/.prompt2_config.ini"

# ── Pre-flight ─────────────────────────────────────────────────────────────
if [[ ! -x "$PROMPT2" ]]; then
  echo "prompt2 not found at $PROMPT2 — adjust PROMPT2 at the top of this script" >&2
  exit 1
fi
if [[ ! -f "$CONFIG" ]]; then
  echo "Config not found at $CONFIG" >&2
  exit 1
fi

# ── Helpers ────────────────────────────────────────────────────────────────

# Call prompt2 for the current directory and render it exactly as bash would.
# ${PS1@P} is bash's own prompt expansion (bash 4.4+): it strips \[ \] markers,
# converts \e to ESC, expands \n to newline, etc. — the same path bash takes
# when displaying PS1, so colours are guaranteed to render correctly.
render_prompt() {
  PS1=$("$PROMPT2" "$CONFIG" 2>/dev/null)
  printf '%s' "${PS1@P}"
}

# Print characters one at a time to simulate live typing.
type_chars() {
  local text="$1" speed="${2:-0.04}"
  for ((i = 0; i < ${#text}; i++)); do
    printf '%s' "${text:i:1}"
    sleep "$speed"
  done
}

# Show the current prompt, "type" a command, then execute it.
run() {
  local cmd="$1" pause="${2:-0.7}"
  render_prompt
  type_chars "$cmd"
  sleep 0.3
  printf '\n'
  eval "$cmd"
  sleep "$pause"
}

# ── Setup ─────────────────────────────────────────────────────────────────

# A local bare repo stands in for a remote so the demo is self-contained.
BARE_REMOTE=/tmp/myapp-remote.git
DEMO_DIR=/tmp/myapp-demo
rm -rf "$BARE_REMOTE" "$DEMO_DIR"
git init --bare "$BARE_REMOTE" -q

cleanup() {
  rm -rf "$BARE_REMOTE" "$DEMO_DIR"
}
trap cleanup EXIT

# ── Demo ──────────────────────────────────────────────────────────────────

cd "$HOME"
sleep 0.8

# Normal (non-git) directory — the default prompt
run 'cd /tmp' 0.5

# Spin up a new project
run "mkdir myapp-demo && cd myapp-demo" 0.5

# After git init the prompt switches to the git variant
run '#
### Initializing a new repo and set it up
###' 0.3
run 'git init' 0.8
run "git remote add origin $BARE_REMOTE" 0.3
run "git config user.email 'demo@example.com' && git config user.name 'Demo'" 0.3

run 'echo "# myapp" > README.md' 0.6
run 'git add README.md' 0.6
run 'git commit -m "initial commit"' 0.8

run '#
### Now you can see the repo stats.
### repo name, branch name, upstream info, local info, and project root directory.
###' 0.7


# Push — prompt shows in sync with remote
run 'git push -u origin main' 1.0

# Add more work
run "#
### Do some more work - counters change accordingly
### Here are the counters. I've grouped it like this [0,0|0,0,0], but
### it's configurable.
###
###  [0,0|0,0,0]
###   ^ ^ ^ ^ ^
###   | | | | +- Number of untracked files
###   | | | +--- Number of modified tracked files
###   | | +----- Number of staged files
###   | +------- Number of commits this repo is ahead of remote
###   +--------- Number of commits this repo is behind remote   
###
###" 0.3
run 'echo foobar > myfile.md  # Creating a new file raises untracked counter to 1' 0.8
run 'echo foobar >> README.md # Modding a tracked file raises tracked counter to 1' 0.8
run 'git add *.md             # Adding the 2 files raises uncommitted changes to 2' 0.8
run 'git commit -m "hello"    # Commiting resets local counters. Upstream raised to 1' 0.8
run 'git push  # On push, all trackers reset' 0.8

# Step out — back to the non-git prompt
run 'cd ~' 0.3

run '#
### prompt2 has support for:
### - showing branches
### - conflicts
### - interactive rebase state
### - AWS session info
### - colours and text style depending on state too.
###' 1

sleep 1.2

