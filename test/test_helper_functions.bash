
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


update_fixture() {
  fixture="$1"
  name="$2"
  value="$3"
  perl -pi -e "s{^$name .*}{$name $value}" "$fixture"
}


# Runs before each individual test
setup () {
  RUN_TMPDIR=$(realpath $(mktemp -d "$BATS_TEST_TMPDIR/tmp.XXXXXX" ))
  HOME_BK=$HOME
  export HOME=$RUN_TMPDIR
  cd "$HOME" || true
  
  FIXTURE_ZERO="$BATS_TEST_TMPDIR/fixture-0"
  cat<<EOF>"$FIXTURE_ZERO"
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

# Runs after each individual test
teardown () {
  HOME=$HOME_BK
  rm -rf "$RUN_TMPDIR" "$FIXTURE_ZERO"
}

