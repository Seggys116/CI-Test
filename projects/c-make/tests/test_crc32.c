#include "ciutil.h"
#include "harness.h"

typedef struct vector {
  const char *text;
  uint32_t expected;
} vector;

static const vector vectors[] = {
    {"", 0x00000000u},
    {"a", 0xe8b7be43u},
    {"abc", 0x352441c2u},
    {"123456789", 0xcbf43926u},
    {"The quick brown fox jumps over the lazy dog", 0x414fa339u},
};

/* bitwise reference, independent of the table in the library */
static uint32_t reference_crc32(const unsigned char *data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  size_t i;
  int bit;
  for (i = 0; i < len; i++) {
    crc ^= (uint32_t)data[i];
    for (bit = 0; bit < 8; bit++) {
      crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

static void test_vectors(void) {
  size_t i;
  for (i = 0; i < sizeof vectors / sizeof vectors[0]; i++) {
    CHECK_EQ_U(ciutil_crc32(vectors[i].text, strlen(vectors[i].text)), vectors[i].expected);
  }
}

static void test_fixed_byte_vectors(void) {
  unsigned char zeros[32];
  unsigned char ones[32];
  memset(zeros, 0x00, sizeof zeros);
  memset(ones, 0xFF, sizeof ones);
  CHECK_EQ_U(ciutil_crc32(zeros, sizeof zeros), 0x190a55adu);
  CHECK_EQ_U(ciutil_crc32(ones, sizeof ones), 0xff6cab0bu);
}

static void test_empty_with_null_pointer(void) {
  CHECK_EQ_U(ciutil_crc32(NULL, 0), 0x00000000u);
}

static void test_incremental_matches_one_shot_at_every_split(void) {
  unsigned char data[1000];
  uint32_t whole;
  size_t split;

  th_fill_lcg(data, sizeof data);
  whole = ciutil_crc32(data, sizeof data);
  for (split = 0; split <= sizeof data; split++) {
    uint32_t state = ciutil_crc32_init();
    state = ciutil_crc32_update(state, data, split);
    state = ciutil_crc32_update(state, data + split, sizeof data - split);
    CHECK_EQ_U(ciutil_crc32_final(state), whole);
  }
}

static void test_incremental_byte_at_a_time(void) {
  unsigned char data[1000];
  uint32_t state = ciutil_crc32_init();
  size_t i;

  th_fill_lcg(data, sizeof data);
  for (i = 0; i < sizeof data; i++) {
    state = ciutil_crc32_update(state, data + i, 1);
  }
  CHECK_EQ_U(ciutil_crc32_final(state), ciutil_crc32(data, sizeof data));
}

static void test_matches_bitwise_reference_for_every_prefix(void) {
  unsigned char data[1000];
  size_t len;

  th_fill_lcg(data, sizeof data);
  for (len = 0; len <= sizeof data; len++) {
    CHECK_EQ_U(ciutil_crc32(data, len), reference_crc32(data, len));
  }
}

static void test_every_single_byte_matches_reference(void) {
  unsigned int b;
  for (b = 0; b < 256u; b++) {
    unsigned char byte = (unsigned char)b;
    CHECK_EQ_U(ciutil_crc32(&byte, 1), reference_crc32(&byte, 1));
  }
}

int main(void) {
  test_vectors();
  test_fixed_byte_vectors();
  test_empty_with_null_pointer();
  test_incremental_matches_one_shot_at_every_split();
  test_incremental_byte_at_a_time();
  test_matches_bitwise_reference_for_every_prefix();
  test_every_single_byte_matches_reference();
  return th_finish();
}
