#!/usr/bin/env bash
# usage (from a cmake project directory): cmake-ci.sh <step>...
# steps: configure build test install consume sanitize cli
# env: BUILD_DIR (default build), CIUTIL_GENERATOR (cmake -G value, default none), CMAKE_EXTRA_ARGS
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
build=${BUILD_DIR:-build}
config=Release
gen=()
if [ -n "${CIUTIL_GENERATOR:-}" ]; then
  gen=(-G "$CIUTIL_GENERATOR")
fi
extra=()
if [ -n "${CMAKE_EXTRA_ARGS:-}" ]; then
  # shellcheck disable=SC2206
  extra=($CMAKE_EXTRA_ARGS)
fi

native_path() {
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -m "$1"
  else
    echo "$1"
  fi
}

find_exe() {
  local dir=$1 name=$2 found
  found=$(find "$dir" -maxdepth 3 -type f \( -name "$name" -o -name "$name.exe" \) | head -n 1)
  if [ -z "$found" ]; then
    echo "cmake-ci: no $name executable under $dir" >&2
    return 1
  fi
  echo "$found"
}

step_configure() {
  cmake -S . -B "$build" ${gen[@]+"${gen[@]}"} -DCMAKE_BUILD_TYPE="$config" ${extra[@]+"${extra[@]}"}
}

step_build() {
  cmake --build "$build" --config "$config" --parallel
}

step_test() {
  ctest --test-dir "$build" -C "$config" --output-on-failure --no-tests=error
}

step_install() {
  cmake --install "$build" --config "$config" --prefix "$build-install"
}

step_consume() {
  local prefix
  prefix=$(native_path "$(cd "$build-install" && pwd)")
  cmake -S consumer -B "$build-consumer" ${gen[@]+"${gen[@]}"} \
    -DCMAKE_BUILD_TYPE="$config" -DCMAKE_PREFIX_PATH="$prefix"
  cmake --build "$build-consumer" --config "$config"
  local exe out
  exe=$(find_exe "$build-consumer" consumer)
  case "$exe" in
    /*) ;;
    *) exe=./$exe ;;
  esac
  out=$("$exe" | tr -d '\r')
  if [ "$out" != "cbf43926" ]; then
    echo "cmake-ci: consumer printed '$out', expected cbf43926" >&2
    return 1
  fi
  echo "consumer ok: $out"
}

step_sanitize() {
  local saved=$build
  build=$build-asan
  config=Debug
  extra=(-DCIUTIL_SANITIZE=ON ${extra[@]+"${extra[@]}"})
  step_configure
  step_build
  step_test
  build=$saved
  config=Release
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
  echo "usage: $0 <configure|build|test|install|consume|sanitize|cli>..." >&2
  exit 64
fi

for step in "$@"; do
  echo "::group::cmake-ci $step"
  "step_$step"
  echo "::endgroup::"
done
