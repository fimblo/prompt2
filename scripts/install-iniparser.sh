#!/usr/bin/env bash
# Build a static iniparser (>= 4.2) into <repo>/deps/iniparser.
#
# prompt2 needs iniparser >= 4.2, but older Debian-based distros (eg.
# Ubuntu 24.04) only package 4.1.x. Instead of replacing the system
# library, this builds iniparser from source as a static library inside
# the repo, so it can be compiled into prompt2 without sudo and without
# touching the installed libiniparser-dev package.
#
# Usage: scripts/install-iniparser.sh [version]
#   version: an iniparser release tag, eg. v4.2.6 (default: latest release)

set -euo pipefail

BOLD="\e[1m"
RESET="\e[0m"
GREENSTAR="\e[1;32m*${RESET}"
REDSTAR="\e[1;31m*${RESET}"

print_title() {
  echo -e "${BOLD}$1${RESET}"
}

die() {
  echo -e "$REDSTAR $1" >&2
  exit 1
}

MIN_VERSION="4.2"
REPO_URL="https://gitlab.com/iniparser/iniparser.git"

repodir=$(cd "$(dirname "$0")/.."; pwd)
prefix="$repodir/deps/iniparser"


# --------------------------------------------------
# Sanity checks
# --------------------------------------------------

print_title "Checking system..."

if [[ -f /etc/os-release ]]; then
  . /etc/os-release
fi
if [[ " ${ID:-} ${ID_LIKE:-} " != *" debian "* && " ${ID:-} ${ID_LIKE:-} " != *" ubuntu "* ]]; then
  die "This script is for Debian-based distros. Elsewhere, install iniparser >= $MIN_VERSION with your package manager."
fi

missing=""
for tool in cc make cmake git; do
  command -v "$tool" &> /dev/null || missing="$missing $tool"
done
if [[ -n $missing ]]; then
  die "Missing tools:$missing. Install them with:
  sudo apt-get install -y build-essential cmake git"
fi
echo -e "$GREENSTAR build tools are installed"


# --------------------------------------------------
# Pick a version
# --------------------------------------------------

version_ge() { # is $1 >= $2?
  [[ "$(printf '%s\n' "$2" "$1" | sort -V | head -n1)" == "$2" ]]
}

if [[ $# -ge 1 ]]; then
  tag="$1"
else
  print_title "Looking up latest iniparser release..."
  tag=$(git ls-remote --tags --refs "$REPO_URL" 'v*' \
          | sed 's#.*refs/tags/##' \
          | grep -E '^v[0-9]+\.[0-9]+(\.[0-9]+)?$' \
          | sort -V | tail -n1)
  [[ -n $tag ]] || die "Could not find any release tags at $REPO_URL"
fi

version_ge "${tag#v}" "$MIN_VERSION" || die "iniparser $tag is too old; prompt2 needs >= $MIN_VERSION"
echo -e "$GREENSTAR using iniparser $tag"


# --------------------------------------------------
# Build and install into deps/
# --------------------------------------------------

workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT

print_title "Downloading iniparser $tag..."
git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$tag" "$REPO_URL" "$workdir/src" \
  || die "Could not clone tag '$tag' from $REPO_URL"

print_title "Building static library..."
cmake -S "$workdir/src" -B "$workdir/build" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DBUILD_TESTING=OFF \
      -DCMAKE_INSTALL_PREFIX="$prefix" \
      -DCMAKE_INSTALL_LIBDIR=lib \
      > "$workdir/cmake.log" 2>&1 \
  || { cat "$workdir/cmake.log" >&2; die "cmake configure failed"; }
cmake --build "$workdir/build" -j > "$workdir/build.log" 2>&1 \
  || { cat "$workdir/build.log" >&2; die "Build failed"; }

rm -rf "$prefix"
cmake --install "$workdir/build" > /dev/null

# Check that we actually got a usable >= 4.2 static library
cat > "$workdir/probe.c" <<-EOF
	#include <iniparser/iniparser.h>
	int main(void) { return iniparser_load_file == 0; }
	EOF
cc "$workdir/probe.c" -I"$prefix/include" "$prefix/lib/libiniparser.a" -o "$workdir/probe" \
  || die "Built iniparser, but could not link against it"

echo -e "$GREENSTAR iniparser $tag installed to $prefix"
