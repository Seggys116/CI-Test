package ciutil_test

import (
	"bytes"
	"errors"
	"strings"
	"testing"

	"ciutil/ciutil"
)

func run(args ...string) (code int, stdout []byte, stderr string) {
	var out, errOut bytes.Buffer
	code = ciutil.Run(args, &out, &errOut)
	return code, out.Bytes(), errOut.String()
}

func expectOK(t *testing.T, want string, args ...string) {
	t.Helper()
	code, stdout, stderr := run(args...)
	if code != 0 || string(stdout) != want || stderr != "" {
		t.Errorf("Run(%q) = %d, %q, %q; want 0, %q, empty stderr", args, code, stdout, stderr, want)
	}
}

func expectExit(t *testing.T, wantCode int, wantStderrPrefix string, args ...string) {
	t.Helper()
	code, stdout, stderr := run(args...)
	if code != wantCode {
		t.Errorf("Run(%q) exit code = %d, want %d", args, code, wantCode)
	}
	if len(stdout) != 0 {
		t.Errorf("Run(%q) wrote to stdout: %q", args, stdout)
	}
	if !strings.HasPrefix(stderr, wantStderrPrefix) {
		t.Errorf("Run(%q) stderr = %q, want prefix %q", args, stderr, wantStderrPrefix)
	}
}

func TestRunChecksums(t *testing.T) {
	expectOK(t, "cbf43926\n", "crc32", "123456789")
	expectOK(t, "00000000\n", "crc32", "")
	expectOK(t, "e8b7be43\n", "crc32", "a")
	expectOK(t, "414fa339\n", "crc32", "The quick brown fox jumps over the lazy dog")
	expectOK(t, "11e60398\n", "adler32", "Wikipedia")
	expectOK(t, "00000001\n", "adler32", "")
	expectOK(t, "091e01de\n", "adler32", "123456789")
}

func TestRunBase64(t *testing.T) {
	expectOK(t, "Zm9vYmFy\n", "b64enc", "foobar")
	expectOK(t, "Zg==\n", "b64enc", "f")
	expectOK(t, "\n", "b64enc", "")
	expectOK(t, "foob", "b64dec", "Zm9vYg==")
	expectOK(t, "", "b64dec", "")
	expectOK(t, "\x00\xff\xfe", "b64dec", "AP/+")
}

func TestRunTreatsInputAsRawUTF8Bytes(t *testing.T) {
	text := "héllo 世界"
	_, encoded, _ := run("b64enc", text)
	expectOK(t, text, "b64dec", strings.TrimSuffix(string(encoded), "\n"))
}

func TestRunRoman(t *testing.T) {
	expectOK(t, "I\n", "roman", "1")
	expectOK(t, "MCMXCIV\n", "roman", "1994")
	expectOK(t, "MMMCMXCIX\n", "roman", "3999")
	expectOK(t, "2024\n", "unroman", "MMXXIV")
	expectOK(t, "4\n", "unroman", "IV")
}

func TestRunSemverCmp(t *testing.T) {
	expectOK(t, "-1\n", "semver-cmp", "1.0.0-alpha", "1.0.0")
	expectOK(t, "1\n", "semver-cmp", "1.0.0", "1.0.0-alpha")
	expectOK(t, "0\n", "semver-cmp", "1.0.0+a", "1.0.0+b")
	expectOK(t, "-1\n", "semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11")
}

func TestRunVersion(t *testing.T) {
	expectOK(t, "ciutil go 1.0.0\n", "version")
}

func TestRunInvalidInputExitsWithErrorCode(t *testing.T) {
	cases := [][]string{
		{"b64dec", "Zg"},
		{"b64dec", "Zh=="},
		{"b64dec", "Zm9-"},
		{"roman", "0"},
		{"roman", "4000"},
		{"roman", "-1"},
		{"roman", "abc"},
		{"roman", ""},
		{"roman", "99999999999999999999999"},
		{"unroman", ""},
		{"unroman", "IIII"},
		{"unroman", "xiv"},
		{"semver-cmp", "1.0", "1.0.0"},
		{"semver-cmp", "1.0.0", "v1.0.0"},
	}
	for _, args := range cases {
		expectExit(t, ciutil.ExitError, "ciutil: ", args...)
	}
}

func TestRunMisuseExitsWithUsageCode(t *testing.T) {
	cases := [][]string{
		{},
		{"frobnicate"},
		{"frobnicate", "x"},
		{"crc32"},
		{"crc32", "a", "b"},
		{"adler32"},
		{"b64enc"},
		{"b64dec", "a", "b"},
		{"roman"},
		{"unroman", "I", "I"},
		{"semver-cmp"},
		{"semver-cmp", "1.0.0"},
		{"semver-cmp", "1.0.0", "1.0.0", "1.0.0"},
		{"version", "extra"},
		{"CRC32", "a"},
	}
	for _, args := range cases {
		expectExit(t, ciutil.ExitUsage, "usage: ciutil", args...)
	}
}

type brokenWriter struct{}

func (brokenWriter) Write([]byte) (int, error) {
	return 0, errors.New("closed")
}

func TestRunReportsWriteFailures(t *testing.T) {
	var errOut bytes.Buffer
	code := ciutil.Run([]string{"crc32", "abc"}, brokenWriter{}, &errOut)
	if code != ciutil.ExitError {
		t.Errorf("exit code = %d, want %d", code, ciutil.ExitError)
	}
	if got, want := errOut.String(), "ciutil: closed\n"; got != want {
		t.Errorf("stderr = %q, want %q", got, want)
	}
}
