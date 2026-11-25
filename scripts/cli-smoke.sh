#!/usr/bin/env bash
# usage: scripts/cli-smoke.sh <cli command and leading args...>
# runs the shared ciutil CLI contract against any project's built binary.
set -u

if [ "$#" -lt 1 ]; then
  echo "usage: $0 <cli command...>" >&2
  exit 64
fi

failures=0
checks=0

hex() { od -An -tx1 -v | tr -d ' \n'; }

check_hex() {
  local label=$1 want=$2
  shift 2
  local got
  got=$("$@" 2>/dev/null | hex)
  checks=$((checks + 1))
  if [ "$got" != "$want" ]; then
    echo "FAIL $label: want hex '$want' got '$got'" >&2
    failures=$((failures + 1))
  else
    echo "ok   $label"
  fi
}

check_exit() {
  local label=$1 want=$2
  shift 2
  local rc
  "$@" >/dev/null 2>&1
  rc=$?
  checks=$((checks + 1))
  if [ "$rc" -ne "$want" ]; then
    echo "FAIL $label: want exit $want got $rc" >&2
    failures=$((failures + 1))
  else
    echo "ok   $label"
  fi
}

check_stderr_nonempty() {
  local label=$1
  shift
  local err
  err=$("$@" 2>&1 >/dev/null)
  checks=$((checks + 1))
  if [ -z "$err" ]; then
    echo "FAIL $label: expected a message on stderr" >&2
    failures=$((failures + 1))
  else
    echo "ok   $label"
  fi
}

text_hex() { printf '%s' "$1" | hex; }

CLI=("$@")

version_out=$("${CLI[@]}" version 2>/dev/null | tr -d '\r')
checks=$((checks + 1))
case "$version_out" in
  "ciutil "*" 1.0.0") echo "ok   version ($version_out)" ;;
  *)
    echo "FAIL version: got '$version_out'" >&2
    failures=$((failures + 1))
    ;;
esac
check_hex "version has no CR" "$(text_hex "$version_out")0a" "${CLI[@]}" version

check_hex "crc32 vector" "$(text_hex $'cbf43926\n')" "${CLI[@]}" crc32 123456789
check_hex "crc32 empty" "$(text_hex $'00000000\n')" "${CLI[@]}" crc32 ""
check_hex "crc32 fox" "$(text_hex $'414fa339\n')" "${CLI[@]}" crc32 "The quick brown fox jumps over the lazy dog"
check_hex "adler32 vector" "$(text_hex $'11e60398\n')" "${CLI[@]}" adler32 Wikipedia
check_hex "adler32 empty" "$(text_hex $'00000001\n')" "${CLI[@]}" adler32 ""
check_hex "b64enc vector" "$(text_hex $'Zm9vYmFy\n')" "${CLI[@]}" b64enc foobar
check_hex "b64enc padded" "$(text_hex $'Zm9vYg==\n')" "${CLI[@]}" b64enc foob
check_hex "b64dec text" "$(text_hex foobar)" "${CLI[@]}" b64dec Zm9vYmFy
check_hex "b64dec binary" "00fffe" "${CLI[@]}" b64dec "AP/+"
check_hex "b64dec empty" "" "${CLI[@]}" b64dec ""
check_hex "roman 1994" "$(text_hex $'MCMXCIV\n')" "${CLI[@]}" roman 1994
check_hex "roman 3999" "$(text_hex $'MMMCMXCIX\n')" "${CLI[@]}" roman 3999
check_hex "unroman" "$(text_hex $'1994\n')" "${CLI[@]}" unroman MCMXCIV
check_hex "semver less" "$(text_hex $'-1\n')" "${CLI[@]}" semver-cmp 1.0.0-beta.2 1.0.0-beta.11
check_hex "semver equal build" "$(text_hex $'0\n')" "${CLI[@]}" semver-cmp 1.0.0+a 1.0.0+b
check_hex "semver greater" "$(text_hex $'1\n')" "${CLI[@]}" semver-cmp 2.0.0 1.0.0
check_hex "semver prerelease below release" "$(text_hex $'-1\n')" "${CLI[@]}" semver-cmp 1.0.0-rc.1 1.0.0

check_exit "b64dec rejects non-canonical bits" 2 "${CLI[@]}" b64dec "Zh=="
check_exit "b64dec rejects bad length" 2 "${CLI[@]}" b64dec "Zg="
check_exit "b64dec rejects url-safe chars" 2 "${CLI[@]}" b64dec "AP_-"
check_exit "roman rejects zero" 2 "${CLI[@]}" roman 0
check_exit "roman rejects 4000" 2 "${CLI[@]}" roman 4000
check_exit "unroman rejects IIII" 2 "${CLI[@]}" unroman IIII
check_exit "unroman rejects lowercase" 2 "${CLI[@]}" unroman xiv
check_exit "semver rejects leading zero" 2 "${CLI[@]}" semver-cmp 01.0.0 1.0.0
check_exit "semver rejects two parts" 2 "${CLI[@]}" semver-cmp 1.0 1.0.0
check_exit "unknown subcommand" 64 "${CLI[@]}" bogus
check_exit "missing argument" 64 "${CLI[@]}" crc32
check_exit "no subcommand" 64 "${CLI[@]}"
check_stderr_nonempty "error reports on stderr" "${CLI[@]}" roman 0
check_stderr_nonempty "usage reports on stderr" "${CLI[@]}" bogus

echo "cli-smoke: $checks checks, $failures failures"
[ "$failures" -eq 0 ]
