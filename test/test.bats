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

  # When we test the prompt lib
  run -0 $TEST_FUNCTION

  # Then
  # - full-path should point all the way to HOME
  # - basename should contain the name of the directory of HOME
  # - home_path should print ~/
  # - and all the git info should be NO_DATA or -1 (see fixture: FIXTURE_NO_GIT)
  select_fixture "no-git"
  update_fixture CWD.full      $(realpath $HOME)
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~\/}"
  FIXTURE=$(commit_fixture)

  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "cd to a non-git dir inside of HOME" {
  # Given
  # - we're in HOME/somedir
  # - it's not a git repo
  tmpdir=$(mktemp -d "$HOME/tmp.XXXXXX")
  cd $tmpdir

  # When we test the prompt lib
  run -0 $TEST_FUNCTION

  # Then
  # - full path should be $HOME/tmp.XXXXXX
  # - basename should be tmp.XXXXXX
  # - home_path should be ~/tmp.XXXXXX
  select_fixture "no-git"
  update_fixture CWD.full      $tmpdir
  update_fixture CWD.basename  $(basename $tmpdir)
  update_fixture CWD.home_path "${PWD/$HOME/\~}"
  FIXTURE=$(commit_fixture)

  diff $FIXTURE <(echo "$output")
  ec=$?

  # cleanup
  rm -rf $tmpdir

  return $ec
}

# --------------------------------------------------
@test "cd to a non-git dir outside of HOME" {
  # Given
  # - we're in a dir outside of HOME
  # - it's not a git repo
  tmpdir=$(realpath $(mktemp -d "/tmp/tmp.XXXXXX"))
  cd $tmpdir


  # When  we test the prompt lib
  run -0 $TEST_FUNCTION

  # Then
  # - full path should point at /tmp/tmp.XXXXXX
  # - basename should be tmp.XXXXXX
  # - home_path should be the same as fullpath
  select_fixture "no-git"
  update_fixture CWD.full      $tmpdir
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~\/}"
  FIXTURE=$(commit_fixture)

  diff $FIXTURE <(echo "$output")
  ec=$?

  # cleanup
  rm -rf $tmpdir
  return $ec
}

# --------------------------------------------------
@test "empty git repository does nothing" {
  # Given
  # - we create an empty git repo
  helper__new_repo

  # When  we test the prompt lib
  run -0 $TEST_FUNCTION

  select_fixture "no-git"
  update_fixture CWD.full      $(realpath $HOME)
  update_fixture CWD.basename  $(basename $PWD)
  update_fixture CWD.home_path "${PWD/$HOME/\~\/}"
  FIXTURE=$(commit_fixture)

  # Then
  # - We should get zero diff
  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "adding file to empty git repo does nothing" {
  # Given
  # - we create an empty git repo
  # - we add (but do not commit) a file
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
  # - we commit a new file
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


  # When we test the prompt lib
  run -0 $TEST_FUNCTION

  select_fixture "git-simple-no-upstream"
  update_fixture CWD.full         $(realpath $PWD)
  update_fixture CWD.basename     $(basename $PWD)
  update_fixture CWD.git_path     '+/'
  update_fixture CWD.home_path    "${PWD/$HOME/\~\/}"
  update_fixture Repo.name        $(basename $PWD)

  # Then we should find one unstaged file
  update_fixture Unstaged.status  'MODIFIED'
  update_fixture Unstaged.num     '1'
  FIXTURE=$(commit_fixture)

  diff $FIXTURE <(echo "$output")
}

# --------------------------------------------------
@test "cd to subdirectory in a git repo" {
  # Given
  # - we chdir to a subdirectory
  helper__new_repo_and_commit "newfile" "some text"
  mkdir -p subdir
  cd subdir


  # When we test the prompt lib
  run -0 $TEST_FUNCTION

  select_fixture "git-simple-no-upstream"
  update_fixture CWD.full         $(realpath $PWD)
  update_fixture CWD.basename     $(basename $PWD)
  update_fixture CWD.home_path    "${PWD/$HOME/\~}"

  # Then
  # - the repo name should stay the same
  # - and the git_path should be updated
  update_fixture Repo.name        $(basename $(git rev-parse --show-toplevel))
  update_fixture CWD.git_path     '+/subdir'
  FIXTURE=$(commit_fixture)
  
  diff $FIXTURE <(echo "$output")

}

# --------------------------------------------------
@test "changing localbranch" {
  # given we have a git repo
  helper__new_repo_and_commit "newfile" "some text"

  # given we change localbranch
  git checkout -b featureBranch

  # when we run the prompt lib
  run -0 $TEST_FUNCTION

  select_fixture "git-simple-no-upstream"
  update_fixture CWD.full         $(realpath $PWD)
  update_fixture CWD.basename     $(basename $PWD)
  update_fixture CWD.git_path     '+/'
  update_fixture CWD.home_path    "${PWD/$HOME/\~\/}"
  update_fixture Repo.name        $(basename $(git rev-parse --show-toplevel))

  # then the branch.name should be 'featureBranch'
  update_fixture Repo.branch.name 'featureBranch'
  FIXTURE=$(commit_fixture)

  diff $FIXTURE <(echo "$output")
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

