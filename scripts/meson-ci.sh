#!/usr/bin/env bash
# usage (from a meson project directory): meson-ci.sh <step>...
# steps: setup build test sanitize install cli
# env: BUILD_DIR (default build)
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
build=${BUILD_DIR:-build}

vsenv=()
case "${RUNNER_OS:-$(uname -s)}" in
  Windows | MINGW* | MSYS* | CYGWIN*) vsenv=(--vsenv) ;;
esac

find_exe() {
  local dir=$1 name=$2 found
  found=$(find "$dir" -maxdepth 3 -type f \( -name "$name" -o -name "$name.exe" \) | head -n 1)
  if [ -z "$found" ]; then
    echo "meson-ci: no $name executable under $dir" >&2
    return 1
  fi
  echo "$found"
}

step_setup() {
  meson setup "$build" ${vsenv[@]+"${vsenv[@]}"} --buildtype=release
}

step_build() {
  meson compile -C "$build"
}

step_test() {
  meson test -C "$build" --print-errorlogs
}

step_sanitize() {
  local dir="$build-asan"
  meson setup "$dir" --buildtype=debug -Db_sanitize=address,undefined -Db_lundef=false
  meson compile -C "$dir"
  meson test -C "$dir" --print-errorlogs
}

step_install() {
  meson install -C "$build" --destdir "$PWD/$build-dest"
}

step_cli() {
  local exe
  exe=$(find_exe "$build" ciutil)
  case "$exe" in
    /*) ;;
    *) exe=./$exe ;;
  esac
  bash "$here/cli-smoke.sh" "$exe"
}

if [ "$#" -eq 0 ]; then
  echo "usage: $0 <setup|build|test|sanitize|install|cli>..." >&2
  exit 64
fi

for step in "$@"; do
  echo "::group::meson-ci $step"
  "step_$step"
  echo "::endgroup::"
done
