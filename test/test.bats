#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats


helper__set_git_config() {
  # Set repo-local git config
  git config user.email "my@test.com"
  git config user.name "Test Person"
  git config pull.rebase true
}

helper__new_repo() {
  git init --initial-branch=main
  helper__set_git_config
}

helper__new_repo_and_add_file() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__new_repo
  echo "$content_to_commit" > $file_to_commit
  git add $file_to_commit
}

helper__new_repo_and_commit() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__new_repo_and_add_file "$file_to_commit" "$content_to_commit"
  git commit -m 'Initial commit'
}



# Binary to test
TEST_FUNCTION="$BATS_TEST_DIRNAME/../bin/test-function"


# run before each test
setup () {
  RUN_TMPDIR=$( mktemp -d "$BATS_TEST_TMPDIR/tmp.XXXXXX" )
  cd $RUN_TMPDIR
}

# run after each test
teardown () {
  rm -rf $RUN_TMPDIR
}



# --------------------------------------------------
@test "empty test" {
  # given
  # when
  # then

  skip
}

