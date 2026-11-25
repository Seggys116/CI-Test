#include <stdlib.h>

#include "ciutil.h"
#include "harness.h"

typedef struct vector {
  const char *text;
  uint32_t expected;
} vector;

static const vector vectors[] = {
    {"", 0x00000001u},
    {"a", 0x00620062u},
    {"abc", 0x024d0127u},
    {"Wikipedia", 0x11e60398u},
    {"123456789", 0x091e01deu},
};

/* naive reference: modulo after every byte */
static uint32_t reference_adler32(const unsigned char *data, size_t len) {
  uint32_t a = 1u;
  uint32_t b = 0u;
  size_t i;
  for (i = 0; i < len; i++) {
    a = (a + (uint32_t)data[i]) % 65521u;
    b = (b + a) % 65521u;
  }
  return (b << 16) | a;
}

static void test_vectors(void) {
  size_t i;
  for (i = 0; i < sizeof vectors / sizeof vectors[0]; i++) {
    CHECK_EQ_U(ciutil_adler32(vectors[i].text, strlen(vectors[i].text)), vectors[i].expected);
  }
}

static void test_empty_with_null_pointer(void) {
  CHECK_EQ_U(ciutil_adler32(NULL, 0), 1u);
}

static void test_long_run_of_ff_matches_reference(void) {
  unsigned char *data = (unsigned char *)malloc(100000u);
  uint32_t expected;

  CHECK(data != NULL);
  if (data == NULL) {
    return;
  }
  memset(data, 0xFF, 100000u);
  expected = reference_adler32(data, 100000u);
  CHECK_EQ_U(ciutil_adler32(data, 100000u), expected);
  free(data);
}

static void test_lengths_around_nmax_match_reference(void) {
  static const size_t lengths[] = {5550u, 5551u, 5552u, 5553u, 5554u, 11103u, 11104u, 11105u};
  unsigned char *data = (unsigned char *)malloc(12000u);
  size_t i;

  CHECK(data != NULL);
  if (data == NULL) {
    return;
  }
  memset(data, 0xFF, 12000u);
  for (i = 0; i < sizeof lengths / sizeof lengths[0]; i++) {
    CHECK_EQ_U(ciutil_adler32(data, lengths[i]), reference_adler32(data, lengths[i]));
  }
  th_fill_lcg(data, 12000u);
  for (i = 0; i < sizeof lengths / sizeof lengths[0]; i++) {
    CHECK_EQ_U(ciutil_adler32(data, lengths[i]), reference_adler32(data, lengths[i]));
  }
  free(data);
}

static void test_incremental_matches_one_shot_at_every_split(void) {
  unsigned char data[1000];
  uint32_t whole;
  size_t split;

  th_fill_lcg(data, sizeof data);
  whole = ciutil_adler32(data, sizeof data);
  CHECK_EQ_U(whole, reference_adler32(data, sizeof data));
  for (split = 0; split <= sizeof data; split++) {
    uint32_t state = ciutil_adler32_init();
    state = ciutil_adler32_update(state, data, split);
    state = ciutil_adler32_update(state, data + split, sizeof data - split);
    CHECK_EQ_U(state, whole);
  }
}

static void test_incremental_across_nmax_boundary(void) {
  static const size_t splits[] = {1u, 5551u, 5552u, 5553u, 11104u, 50000u, 99999u};
  unsigned char *data = (unsigned char *)malloc(100000u);
  uint32_t whole;
  size_t i;

  CHECK(data != NULL);
  if (data == NULL) {
    return;
  }
  memset(data, 0xFF, 100000u);
  whole = ciutil_adler32(data, 100000u);
  for (i = 0; i < sizeof splits / sizeof splits[0]; i++) {
    uint32_t state = ciutil_adler32_init();
    state = ciutil_adler32_update(state, data, splits[i]);
    state = ciutil_adler32_update(state, data + splits[i], 100000u - splits[i]);
    CHECK_EQ_U(state, whole);
  }
  free(data);
}

int main(void) {
  test_vectors();
  test_empty_with_null_pointer();
  test_long_run_of_ff_matches_reference();
  test_lengths_around_nmax_match_reference();
  test_incremental_matches_one_shot_at_every_split();
  test_incremental_across_nmax_boundary();
  return th_finish();
}
