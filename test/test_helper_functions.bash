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
# Helper functions to create, select and update fixtures
# --------------------------------------------------
create_fixtures_before_tests() {
  FIXTURE_NO_GIT="$BATS_FILE_TMPDIR/fixture-no-git"
  cat<<EOF>"$FIXTURE_NO_GIT"
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
  
  FIXTURE_GIT_SIMPLE="$BATS_FILE_TMPDIR/fixture-git-simple"
  cat<<EOF>"$FIXTURE_GIT_SIMPLE"
CWD.full full path to current working directory
CWD.basename basename of current working directory
CWD.git_path path from git project root
CWD.home_path path from home
Repo.name NO_DATA
Repo.branch.name $DEFAULT_GIT_BRANCH_NAME
Repo.status UP_TO_DATE
Repo.ahead 0
Repo.behind 0
Staged.status UP_TO_DATE
Staged.num 0
Unstaged.status UP_TO_DATE
Unstaged.num 0
EOF
  
}

select_fixture() {
  fixture_name="fixture-$1"
  if [[ -e "$BATS_FILE_TMPDIR/$fixture_name" ]] ; then
    FIXTURE_COPY=$(mktemp "$BATS_TEST_TMPDIR/fixture.XXXXXX")
    cp "$BATS_FILE_TMPDIR/$fixture_name" "$FIXTURE_COPY"
    echo "$FIXTURE_COPY"
    return 0
  fi

  echo "No fixture with name '$fixture_name'." > /dev/stderr
  return 1
}

update_fixture() {
  fixture="$1"
  name="$2"
  value="$3"
  perl -pi -e "s{^$name .*}{$name $value}" "$fixture"
}


# These two functions run before and after all tests.
setup_file() {
  create_fixtures_before_tests
}
teardown_file() {
  rm -rf "$BATS_FILE_TMPDIR"
}


# These two functions run before and after each test
setup () {
  RUN_TMPDIR=$(realpath $(mktemp -d "$BATS_TEST_TMPDIR/tmp.XXXXXX" ))
  HOME_BK=$HOME
  export HOME=$RUN_TMPDIR
  cd "$HOME" || true
}
teardown () {
  HOME=$HOME_BK
  rm -rf "$BATS_TEST_TMPDIR"
}
