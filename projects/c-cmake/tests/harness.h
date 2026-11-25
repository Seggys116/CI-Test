#ifndef CIUTIL_TEST_HARNESS_H
#define CIUTIL_TEST_HARNESS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int th_checks;
static int th_failures;

static inline void th_check(int ok, const char *expr, const char *file, int line) {
  th_checks++;
  if (!ok) {
    th_failures++;
    fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expr);
  }
}

static inline void th_check_u64(unsigned long long actual, unsigned long long expected,
                                const char *expr, const char *file, int line) {
  th_checks++;
  if (actual != expected) {
    th_failures++;
    fprintf(stderr, "%s:%d: check failed: %s: got %llu (0x%llx), expected %llu (0x%llx)\n", file,
            line, expr, actual, actual, expected, expected);
  }
}

static inline void th_check_i64(long long actual, long long expected, const char *expr,
                                const char *file, int line) {
  th_checks++;
  if (actual != expected) {
    th_failures++;
    fprintf(stderr, "%s:%d: check failed: %s: got %lld, expected %lld\n", file, line, expr, actual,
            expected);
  }
}

static inline void th_check_str(const char *actual, const char *expected, const char *expr,
                                const char *file, int line) {
  int same;
  th_checks++;
  if (actual == NULL || expected == NULL) {
    same = (actual == expected);
  } else {
    same = (strcmp(actual, expected) == 0);
  }
  if (!same) {
    th_failures++;
    fprintf(stderr, "%s:%d: check failed: %s: got \"%s\", expected \"%s\"\n", file, line, expr,
            actual != NULL ? actual : "(null)", expected != NULL ? expected : "(null)");
  }
}

static inline void th_check_mem(const void *actual, const void *expected, size_t len,
                                const char *expr, const char *file, int line) {
  th_checks++;
  if (len > 0 && memcmp(actual, expected, len) != 0) {
    th_failures++;
    fprintf(stderr, "%s:%d: check failed: %s: %lu bytes differ\n", file, line, expr,
            (unsigned long)len);
  }
}

/* nonzero when any check failed or none ran */
static inline int th_finish(void) {
  printf("%d checks, %d failures\n", th_checks, th_failures);
  return (th_failures == 0 && th_checks > 0) ? 0 : 1;
}

/* deterministic bytes: x = x * 1103515245 + 12345 mod 2^31, byte = (x >> 16) & 0xff, seed 42 */
static inline void th_fill_lcg(unsigned char *buf, size_t len) {
  uint32_t x = 42u;
  size_t i;
  for (i = 0; i < len; i++) {
    x = (x * 1103515245u + 12345u) & 0x7FFFFFFFu;
    buf[i] = (unsigned char)((x >> 16) & 0xFFu);
  }
}

#define CHECK(cond) th_check((cond) != 0, #cond, __FILE__, __LINE__)
#define CHECK_EQ_U(actual, expected)                                                    \
  th_check_u64((unsigned long long)(actual), (unsigned long long)(expected),            \
               #actual " == " #expected, __FILE__, __LINE__)
#define CHECK_EQ_I(actual, expected)                                          \
  th_check_i64((long long)(actual), (long long)(expected), #actual " == " #expected, \
               __FILE__, __LINE__)
#define CHECK_STR(actual, expected) \
  th_check_str((actual), (expected), #actual " == " #expected, __FILE__, __LINE__)
#define CHECK_MEM(actual, expected, len) \
  th_check_mem((actual), (expected), (len), #actual " == " #expected, __FILE__, __LINE__)

#endif
