#include "ciutil.h"
#include "harness.h"

typedef struct cli_result {
  int code;
  char out[2048];
  size_t out_len;
  char err[2048];
  size_t err_len;
} cli_result;

static size_t slurp(FILE *f, char *buf, size_t cap) {
  size_t n;
  fflush(f);
  rewind(f);
  n = fread(buf, 1, cap - 1u, f);
  buf[n] = '\0';
  return n;
}

static void run_cli(cli_result *r, int argc, char *const *argv) {
  FILE *out = tmpfile();
  FILE *err = tmpfile();

  memset(r, 0, sizeof *r);
  r->code = -1;
  CHECK(out != NULL);
  CHECK(err != NULL);
  if (out != NULL && err != NULL) {
    r->code = ciutil_cli_run(argc, argv, out, err);
    r->out_len = slurp(out, r->out, sizeof r->out);
    r->err_len = slurp(err, r->err, sizeof r->err);
  }
  if (out != NULL) {
    fclose(out);
  }
  if (err != NULL) {
    fclose(err);
  }
}

#define RUN(res, ...)                                                        \
  do {                                                                       \
    char *argv_[] = {"ciutil", __VA_ARGS__, NULL};                           \
    run_cli(&(res), (int)(sizeof argv_ / sizeof argv_[0]) - 1, argv_);       \
  } while (0)

static void expect_ok(const char *a1, const char *a2, const char *expected_out) {
  cli_result r;
  char *argv[4];
  argv[0] = "ciutil";
  argv[1] = (char *)a1;
  argv[2] = (char *)a2;
  argv[3] = NULL;
  run_cli(&r, 3, argv);
  CHECK_EQ_I(r.code, 0);
  CHECK_STR(r.out, expected_out);
  CHECK_EQ_U(r.err_len, 0);
}

static void test_crc32(void) {
  expect_ok("crc32", "", "00000000\n");
  expect_ok("crc32", "a", "e8b7be43\n");
  expect_ok("crc32", "abc", "352441c2\n");
  expect_ok("crc32", "123456789", "cbf43926\n");
  expect_ok("crc32", "The quick brown fox jumps over the lazy dog", "414fa339\n");
}

static void test_adler32(void) {
  expect_ok("adler32", "", "00000001\n");
  expect_ok("adler32", "a", "00620062\n");
  expect_ok("adler32", "abc", "024d0127\n");
  expect_ok("adler32", "Wikipedia", "11e60398\n");
  expect_ok("adler32", "123456789", "091e01de\n");
}

static void test_b64enc(void) {
  expect_ok("b64enc", "", "\n");
  expect_ok("b64enc", "f", "Zg==\n");
  expect_ok("b64enc", "fo", "Zm8=\n");
  expect_ok("b64enc", "foobar", "Zm9vYmFy\n");
}

static void test_b64dec_text(void) {
  cli_result r;
  RUN(r, "b64dec", "Zm9vYmFy");
  CHECK_EQ_I(r.code, 0);
  CHECK_STR(r.out, "foobar");
  CHECK_EQ_U(r.out_len, 6);
  CHECK_EQ_U(r.err_len, 0);

  RUN(r, "b64dec", "");
  CHECK_EQ_I(r.code, 0);
  CHECK_EQ_U(r.out_len, 0);
}

static void test_b64dec_binary(void) {
  static const unsigned char expected[] = {0x00, 0xFF, 0xFE};
  cli_result r;
  RUN(r, "b64dec", "AP/+");
  CHECK_EQ_I(r.code, 0);
  CHECK_EQ_U(r.out_len, 3);
  if (r.out_len == 3) {
    CHECK_MEM(r.out, expected, 3);
  }
}

static void test_b64dec_newline_is_not_added(void) {
  cli_result r;
  RUN(r, "b64dec", "Zg==");
  CHECK_EQ_I(r.code, 0);
  CHECK_EQ_U(r.out_len, 1);
  CHECK(r.out[0] == 'f');
}

static void test_roman(void) {
  expect_ok("roman", "1", "I\n");
  expect_ok("roman", "4", "IV\n");
  expect_ok("roman", "1994", "MCMXCIV\n");
  expect_ok("roman", "1999", "MCMXCIX\n");
  expect_ok("roman", "2024", "MMXXIV\n");
  expect_ok("roman", "3999", "MMMCMXCIX\n");
}

static void test_unroman(void) {
  expect_ok("unroman", "I", "1\n");
  expect_ok("unroman", "XIV", "14\n");
  expect_ok("unroman", "MCMXCIV", "1994\n");
  expect_ok("unroman", "MCMXCIX", "1999\n");
  expect_ok("unroman", "MMMCMXCIX", "3999\n");
}

static void test_semver_cmp(void) {
  cli_result r;
  RUN(r, "semver-cmp", "1.0.0-alpha", "1.0.0");
  CHECK_EQ_I(r.code, 0);
  CHECK_STR(r.out, "-1\n");
  RUN(r, "semver-cmp", "1.0.0", "1.0.0-alpha");
  CHECK_STR(r.out, "1\n");
  RUN(r, "semver-cmp", "1.0.0+a", "1.0.0+b");
  CHECK_STR(r.out, "0\n");
  RUN(r, "semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11");
  CHECK_STR(r.out, "-1\n");
  RUN(r, "semver-cmp", "2.1.1", "2.1.0");
  CHECK_STR(r.out, "1\n");
}

static void test_version(void) {
  cli_result r;
  RUN(r, "version");
  CHECK_EQ_I(r.code, 0);
  CHECK_STR(r.out, "ciutil c-make 1.0.0\n");
  CHECK_EQ_U(r.err_len, 0);
}

static void expect_runtime_error(const char *a1, const char *a2) {
  cli_result r;
  char *argv[4];
  argv[0] = "ciutil";
  argv[1] = (char *)a1;
  argv[2] = (char *)a2;
  argv[3] = NULL;
  run_cli(&r, 3, argv);
  CHECK_EQ_I(r.code, 2);
  CHECK_EQ_U(r.out_len, 0);
  CHECK(r.err_len > 0);
}

static void test_runtime_errors(void) {
  cli_result r;
  expect_runtime_error("b64dec", "Zg=");
  expect_runtime_error("b64dec", "Zh==");
  expect_runtime_error("b64dec", "Zm-v");
  expect_runtime_error("roman", "0");
  expect_runtime_error("roman", "4000");
  expect_runtime_error("roman", "-5");
  expect_runtime_error("roman", "abc");
  expect_runtime_error("roman", "");
  expect_runtime_error("roman", "99999999999999999999");
  expect_runtime_error("unroman", "IIII");
  expect_runtime_error("unroman", "");
  expect_runtime_error("unroman", "xiv");
  RUN(r, "semver-cmp", "1.0", "1.0.0");
  CHECK_EQ_I(r.code, 2);
  CHECK_EQ_U(r.out_len, 0);
  RUN(r, "semver-cmp", "1.0.0", "v2.0.0");
  CHECK_EQ_I(r.code, 2);
  CHECK_EQ_U(r.out_len, 0);
}

static void test_usage_errors(void) {
  cli_result r;
  char *bare[] = {"ciutil", NULL};

  run_cli(&r, 1, bare);
  CHECK_EQ_I(r.code, 64);
  CHECK_EQ_U(r.out_len, 0);
  CHECK(r.err_len > 0);

  RUN(r, "bogus");
  CHECK_EQ_I(r.code, 64);
  CHECK_EQ_U(r.out_len, 0);
  CHECK(r.err_len > 0);

  RUN(r, "crc32");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "crc32", "a", "b");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "adler32");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "b64enc");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "b64dec", "Zg==", "x");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "roman");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "unroman", "I", "II");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "semver-cmp", "1.0.0");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "semver-cmp", "1.0.0", "1.0.0", "1.0.0");
  CHECK_EQ_I(r.code, 64);
  RUN(r, "version", "extra");
  CHECK_EQ_I(r.code, 64);
  CHECK_EQ_U(r.out_len, 0);
}

static void test_no_carriage_returns(void) {
  cli_result r;
  RUN(r, "crc32", "abc");
  CHECK(strchr(r.out, '\r') == NULL);
  RUN(r, "bogus");
  CHECK(strchr(r.err, '\r') == NULL);
  RUN(r, "roman", "0");
  CHECK(strchr(r.err, '\r') == NULL);
}

int main(void) {
  test_crc32();
  test_adler32();
  test_b64enc();
  test_b64dec_text();
  test_b64dec_binary();
  test_b64dec_newline_is_not_added();
  test_roman();
  test_unroman();
  test_semver_cmp();
  test_version();
  test_runtime_errors();
  test_usage_errors();
  test_no_carriage_returns();
  return th_finish();
}
