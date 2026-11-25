#include "ciutil.h"
#include "harness.h"

static const char *const rejected[] = {
    "",
    "1",
    "1.0",
    "1.0.0.0",
    "v1.0.0",
    "01.0.0",
    "1.00.0",
    "1.0.00",
    "1.0.0-",
    "1.0.0+",
    "1.0.0-01",
    "1.0.0-a..b",
    "1.0.0-a_b",
    "-1.0.0",
    " 1.0.0",
    "1.0.0 ",
    "1.0.0-alpha+",
    "1.0.0-.a",
    "1.0.0-a.",
    "1.0.0-a.01",
    "1.0.0-01.a",
    "1.0.0+a..b",
    "1.0.0+.a",
    "1.0.0+a.",
    "1.0.0+a+b",
    "1.0.0-a+b+c",
    "1.0.0-+a",
    "1.0.0++a",
    "1..0",
    "1.0.",
    ".1.0.0",
    "a.b.c",
    "1.0.0-\xC3\xA9",
    "1.x.0",
    "1.0.0\n",
    "+1.0.0",
};

static const char *const accepted[] = {
    "0.0.0",
    "1.2.3",
    "1.0.0-0",
    "1.0.0-0a",
    "1.0.0-x-y-z.-",
    "1.0.0+001",
    "1.0.0-rc.1+build.5",
    "1.0.0-alpha",
    "1.0.0-alpha.1",
    "1.0.0-0.3.7",
    "1.0.0-x.7.z.92",
    "1.0.0+20130313144700",
    "1.0.0-beta+exp.sha.5114f85",
    "10.20.30",
    "1.0.0-a.0",
    "1.0.0+0.0",
    "18446744073709551615.18446744073709551615.18446744073709551615",
};

static const char *const chain[] = {
    "1.0.0-alpha",  "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta",
    "1.0.0-beta.2", "1.0.0-beta.11", "1.0.0-rc.1",       "1.0.0",
};

static const char *const core_chain[] = {"0.9.9", "1.0.0", "2.0.0", "2.1.0", "2.1.1", "10.0.0"};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

static int cmp_text(const char *a, const char *b) {
  ciutil_semver va;
  ciutil_semver vb;
  int result = 99;

  CHECK_EQ_I(ciutil_semver_parse(a, &va), CIUTIL_OK);
  CHECK_EQ_I(ciutil_semver_parse(b, &vb), CIUTIL_OK);
  result = ciutil_semver_compare(&va, &vb);
  ciutil_semver_free(&va);
  ciutil_semver_free(&vb);
  return result;
}

static void test_parse_fields(void) {
  ciutil_semver v;

  CHECK_EQ_I(ciutil_semver_parse("1.2.3", &v), CIUTIL_OK);
  CHECK_EQ_U(v.major, 1);
  CHECK_EQ_U(v.minor, 2);
  CHECK_EQ_U(v.patch, 3);
  CHECK_EQ_U(v.prerelease_count, 0);
  CHECK_EQ_U(v.build_count, 0);
  ciutil_semver_free(&v);

  CHECK_EQ_I(ciutil_semver_parse("1.0.0-rc.1+build.5", &v), CIUTIL_OK);
  CHECK_EQ_U(v.prerelease_count, 2);
  CHECK_EQ_U(v.build_count, 2);
  if (v.prerelease_count == 2 && v.build_count == 2) {
    CHECK_STR(v.prerelease[0], "rc");
    CHECK_STR(v.prerelease[1], "1");
    CHECK_STR(v.build[0], "build");
    CHECK_STR(v.build[1], "5");
  }
  ciutil_semver_free(&v);

  CHECK_EQ_I(ciutil_semver_parse("1.0.0-x-y-z.-", &v), CIUTIL_OK);
  CHECK_EQ_U(v.prerelease_count, 2);
  if (v.prerelease_count == 2) {
    CHECK_STR(v.prerelease[0], "x-y-z");
    CHECK_STR(v.prerelease[1], "-");
  }
  ciutil_semver_free(&v);

  CHECK_EQ_I(ciutil_semver_parse("1.0.0+001", &v), CIUTIL_OK);
  CHECK_EQ_U(v.prerelease_count, 0);
  CHECK_EQ_U(v.build_count, 1);
  if (v.build_count == 1) {
    CHECK_STR(v.build[0], "001");
  }
  ciutil_semver_free(&v);

  CHECK_EQ_I(ciutil_semver_parse("1.0.0-0a", &v), CIUTIL_OK);
  if (v.prerelease_count == 1) {
    CHECK_STR(v.prerelease[0], "0a");
  }
  ciutil_semver_free(&v);
}

static void test_accepts_valid(void) {
  size_t i;
  for (i = 0; i < COUNT(accepted); i++) {
    ciutil_semver v;
    ciutil_status status = ciutil_semver_parse(accepted[i], &v);
    CHECK_EQ_I(status, CIUTIL_OK);
    if (status == CIUTIL_OK) {
      ciutil_semver_free(&v);
    }
  }
}

static void test_rejects_invalid(void) {
  size_t i;
  for (i = 0; i < COUNT(rejected); i++) {
    ciutil_semver v;
    ciutil_status status = ciutil_semver_parse(rejected[i], &v);
    CHECK_EQ_I(status, CIUTIL_ERR_SEMVER_SYNTAX);
    CHECK(v.prerelease == NULL && v.build == NULL);
    if (status == CIUTIL_OK) {
      ciutil_semver_free(&v);
    }
  }
}

static void test_numeric_limits(void) {
  ciutil_semver v;

  CHECK_EQ_I(ciutil_semver_parse("18446744073709551615.0.0", &v), CIUTIL_OK);
  CHECK(v.major == UINT64_MAX);
  ciutil_semver_free(&v);

  CHECK_EQ_I(ciutil_semver_parse("9223372036854775808.0.0", &v), CIUTIL_OK);
  CHECK(v.major == UINT64_C(9223372036854775808));
  ciutil_semver_free(&v);

  CHECK_EQ_I(ciutil_semver_parse("18446744073709551616.0.0", &v), CIUTIL_ERR_SEMVER_OVERFLOW);
  CHECK_EQ_I(ciutil_semver_parse("0.18446744073709551616.0", &v), CIUTIL_ERR_SEMVER_OVERFLOW);
  CHECK_EQ_I(ciutil_semver_parse("0.0.18446744073709551616", &v), CIUTIL_ERR_SEMVER_OVERFLOW);
  CHECK_EQ_I(ciutil_semver_parse("99999999999999999999.0.0", &v), CIUTIL_ERR_SEMVER_OVERFLOW);
  CHECK_EQ_I(ciutil_semver_parse("0.0.184467440737095516150", &v), CIUTIL_ERR_SEMVER_OVERFLOW);
}

static void test_invalid_arguments(void) {
  ciutil_semver v;
  CHECK_EQ_I(ciutil_semver_parse(NULL, &v), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_semver_parse("1.0.0", NULL), CIUTIL_ERR_INVALID_ARG);
}

static void test_precedence_chain_pairwise(void) {
  size_t i;
  size_t j;
  for (i = 0; i < COUNT(chain); i++) {
    for (j = 0; j < COUNT(chain); j++) {
      int expected = i < j ? -1 : (i > j ? 1 : 0);
      CHECK_EQ_I(cmp_text(chain[i], chain[j]), expected);
    }
  }
}

static void test_core_chain_pairwise(void) {
  size_t i;
  size_t j;
  for (i = 0; i < COUNT(core_chain); i++) {
    for (j = 0; j < COUNT(core_chain); j++) {
      int expected = i < j ? -1 : (i > j ? 1 : 0);
      CHECK_EQ_I(cmp_text(core_chain[i], core_chain[j]), expected);
    }
  }
}

static void test_build_metadata_is_ignored(void) {
  CHECK_EQ_I(cmp_text("1.0.0+a", "1.0.0+b"), 0);
  CHECK_EQ_I(cmp_text("1.0.0", "1.0.0+b"), 0);
  CHECK_EQ_I(cmp_text("1.0.0-rc.1+x", "1.0.0-rc.1+y.z"), 0);
  CHECK_EQ_I(cmp_text("1.0.0-alpha+z", "1.0.0-beta+a"), -1);
}

static void test_identifier_rules(void) {
  CHECK_EQ_I(cmp_text("1.0.0-beta.2", "1.0.0-beta.11"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-1", "1.0.0-a"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-9", "1.0.0-10"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-99999999999999999999", "1.0.0-100000000000000000000"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-B", "1.0.0-a"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-a", "1.0.0-a-"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-0a", "1.0.0-1"), 1);
  CHECK_EQ_I(cmp_text("1.0.0-alpha", "1.0.0-alpha.1"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-alpha.1", "1.0.0-alpha.beta"), -1);
  CHECK_EQ_I(cmp_text("1.0.0-rc.1", "1.0.0"), -1);
  CHECK_EQ_I(cmp_text("2.0.0-rc.1", "1.9.9"), 1);
  CHECK_EQ_I(cmp_text("1.9.9", "2.0.0-alpha"), -1);
  CHECK_EQ_I(cmp_text("18446744073709551615.0.0", "18446744073709551614.9.9"), 1);
}

static void test_to_string_round_trip(void) {
  size_t i;
  for (i = 0; i < COUNT(accepted); i++) {
    ciutil_semver v;
    ciutil_semver again;
    char *text = NULL;

    CHECK_EQ_I(ciutil_semver_parse(accepted[i], &v), CIUTIL_OK);
    CHECK_EQ_I(ciutil_semver_to_string(&v, &text), CIUTIL_OK);
    CHECK_STR(text, accepted[i]);
    if (text != NULL) {
      CHECK_EQ_I(ciutil_semver_parse(text, &again), CIUTIL_OK);
      CHECK_EQ_I(ciutil_semver_compare(&v, &again), 0);
      ciutil_semver_free(&again);
    }
    ciutil_free(text);
    ciutil_semver_free(&v);
  }
}

static void test_free_is_idempotent(void) {
  ciutil_semver v;
  CHECK_EQ_I(ciutil_semver_parse("1.0.0-a+b", &v), CIUTIL_OK);
  ciutil_semver_free(&v);
  CHECK(v.prerelease == NULL);
  CHECK_EQ_U(v.prerelease_count, 0);
  ciutil_semver_free(&v);
  ciutil_semver_free(NULL);
}

int main(void) {
  test_parse_fields();
  test_accepts_valid();
  test_rejects_invalid();
  test_numeric_limits();
  test_invalid_arguments();
  test_precedence_chain_pairwise();
  test_core_chain_pairwise();
  test_build_metadata_is_ignored();
  test_identifier_rules();
  test_to_string_round_trip();
  test_free_is_idempotent();
  return th_finish();
}
