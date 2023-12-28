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
  # - git_path should be NO_DATA

  echo "$output" > "$HOME/assert-file"
  assert CWD.full      $(realpath $HOME)
  assert CWD.basename  $(basename $PWD)
  assert CWD.home_path "${PWD/$HOME/\~\/}"
  assert CWD.git_path  "NO_DATA"
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
  # - git_path should be NO_DATA
  echo "$output" > "$HOME/assert-file"
  assert CWD.full      $tmpdir
  assert CWD.basename  $(basename $tmpdir)
  assert CWD.home_path "${PWD/$HOME/\~}"
  assert CWD.git_path  "NO_DATA"
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
  # - git_path should be NO_DATA
  echo "$output" > "$HOME/assert-file"
  assert CWD.full      $(realpath $PWD)
  assert CWD.basename  $(basename $PWD)
  assert CWD.home_path "${PWD/$HOME/\~\/}"
  assert CWD.git_path  "NO_DATA"
}

# --------------------------------------------------
@test "empty git repository does nothing" {
  # Given
  # - we create an empty git repo
  helper__new_repo

  # When  we test the prompt lib
  run -0 $TEST_FUNCTION


  # Then
  # - it should behave as if it was a normal (non-git) directory
  echo "$output" > "$HOME/assert-file"
  assert CWD.full      $(realpath $PWD)
  assert CWD.basename  $(basename $PWD)
  assert CWD.home_path "${PWD/$HOME/\~\/}"
  assert CWD.git_path  "NO_DATA"
}

# --------------------------------------------------
@test "adding file to empty git repo does nothing" {
  # Given
  # - we create an empty git repo
  # - we add (but do not commit) a file
  helper__new_repo_and_add_file "newfile" "some text"


  # When we test the prompt lib
  run -0 $TEST_FUNCTION

  # Then
  # - it should behave as if it was a normal (non-git) directory
  echo "$output" > "$HOME/assert-file"
  assert CWD.full      $(realpath $PWD)
  assert CWD.basename  $(basename $PWD)
  assert CWD.home_path "${PWD/$HOME/\~\/}"
  assert CWD.git_path  "NO_DATA"
}

# --------------------------------------------------
@test "committing in empty git repo updates state" {
  # Given
  # - we create an empty git repo
  # - we commit a new file
  helper__new_repo_and_commit "newfile" "some text"

  # When we test the prompt lib
  run -0 $TEST_FUNCTION


  # Then
  # - git_path should be set to +/
  # - repo name should be set to basename of pwd
  echo "$output" > "$HOME/assert-file"
  assert CWD.git_path     '+/'
  assert CWD.home_path    "${PWD/$HOME/\~\/}"
  assert Repo.name        $(basename $PWD)
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

  # Then we should find one unstaged file
  echo "$output" > "$HOME/assert-file"
  assert Unstaged.status  'MODIFIED'
  assert Unstaged.num     '1'
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

  # Then
  # - the repo name should stay the same
  # - and the git_path should be updated
  echo "$output" > "$HOME/assert-file"
  assert Repo.name        $(basename $(git rev-parse --show-toplevel))
  assert CWD.git_path     '+/subdir'
}

# --------------------------------------------------
@test "changing localbranch" {
  # given we have a git repo
  helper__new_repo_and_commit "newfile" "some text"

  # given we change localbranch
  git checkout -b featureBranch

  # when we run the prompt lib
  run -0 $TEST_FUNCTION

  # then the branch.name should be 'featureBranch'
  echo "$output" > "$HOME/assert-file"
  assert Repo.branch.name 'featureBranch'
}

# --------------------------------------------------
@test "cloning a repo and entering it" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config


  # when we run the prompt
  run -0 $TEST_FUNCTION

  # then
  # - the repo name should be 'myRepo'
  # - and it should be in sync with remote
  echo "$output" > "$HOME/assert-file"
  assert Repo.name        'myRepo'
  assert Repo.status      'UP_TO_DATE'
  assert Repo.ahead       '0'
  assert Repo.behind      '0'
}

# --------------------------------------------------
@test "edit a tracked file in a cloned repo" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config

  # given we commit a change
  echo > newfile

  # when we run the prompt
  run -0 $TEST_FUNCTION

  # then
  # - should be ahead by one commit
  echo "$output" > "$HOME/assert-file"
  assert Unstaged.status  'MODIFIED'
  assert Unstaged.num     '1'
}

# --------------------------------------------------
@test "stage a file in a cloned repo" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config

  # given we commit a change
  echo > newfile
  git add newfile

  # when we run the prompt
  run -0 $TEST_FUNCTION

  # then
  # - should be ahead by one commit
  echo "$output" > "$HOME/assert-file"
  assert Staged.status  'MODIFIED'
  assert Staged.num     '1'
}

# --------------------------------------------------
@test "committing a change in a cloned repo" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config

  # given we commit a change
  echo > newfile
  git add newfile
  git commit -m 'update the file'

  # when we run the prompt
  run -0 $TEST_FUNCTION

  # then
  # - should be ahead by one commit
  echo "$output" > "$HOME/assert-file"
  assert Repo.status      'MODIFIED'
  assert Repo.ahead       '1'
}

# --------------------------------------------------
@test "when local is behind upstream" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it to anotherLocation/myRepo
  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # given we commit a change in the first repo 
  cd myRepo
  echo "new text" > newfile
  git add newfile
  git commit -m 'update the file with "new text"'
  cd -

  # given we git fetch in anotherLocation/myRepo
  cd anotherLocation/myRepo
  git fetch

  # when we run the prompt
  run -0 $TEST_FUNCTION

  # then
  # - should be behind by one commit
  echo "$output" > "$HOME/assert-file"
  assert Repo.status      'MODIFIED'
  assert Repo.behind      '1'
}

# --------------------------------------------------
@test "when local is both ahead and behind upstream" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it to anotherLocation/myRepo
  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # given we commit a change in the first repo 
  cd myRepo
  echo "new text" > newfile
  git add newfile
  git commit -m 'update the file with "new text"'
  cd -

  # given we commit another change in anotherLocation/myRepo
  cd anotherLocation/myRepo
  echo "new text2" > otherfile
  git add otherfile
  git commit -m 'update a file with "new text2"'
  cd -

  # given we git fetch in anotherLocation/myRepo
  cd anotherLocation/myRepo
  git fetch


  # when we run the prompt
  run -0 $TEST_FUNCTION

  # then
  # - should be ahead by one commit
  # - .. and behind by one
  echo "$output" > "$HOME/assert-file"
  assert Repo.status      'MODIFIED'
  assert Repo.ahead       '1'
  assert Repo.behind      '1'
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

