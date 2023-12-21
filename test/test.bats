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
  select_fixture "no-git"
  update_fixture CWD.full      $(realpath $PWD)
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~\/}"
  FIXTURE=$(commit_fixture)

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "cd to a non-git dir inside of HOME" {
  # Given
  # - we're in HOME/somedir
  # - it's not a git repo
  tmpdir=$(mktemp -d "$HOME/tmp.XXXXXX")
  cd $tmpdir
  select_fixture "no-git"
  update_fixture CWD.full      $(realpath $PWD)
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~}"
  FIXTURE=$(commit_fixture)

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")

  # cleanup
  rm -rf $tmpdir
}

# --------------------------------------------------
@test "cd to a non-git dir outside of HOME" {
  # Given
  # - we're in a dir outside of HOME
  # - it's not a git repo
  
  tmpdir=$(mktemp -d "/tmp/tmp.XXXXXX")
  cd $(realpath $tmpdir)

  select_fixture "no-git"
  update_fixture CWD.full      $(realpath $PWD)
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~\/}"
  FIXTURE=$(commit_fixture)

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")

  # cleanup
  rm -rf $tmpdir
}

# --------------------------------------------------
@test "empty git repository does nothing" {
  # Given
  # - we create an empty git repo
  helper__new_repo

  select_fixture "no-git"
  update_fixture CWD.full      $(realpath $PWD)
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~\/}"
  FIXTURE=$(commit_fixture)

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

  select_fixture "no-git"
  update_fixture CWD.full      $(realpath $PWD)
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~\/}"
  FIXTURE=$(commit_fixture)

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

  select_fixture "git-simple-no-upstream"
  update_fixture CWD.full         $(realpath $PWD)
  update_fixture CWD.basename     $(basename $PWD)
  update_fixture CWD.git_path     '+/'
  update_fixture CWD.home_path    "${PWD/$HOME/\~\/}"

  update_fixture Repo.name        $(basename $PWD)
  FIXTURE=$(commit_fixture)

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "modifying tracked file" {
  # Given
  # - we create an empty git repo with one tracked file
  # - we modify it
  helper__new_repo_and_commit "newfile" "some text"
  echo > newfile

  select_fixture "git-simple-no-upstream"
  update_fixture CWD.full         $(realpath $PWD)
  update_fixture CWD.basename     $(basename $PWD)
  update_fixture CWD.git_path     '+/'
  update_fixture CWD.home_path    "${PWD/$HOME/\~\/}"

  update_fixture Repo.name        $(basename $PWD)
  update_fixture Unstaged.status  'MODIFIED'
  update_fixture Unstaged.num     '1'

  FIXTURE=$(commit_fixture)

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "changing localbranch" {
  skip
}

# --------------------------------------------------
@test "cloning a repo and entering it" {
  skip
}

# --------------------------------------------------
@test "committing a change in a cloned repo" {
  skip
}

# --------------------------------------------------
@test "when local is behind upstream" {
  skip
}

# --------------------------------------------------
@test "when local is ahead of upstream" {
  skip
}

# --------------------------------------------------
@test "when local is both ahead and behind upstream" {
  skip
}

# --------------------------------------------------
@test "a conflict should update state" {
  skip
}

# --------------------------------------------------
@test "an add after conflict" {
  skip
}

# --------------------------------------------------
@test "interactive rebase" {
  skip
}

