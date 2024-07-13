#!/usr/bin/env bash
BOLD="\e[1m"
RESET="\e[0m"
GREENSTAR="\e[1;32m*${RESET}"
REDSTAR="\e[1;31m*${RESET}"

print_title() {
  msg="$1"
  echo -e "${BOLD}${msg}${RESET}"
}


# --------------------------------------------------
# Deal with dependencies
# --------------------------------------------------

if [[ "$OSTYPE" == "darwin"* ]]; then
  print_title "Checking for dependencies..."
  if ! xcode-select -p &> /dev/null; then
    cat<<-EOF
        $REDSTAR Xcode Command Line Tools are not installed.
		  Please install with the following command:
		  xcode-select --install
		EOF
    exit 1
  else
    echo -e "$GREENSTAR xcode command line tools are installed"
  fi

  if ! command -v brew &> /dev/null; then
    cat<<-EOF
		$REDSTAR Homebrew is not installed.
		  Please install Homebrew by following the instructions at
		  https://brew.sh/
		EOF
    exit 1
  else
    echo -e "$GREENSTAR homebrew is installed"
  fi

  print_title "Installing dependencies using homebrew..."
  brew install libgit2 json-c uthash iniparser
  brew install bats-core # for testing
  brew install bash # since macos vanilla bash is outdated

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
  if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
  else
    DISTRO=$(lsb_release -is)
  fi

  case "$DISTRO" in
    ubuntu|debian)
      print_title "Checking dependencies using dpkg-query..."


      # List of packages to check
      packages="build-essential git libgit2-dev libiniparser-dev libjson-c-dev uthash-dev bats"
      to_install=""

      # Check each package
      for pkg in $packages; do
        if ! dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "ok installed"; then
          to_install="$to_install $pkg"
        fi
      done

      if [[ -n $to_install ]] ; then
        cat<<-EOF
		${REDSTAR} You need to install some packages first. Please run the following commands:
		  sudo apt-get update
		  sudo apt-get install -y $to_install
		EOF
      fi
      ;;
    fedora|centos|rhel)
      print_title "Checking dependencies using rpm..."
      packages="gcc make git libgit2-devel iniparser-devel json-c-devel uthash-devel bats"
      to_install=""

      for pkg in $packages; do
        if ! rpm -q "$pkg" &> /dev/null; then
          to_install="$to_install $pkg"
        fi
      done

      if [[ -n $to_install ]] ; then
        cat<<-EOF
		${RED_STAR} You need to install some packages first. Please run the following command:
		  sudo dnf install -y $to_install
		EOF
      fi
      ;;
    *)
      echo -e "${REDSTAR} Unsupported Linux distribution: '$DISTRO'"
      exit 1
      ;;
  esac
else
  echo -e "${REDSTAR} Unsupported OS: '$OSTYPE'"
  exit 1
fi

# --------------------------------------------------
# build and install to ~/bin
# --------------------------------------------------


repodir=$(cd "$(dirname "$0")"; pwd) # since 'realpath' might not exist on target system
cd "$repodir"

if ! [[ -e Makefile ]] ; then
  echo -e "${REDSTAR} Cannot find Makefile in '$repodir'."
  exit 1
fi

print_title "Building binaries..."
make build

print_title "Install prompt2 and get-attribute to ~/bin"
make install-local

print_title "All done"

exit 0
