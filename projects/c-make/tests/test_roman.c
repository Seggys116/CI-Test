#include <limits.h>

#include "ciutil.h"
#include "harness.h"

typedef struct vector {
  int value;
  const char *roman;
} vector;

static const vector vectors[] = {
    {1, "I"},       {4, "IV"},      {9, "IX"},        {14, "XIV"},      {40, "XL"},
    {90, "XC"},     {400, "CD"},    {1994, "MCMXCIV"}, {2024, "MMXXIV"}, {3999, "MMMCMXCIX"},
    {1999, "MCMXCIX"},
    {2, "II"},      {3, "III"},     {5, "V"},          {10, "X"},        {50, "L"},
    {100, "C"},     {500, "D"},     {1000, "M"},       {3888, "MMMDCCCLXXXVIII"},
};

static const char *const invalid_numerals[] = {
    "",       "IIII",  "VX",    "IC",    "XM",    "VV",    "IIX",   "xiv",   "Xiv",
    "MMMM",   "IL",    "XD",    "VIV",   "CDC",   "IXI",   "MCMC",  "XXXX",  "LL",
    "DD",     "IIIII", " X",    "X ",    "XIV\n", "A",     "IVI",   "VL",    "LC",
    "DM",     "CMC",   "MMMMCMXCIX", "IIV", "XXXXX", "CCCC", "MDM", "XIIII",
};

static void test_encode_vectors(void) {
  size_t i;
  for (i = 0; i < sizeof vectors / sizeof vectors[0]; i++) {
    char buf[CIUTIL_ROMAN_MAX_LEN + 1];
    size_t len = 0;
    CHECK_EQ_I(ciutil_roman_encode(vectors[i].value, buf, sizeof buf, &len), CIUTIL_OK);
    CHECK_STR(buf, vectors[i].roman);
    CHECK_EQ_U(len, strlen(vectors[i].roman));
  }
}

static void test_decode_vectors(void) {
  size_t i;
  for (i = 0; i < sizeof vectors / sizeof vectors[0]; i++) {
    unsigned int value = 0;
    CHECK_EQ_I(ciutil_roman_decode(vectors[i].roman, &value), CIUTIL_OK);
    CHECK_EQ_U(value, vectors[i].value);
  }
}

static void test_encode_rejects_out_of_range(void) {
  char buf[CIUTIL_ROMAN_MAX_LEN + 1];
  size_t len = 7;
  CHECK_EQ_I(ciutil_roman_encode(0, buf, sizeof buf, &len), CIUTIL_ERR_ROMAN_RANGE);
  CHECK_EQ_U(len, 0);
  CHECK_EQ_I(ciutil_roman_encode(4000, buf, sizeof buf, &len), CIUTIL_ERR_ROMAN_RANGE);
  CHECK_EQ_I(ciutil_roman_encode(-1, buf, sizeof buf, &len), CIUTIL_ERR_ROMAN_RANGE);
  CHECK_EQ_I(ciutil_roman_encode(-3999, buf, sizeof buf, &len), CIUTIL_ERR_ROMAN_RANGE);
  CHECK_EQ_I(ciutil_roman_encode(INT_MAX, buf, sizeof buf, &len), CIUTIL_ERR_ROMAN_RANGE);
  CHECK_EQ_I(ciutil_roman_encode(INT_MIN, buf, sizeof buf, &len), CIUTIL_ERR_ROMAN_RANGE);
}

static void test_encode_buffer_limits(void) {
  char buf[CIUTIL_ROMAN_MAX_LEN + 1];
  size_t len = 0;
  CHECK_EQ_I(ciutil_roman_encode(3888, buf, CIUTIL_ROMAN_MAX_LEN, &len),
             CIUTIL_ERR_BUFFER_TOO_SMALL);
  CHECK_EQ_I(ciutil_roman_encode(3888, buf, CIUTIL_ROMAN_MAX_LEN + 1, &len), CIUTIL_OK);
  CHECK_EQ_U(len, CIUTIL_ROMAN_MAX_LEN);
  CHECK_EQ_I(ciutil_roman_encode(1, NULL, 16, &len), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_roman_encode(1, buf, sizeof buf, NULL), CIUTIL_ERR_INVALID_ARG);
}

static void test_decode_rejects_non_canonical(void) {
  size_t i;
  for (i = 0; i < sizeof invalid_numerals / sizeof invalid_numerals[0]; i++) {
    unsigned int value = 12345u;
    CHECK(ciutil_roman_decode(invalid_numerals[i], &value) != CIUTIL_OK);
    CHECK_EQ_U(value, 12345u);
  }
}

static void test_decode_invalid_arguments(void) {
  unsigned int value = 0;
  CHECK_EQ_I(ciutil_roman_decode(NULL, &value), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_roman_decode("I", NULL), CIUTIL_ERR_INVALID_ARG);
}

static void test_exhaustive_round_trip(void) {
  int n;
  for (n = 1; n <= 3999; n++) {
    char buf[CIUTIL_ROMAN_MAX_LEN + 1];
    size_t len = 0;
    unsigned int back = 0;
    ciutil_status status = ciutil_roman_encode(n, buf, sizeof buf, &len);
    CHECK_EQ_I(status, CIUTIL_OK);
    if (status != CIUTIL_OK) {
      continue;
    }
    CHECK_EQ_U(len, strlen(buf));
    CHECK_EQ_I(ciutil_roman_decode(buf, &back), CIUTIL_OK);
    CHECK_EQ_U(back, n);
  }
}

static void test_encodings_are_distinct_from_neighbours(void) {
  int n;
  char previous[CIUTIL_ROMAN_MAX_LEN + 1] = "";
  for (n = 1; n <= 3999; n++) {
    char buf[CIUTIL_ROMAN_MAX_LEN + 1];
    size_t len = 0;
    if (ciutil_roman_encode(n, buf, sizeof buf, &len) != CIUTIL_OK) {
      CHECK(0);
      return;
    }
    CHECK(strcmp(buf, previous) != 0);
    memcpy(previous, buf, len + 1u);
  }
}

int main(void) {
  test_encode_vectors();
  test_decode_vectors();
  test_encode_rejects_out_of_range();
  test_encode_buffer_limits();
  test_decode_rejects_non_canonical();
  test_decode_invalid_arguments();
  test_exhaustive_round_trip();
  test_encodings_are_distinct_from_neighbours();
  return th_finish();
}
