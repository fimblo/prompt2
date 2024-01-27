#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats


# Binary to test
TEST_FUNCTION="$BATS_TEST_DIRNAME/../bin/test-functions"

load test_helper_functions


# --------------------------------------------------
@test "running from a non-git-repo" {
  # Given
  # - it's not a git repo

  # When we test the prompt lib
  run -0 $TEST_FUNCTION

  # Then
  # - is_git_repo should be 0

  echo "$output" > "$HOME/assert-file"
  assert Repo.is_git_repo 0
}

# --------------------------------------------------
@test "running from HOME" {
  # Given
  # - we're in HOME
  # - it's not a git repo

  # When we test the prompt lib
  run -0 $TEST_FUNCTION

  # Then
  # - is_git_repo should be 0
  # - full-path should point all the way to HOME
  # - basename should contain the name of the directory of HOME
  # - home_path should print ~/
  # - git_path should be NO_DATA

  echo "$output" > "$HOME/assert-file"
  assert Repo.is_git_repo 0
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
  # - is_git_repo should be 0
  # - full path should be $HOME/tmp.XXXXXX
  # - basename should be tmp.XXXXXX
  # - home_path should be ~/tmp.XXXXXX
  # - git_path should be NO_DATA
  echo "$output" > "$HOME/assert-file"
  assert Repo.is_git_repo 0
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
  # - is_git_repo should be 0
  # - full path should point at /tmp/tmp.XXXXXX
  # - basename should be tmp.XXXXXX
  # - home_path should be the same as fullpath
  # - git_path should be NO_DATA
  echo "$output" > "$HOME/assert-file"
  assert Repo.is_git_repo 0
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
  # - it should almost behave as if it was a normal (non-git) directory
  #
  # More specifically, everything except is_git_repo should be the
  # same as a non-git directory
  echo "$output" > "$HOME/assert-file"
  assert Repo.is_git_repo 1
  assert CWD.full      $(realpath $PWD)
  assert CWD.basename  $(basename $PWD)
  assert CWD.home_path "${PWD/$HOME/\~\/}"
  assert CWD.git_path  "NO_DATA"
  assert Repo.has_upstream '-1'

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
  assert Repo.is_git_repo 1
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
@test "creating some untracked files updates state" {
  # Given
  # - we have a repo with some tracked files
  # - and we create three new files (untracked)
  helper__new_repo_and_commit "newfile" "some text"

  touch one two three

  # When we test the prompt lib
  run -0 $TEST_FUNCTION


  # Then
  # - we should find three untracked files detected
  echo "$output" > "$HOME/assert-file"
  assert Untracked.num    3
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
  # - we should detect that there is a remote
  # - and it should be in sync with remote
  echo "$output" > "$HOME/assert-file"
  assert Repo.name        'myRepo'
  assert Repo.has_upstream   '1'
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
  assert Repo.ahead       '1'
  assert Repo.behind      '1'
}

# --------------------------------------------------
@test "when local has a merge conflict with upstream" {
  # Setup: Create a git repo and clone it
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "commonfile" "initial text"
  cd -

  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # Given a change in the original repo
  cd myRepo
  echo "change in original repo" > commonfile
  git add commonfile
  git commit -m 'change in original repo'
  cd -

  # Given a change to the same file in the cloned repo
  cd anotherLocation/myRepo
  echo "conflicting change in cloned repo" > commonfile
  git add commonfile
  git commit -m 'conflicting change in cloned repo'

  # When running the test function, expect a conflict
  git pull || true  #force happy ending else test fails
  run -0 $TEST_FUNCTION

  # Then check that conflict_num has increased to 1
  # .. and that Repo.has_upstream should be 0
  echo "$output" > "$HOME/assert-file"
  assert Repo.conflict.num         '1'
  assert Repo.rebase_active   '1'
  assert Repo.has_upstream '0'
}

# --------------------------------------------------
@test "when adding file after conflict" {
  # Setup: Create a git repo and clone it
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "commonfile" "initial text"
  cd -

  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # Given a change in the original repo
  cd myRepo
  echo "change in original repo" > commonfile
  git add commonfile
  git commit -m 'change in original repo'
  cd -

  # Given a change to the same file in the cloned repo
  cd anotherLocation/myRepo
  echo "conflicting change in cloned repo" > commonfile
  git add commonfile
  git commit -m 'conflicting change in cloned repo'

  # Given a conflict
  git pull || true  #force happy ending else test fails

  # When fixing the problem and adding it
  echo "tomato" > commonfile
  git add commonfile

  run -0 $TEST_FUNCTION

  # Then number of conflicts should decrease to zero
  # .. number of staged should be 1
  # .. and Repo.has_upstream should still be 0
  echo "$output" > "$HOME/assert-file"
  assert Staged.num        '1'
  assert Repo.has_upstream '0'
  assert Repo.conflict.num      '0'
}

# --------------------------------------------------
@test "when committing file after conflict" {
  # Setup: Create a git repo and clone it
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "commonfile" "initial text"
  cd -

  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # Given a change in the original repo
  cd myRepo
  echo "change in original repo" > commonfile
  git add commonfile
  git commit -m 'change in original repo'
  cd -

  # Given a change to the same file in the cloned repo
  cd anotherLocation/myRepo
  echo "conflicting change in cloned repo" > commonfile
  git add commonfile
  git commit -m 'conflicting change in cloned repo'

  # Given a conflict
  git pull || true  #force happy ending else test fails

  # When resolving the conflict and committing it
  echo "tomato" > commonfile
  git add commonfile
  git commit -m 'resolve conflicting change'

  run -0 $TEST_FUNCTION

  # Then number of conflicts should be zero
  # .. number of staged should be 0
  echo "$output" > "$HOME/assert-file"
  assert Staged.num        '0'
  assert Repo.conflict.num      '0'
  assert Repo.rebase_active '1'
}

# --------------------------------------------------
@test "interactive rebase" {
  # given we have a git repo which we have some commits on
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  echo "hello" > newfile
  git commit -a -m '2nd commit'
  echo "bye" > newfile
  git commit -a -m '3rd commit'

  # when we enter an interactive rebase session
  cat<<-EOF>edit.sh
	#!/bin/bash
	sed -i'bak' 's/^pick/edit/' \$1
	EOF
  chmod 755 edit.sh
  cat edit.sh >&2
  GIT_SEQUENCE_EDITOR=./edit.sh git rebase -i HEAD^1

  run -0 $TEST_FUNCTION
  
  # Then number of conflicts should be zero
  # .. number of staged should be 0
  echo "$output" > "$HOME/assert-file"
  assert Staged.num        '0'
  assert Repo.conflict.num      '0'
  assert Repo.rebase_active '1'
}

# --------------------------------------------------
@test "aws sso token has not been granted" {
  # given no cache directory for the tokens to be in
  rm -rf $HOME/.aws/sso/cache || true  # don't worry, HOME is pointing to a tmpdir, and it should always fail to delete

  # when we run the test lib
  run -0 $TEST_FUNCTION

  # then it should tell us that the aws_token is not valid
  # .. and all remaining time counters should be -1 (no tokens found)
  echo "$output" > "$HOME/assert-file"
  assert AWS.token_is_valid          '0'
  assert AWS.token_remaining_hours   '-1'
  assert AWS.token_remaining_minutes '-1'
}

# --------------------------------------------------
@test "aws sso token is outdated" {
  # given an expired aws token
  if [[ "$(uname)" == "Linux" ]]; then
    timestamp=$(date --date='-30 days' +"%Y-%m-%dT%H:%M:%SZ")
  else
    timestamp=$(date -v-30d +"%Y-%m-%dT%H:%M:%SZ")
  fi

  mkdir -p $HOME/.aws/sso/cache
  cat<<-EOF>$HOME/.aws/sso/cache/token.json
	{
	 "startUrl":  "https://someurl.awsapps.com/start#/",
	 "region":    "eu-central-1",
	 "expiresAt": "${timestamp}"
	}
	EOF

  # when we run the test lib
  run -0 $TEST_FUNCTION

  # then it should tell us that the aws_token is not valid
  # .. and all remaining time counters should be 0
  echo "$output" > "$HOME/assert-file"
  assert AWS.token_is_valid          '0'
  assert AWS.token_remaining_hours   '0'
  assert AWS.token_remaining_minutes '0'
}

# --------------------------------------------------
@test "aws sso token is valid" {
  # given a valid aws token
  mkdir -p $HOME/.aws/sso/cache
  if [[ "$(uname)" == "Linux" ]]; then
    timestamp=$(date -u --date='+135 minutes' +"%Y-%m-%dT%H:%M:%SZ")
  else
    timestamp=$(date -u -v+135M +"%Y-%m-%dT%H:%M:%SZ")
  fi

  cat<<-EOF>$HOME/.aws/sso/cache/token.json
	{
	 "startUrl":  "https://someurl.awsapps.com/start#/",
	 "region":    "eu-central-1",
	 "expiresAt": "${timestamp}"
	}
	EOF

  # when we run the test lib
  run -0 $TEST_FUNCTION
  cat $HOME/.aws/sso/cache/token.json > /dev/stderr

  # then it should tell us that the aws_token is valid
  echo "$output" > "$HOME/assert-file"
  assert AWS.token_is_valid          '1'
  assert AWS.token_remaining_hours   '2'

  # Sometimes, the expected and received minutes differ by one because
  # the test was run just on the verge of the clock turning from one
  # minute to the next.
  # So we test both 15 and 14.
  assert AWS.token_remaining_minutes '15' \
    || assert AWS.token_remaining_minutes '14'
}
