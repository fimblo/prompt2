#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats

DEFAULT_GIT_BRANCH_NAME='main'

# Binary to test
TEST_FUNCTION="$BATS_TEST_DIRNAME/../bin/test-functions"


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
  echo "$content_to_commit" > $file_to_commit
  git add $file_to_commit
}

helper__new_repo_and_commit() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__new_repo_and_add_file "$file_to_commit" "$content_to_commit"
  git commit -m 'Initial commit'
}


update_fixture() {
  fixture="$1"
  name="$2"
  value="$3"
  perl -pi -e "s{^$name .*}{$name $value}" $FIXTURE_ZERO
}





# run before each test
setup () {
  RUN_TMPDIR=$(realpath $(mktemp -d "$BATS_TEST_TMPDIR/tmp.XXXXXX" ))
  HOME_BK=$HOME
  export HOME=$RUN_TMPDIR
  cd
  
  FIXTURE_ZERO=$(mktemp)
  cat<<EOF> $FIXTURE_ZERO
CWD.full full path to current working directory
CWD.basename basename of current working directory
CWD.git_path path from git project root
CWD.home_path path from home
Repo.name NO_DATA
Repo.branch.name NO_DATA
Repo.status NO_DATA
Repo.ahead -1
Repo.behind -1
Staged.status NO_DATA
Staged.num -1
Unstaged.status NO_DATA
Unstaged.num -1
EOF
}

# run after each test
teardown () {
  HOME=$HOME_BK
  rm -rf $RUN_TMPDIR $FIXTURE_ZERO

}



# --------------------------------------------------
@test "running from HOME" {
  # Given
  # - we're in HOME
  # - it's not a git repo
  update_fixture $FIXTURE_ZERO CWD.full      $(realpath $PWD)
  update_fixture $FIXTURE_ZERO CWD.basename  $(basename $PWD)
  update_fixture $FIXTURE_ZERO CWD.git_path  'NO_DATA'
  update_fixture $FIXTURE_ZERO CWD.home_path "${PWD/$HOME/\~\/}"

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE_ZERO <(echo "$output")
}

# --------------------------------------------------
@test "empty git repository does nothing" {
  # Given
  # - we create an empty git repo
  helper__new_repo

  update_fixture $FIXTURE_ZERO CWD.full      $(realpath $PWD)
  update_fixture $FIXTURE_ZERO CWD.basename  $(basename $PWD)
  update_fixture $FIXTURE_ZERO CWD.git_path  'NO_DATA'
  update_fixture $FIXTURE_ZERO CWD.home_path "${PWD/$HOME/\~\/}"

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE_ZERO <(echo "$output")
}

# --------------------------------------------------
@test "adding file to empty git repo does nothing" {
  # Given
  # - we create an empty git repo
  helper__new_repo_and_add_file "newfile" "some text"

  update_fixture $FIXTURE_ZERO CWD.full      $(realpath $PWD)
  update_fixture $FIXTURE_ZERO CWD.basename  $(basename $PWD)
  update_fixture $FIXTURE_ZERO CWD.git_path  'NO_DATA'
  update_fixture $FIXTURE_ZERO CWD.home_path "${PWD/$HOME/\~\/}"

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE_ZERO <(echo "$output")
}

# --------------------------------------------------
@test "committing in empty git repo updates state" {
  # Given
  # - we create an empty git repo
  helper__new_repo_and_commit "newfile" "some text"

  update_fixture $FIXTURE_ZERO CWD.full         $(realpath $PWD)
  update_fixture $FIXTURE_ZERO CWD.basename     $(basename $PWD)
  update_fixture $FIXTURE_ZERO CWD.git_path     '+/'
  update_fixture $FIXTURE_ZERO CWD.home_path    "${PWD/$HOME/\~\/}"

  update_fixture $FIXTURE_ZERO Repo.name        $(basename $PWD)
  update_fixture $FIXTURE_ZERO Repo.branch.name $DEFAULT_GIT_BRANCH_NAME
  update_fixture $FIXTURE_ZERO Repo.status      'NO_UPSTREAM'
  update_fixture $FIXTURE_ZERO Staged.status    'UP_TO_DATE'
  update_fixture $FIXTURE_ZERO Staged.num       '0'
  update_fixture $FIXTURE_ZERO Unstaged.status  'UP_TO_DATE'
  update_fixture $FIXTURE_ZERO Unstaged.num     '0'

  # When
  # - we test the lib
  run -0 $TEST_FUNCTION

  # Then
  # - We should get zero diff
  diff $FIXTURE_ZERO <(echo "$output")
}

