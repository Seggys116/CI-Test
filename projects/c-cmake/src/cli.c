#include "ciutil.h"

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROJECT_NAME "c-cmake"

#define CLI_EXIT_OK 0
#define CLI_EXIT_ERROR 2
#define CLI_EXIT_USAGE 64

static int usage(FILE *err) {
  fputs(
      "usage: ciutil <command> [arguments]\n"
      "  crc32 STRING\n"
      "  adler32 STRING\n"
      "  b64enc STRING\n"
      "  b64dec BASE64\n"
      "  roman NUMBER\n"
      "  unroman ROMAN\n"
      "  semver-cmp A B\n"
      "  version\n",
      err);
  return CLI_EXIT_USAGE;
}

static int report(FILE *err, const char *what, ciutil_status status) {
  fprintf(err, "ciutil: %s: %s\n", what, ciutil_status_message(status));
  return CLI_EXIT_ERROR;
}

static int parse_int(const char *s, int *value) {
  int negative = 0;
  int result = 0;

  if (*s == '-') {
    negative = 1;
    s++;
  }
  if (*s == '\0') {
    return 0;
  }
  for (; *s != '\0'; s++) {
    int digit;
    if (*s < '0' || *s > '9') {
      return 0;
    }
    digit = *s - '0';
    if (result > (INT_MAX - digit) / 10) {
      return 0;
    }
    result = result * 10 + digit;
  }
  *value = negative ? -result : result;
  return 1;
}

static int cmd_crc32(const char *arg, FILE *out) {
  fprintf(out, "%08" PRIx32 "\n", ciutil_crc32(arg, strlen(arg)));
  return CLI_EXIT_OK;
}

static int cmd_adler32(const char *arg, FILE *out) {
  fprintf(out, "%08" PRIx32 "\n", ciutil_adler32(arg, strlen(arg)));
  return CLI_EXIT_OK;
}

static int cmd_b64enc(const char *arg, FILE *out, FILE *err) {
  char *encoded = NULL;
  ciutil_status status = ciutil_base64_encode(arg, strlen(arg), &encoded);
  if (status != CIUTIL_OK) {
    return report(err, "b64enc", status);
  }
  fprintf(out, "%s\n", encoded);
  ciutil_free(encoded);
  return CLI_EXIT_OK;
}

static int cmd_b64dec(const char *arg, FILE *out, FILE *err) {
  uint8_t *decoded = NULL;
  size_t len = 0;
  ciutil_status status = ciutil_base64_decode_alloc(arg, strlen(arg), &decoded, &len);
  if (status != CIUTIL_OK) {
    return report(err, "b64dec", status);
  }
  if (len > 0 && fwrite(decoded, 1, len, out) != len) {
    ciutil_free(decoded);
    fputs("ciutil: b64dec: write failed\n", err);
    return CLI_EXIT_ERROR;
  }
  ciutil_free(decoded);
  return CLI_EXIT_OK;
}

static int cmd_roman(const char *arg, FILE *out, FILE *err) {
  int value = 0;
  char buf[CIUTIL_ROMAN_MAX_LEN + 1];
  size_t len = 0;
  ciutil_status status;

  if (!parse_int(arg, &value)) {
    fprintf(err, "ciutil: roman: invalid number: %s\n", arg);
    return CLI_EXIT_ERROR;
  }
  status = ciutil_roman_encode(value, buf, sizeof buf, &len);
  if (status != CIUTIL_OK) {
    return report(err, "roman", status);
  }
  fprintf(out, "%s\n", buf);
  return CLI_EXIT_OK;
}

static int cmd_unroman(const char *arg, FILE *out, FILE *err) {
  unsigned int value = 0;
  ciutil_status status = ciutil_roman_decode(arg, &value);
  if (status != CIUTIL_OK) {
    return report(err, "unroman", status);
  }
  fprintf(out, "%u\n", value);
  return CLI_EXIT_OK;
}

static int cmd_semver_cmp(const char *a_text, const char *b_text, FILE *out, FILE *err) {
  ciutil_semver a;
  ciutil_semver b;
  ciutil_status status;
  int result;

  status = ciutil_semver_parse(a_text, &a);
  if (status != CIUTIL_OK) {
    return report(err, "semver-cmp", status);
  }
  status = ciutil_semver_parse(b_text, &b);
  if (status != CIUTIL_OK) {
    ciutil_semver_free(&a);
    return report(err, "semver-cmp", status);
  }
  result = ciutil_semver_compare(&a, &b);
  ciutil_semver_free(&a);
  ciutil_semver_free(&b);
  fprintf(out, "%d\n", result);
  return CLI_EXIT_OK;
}

int ciutil_cli_run(int argc, char *const *argv, FILE *out, FILE *err) {
  const char *cmd;

  if (argc < 2 || argv == NULL) {
    return usage(err);
  }
  cmd = argv[1];

  if (strcmp(cmd, "version") == 0) {
    if (argc != 2) {
      return usage(err);
    }
    fprintf(out, "ciutil %s %s\n", PROJECT_NAME, CIUTIL_VERSION);
    return CLI_EXIT_OK;
  }
  if (strcmp(cmd, "crc32") == 0) {
    return argc == 3 ? cmd_crc32(argv[2], out) : usage(err);
  }
  if (strcmp(cmd, "adler32") == 0) {
    return argc == 3 ? cmd_adler32(argv[2], out) : usage(err);
  }
  if (strcmp(cmd, "b64enc") == 0) {
    return argc == 3 ? cmd_b64enc(argv[2], out, err) : usage(err);
  }
  if (strcmp(cmd, "b64dec") == 0) {
    return argc == 3 ? cmd_b64dec(argv[2], out, err) : usage(err);
  }
  if (strcmp(cmd, "roman") == 0) {
    return argc == 3 ? cmd_roman(argv[2], out, err) : usage(err);
  }
  if (strcmp(cmd, "unroman") == 0) {
    return argc == 3 ? cmd_unroman(argv[2], out, err) : usage(err);
  }
  if (strcmp(cmd, "semver-cmp") == 0) {
    return argc == 4 ? cmd_semver_cmp(argv[2], argv[3], out, err) : usage(err);
  }
  return usage(err);
}
