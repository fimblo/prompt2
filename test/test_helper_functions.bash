# --------------------------------------------------
# Common variables
# --------------------------------------------------

DEFAULT_GIT_BRANCH_NAME='main'




# --------------------------------------------------
# Helper functions to simplify git operations
# --------------------------------------------------
helper__set_git_config() {
  # Set repo-local git config
  git config user.email "my@test.com"
  git config user.name "Test Person"
  git config pull.rebase true
}

helper__new_repo() {
  git init --initial-branch="$DEFAULT_GIT_BRANCH_NAME"
  helper__set_git_config
}

helper__new_repo_and_add_file() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__new_repo
  echo "$content_to_commit" > "$file_to_commit"
  git add "$file_to_commit"
}

helper__new_repo_and_commit() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__new_repo_and_add_file "$file_to_commit" "$content_to_commit"
  git commit -m 'Initial commit'
}


# --------------------------------------------------
# Helper functions for assisting with tests
# --------------------------------------------------

assert() {
  file="$HOME/assert-file"
  key="$1"
  expected_value="$2"

  if [[ ! -e $file ]] ; then
    echo "file '$file' does not exist."
    echo "ensure test output is stored there"
    exit 1
  fi

  received_value=$(grep "^$key" "$file" | cut -d' ' -f2)
  if [[ "$expected_value" == "$received_value" ]] ; then
    return 0
  fi
  
  echo "Expected: '$key' '$expected_value'" > /dev/stderr
  echo "Received: '$key' '$received_value'" > /dev/stderr
  return 1
}

# --------------------------------------------------
# Regular set up and tear down functions
# --------------------------------------------------

# These two functions run before and after all tests.
setup_file() {
  create_fixtures_before_tests
}
teardown_file() {
  rm -rf "$BATS_FILE_TMPDIR"
}


# These two functions run before and after each test
setup () {
  # shellcheck disable=SC2046
  RUN_TMPDIR=$(realpath $(mktemp -d "$BATS_TEST_TMPDIR/tmp.XXXXXX" ))
  HOME_BK=$HOME
  export HOME=$RUN_TMPDIR
  cd "$HOME" || true
}
teardown () {
  HOME=$HOME_BK
  rm -rf "$BATS_TEST_TMPDIR"
}
