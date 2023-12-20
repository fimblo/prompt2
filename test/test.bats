#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats


# Binary to test
TEST_FUNCTION="$BATS_TEST_DIRNAME/../bin/test-functions"

load test_helper_functions

# --------------------------------------------------
@test "running from HOME" {
  # Given
  # - we're in HOME
  # - it's not a git repo
  FIXTURE=$(select_fixture "no-git")
  update_fixture $FIXTURE CWD.full      $(realpath $PWD)
  update_fixture $FIXTURE CWD.basename  $(basename $PWD)
  update_fixture $FIXTURE CWD.git_path  'NO_DATA'
  update_fixture $FIXTURE CWD.home_path "${PWD/$HOME/\~\/}"

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "empty git repository does nothing" {
  # Given
  # - we create an empty git repo
  helper__new_repo

  FIXTURE=$(select_fixture "no-git")
  update_fixture $FIXTURE CWD.full      $(realpath $PWD)
  update_fixture $FIXTURE CWD.basename  $(basename $PWD)
  update_fixture $FIXTURE CWD.git_path  'NO_DATA'
  update_fixture $FIXTURE CWD.home_path "${PWD/$HOME/\~\/}"

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "adding file to empty git repo does nothing" {
  # Given
  # - we create an empty git repo
  helper__new_repo_and_add_file "newfile" "some text"

  FIXTURE=$(select_fixture "no-git")
  update_fixture $FIXTURE CWD.full      $(realpath $PWD)
  update_fixture $FIXTURE CWD.basename  $(basename $PWD)
  update_fixture $FIXTURE CWD.git_path  'NO_DATA'
  update_fixture $FIXTURE CWD.home_path "${PWD/$HOME/\~\/}"

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "committing in empty git repo updates state" {
  # Given
  # - we create an empty git repo
  helper__new_repo_and_commit "newfile" "some text"

  FIXTURE=$(select_fixture "git-simple")
  update_fixture $FIXTURE CWD.full         $(realpath $PWD)
  update_fixture $FIXTURE CWD.basename     $(basename $PWD)
  update_fixture $FIXTURE CWD.git_path     '+/'
  update_fixture $FIXTURE CWD.home_path    "${PWD/$HOME/\~\/}"

  update_fixture $FIXTURE Repo.name        $(basename $PWD)
  update_fixture $FIXTURE Repo.status      'NO_UPSTREAM'
  update_fixture $FIXTURE Repo.ahead       '-1'
  update_fixture $FIXTURE Repo.behind      '-1'

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

