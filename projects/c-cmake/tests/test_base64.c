#include <stdlib.h>

#include "ciutil.h"
#include "harness.h"

typedef struct vector {
  const char *plain;
  const char *encoded;
} vector;

static const vector vectors[] = {
    {"", ""},
    {"f", "Zg=="},
    {"fo", "Zm8="},
    {"foo", "Zm9v"},
    {"foob", "Zm9vYg=="},
    {"fooba", "Zm9vYmE="},
    {"foobar", "Zm9vYmFy"},
};

static const char *const invalid_inputs[] = {
    "Z",          /* length not a multiple of 4 */
    "Zg",         /* missing padding */
    "Zg=",        /* short padding */
    "Zm9vY",      /* length 5 */
    "Zg===",      /* length 5 with three pads */
    "Z===",       /* three pads */
    "====",       /* only pads */
    "=Zg=",       /* pad first */
    "Zg==Zg==",   /* padding in the middle */
    "Zm=9",       /* data after pad */
    "Zg=a",       /* pad then data */
    "Zm9vYg=\n",  /* newline after pad */
    "Zm-v",       /* url-safe minus */
    "Zm_v",       /* url-safe underscore */
    "Zm 9",       /* space */
    "Zm9\n",      /* newline */
    "Zm9v\n",     /* newline, length 5 */
    "Zm9!",       /* punctuation */
    "Zm\xC3\xA9", /* non-ascii bytes */
    "Zh==",       /* non-canonical trailing bits */
    "Zm9=",       /* non-canonical trailing bits */
    "Zm9vZh==",   /* non-canonical bits in last group only */
    "Zm9vZm9=",   /* non-canonical bits in last group only */
};

static void test_encode_vectors(void) {
  size_t i;
  for (i = 0; i < sizeof vectors / sizeof vectors[0]; i++) {
    char *encoded = NULL;
    ciutil_status status =
        ciutil_base64_encode(vectors[i].plain, strlen(vectors[i].plain), &encoded);
    CHECK_EQ_I(status, CIUTIL_OK);
    CHECK_STR(encoded, vectors[i].encoded);
    ciutil_free(encoded);
  }
}

static void test_decode_vectors(void) {
  size_t i;
  for (i = 0; i < sizeof vectors / sizeof vectors[0]; i++) {
    uint8_t *decoded = NULL;
    size_t len = 0;
    ciutil_status status =
        ciutil_base64_decode_alloc(vectors[i].encoded, strlen(vectors[i].encoded), &decoded, &len);
    CHECK_EQ_I(status, CIUTIL_OK);
    CHECK_EQ_U(len, strlen(vectors[i].plain));
    CHECK(decoded != NULL);
    if (decoded != NULL) {
      CHECK_MEM(decoded, vectors[i].plain, len);
    }
    ciutil_free(decoded);
  }
}

static void test_binary_vector(void) {
  static const uint8_t raw[] = {0x00, 0xFF, 0xFE};
  char *encoded = NULL;
  uint8_t *decoded = NULL;
  size_t len = 0;

  CHECK_EQ_I(ciutil_base64_encode(raw, sizeof raw, &encoded), CIUTIL_OK);
  CHECK_STR(encoded, "AP/+");
  ciutil_free(encoded);

  CHECK_EQ_I(ciutil_base64_decode_alloc("AP/+", 4, &decoded, &len), CIUTIL_OK);
  CHECK_EQ_U(len, sizeof raw);
  if (decoded != NULL) {
    CHECK_MEM(decoded, raw, sizeof raw);
  }
  ciutil_free(decoded);
}

static void test_rejects_invalid_input(void) {
  size_t i;
  for (i = 0; i < sizeof invalid_inputs / sizeof invalid_inputs[0]; i++) {
    uint8_t *decoded = NULL;
    size_t len = 99;
    ciutil_status status =
        ciutil_base64_decode_alloc(invalid_inputs[i], strlen(invalid_inputs[i]), &decoded, &len);
    CHECK(status != CIUTIL_OK);
    CHECK(decoded == NULL);
    CHECK_EQ_U(len, 0);
    ciutil_free(decoded);
  }
}

static void test_specific_error_codes(void) {
  uint8_t out[16];
  size_t len = 0;

  CHECK_EQ_I(ciutil_base64_decode("Zg=", 3, out, sizeof out, &len), CIUTIL_ERR_BASE64_LENGTH);
  CHECK_EQ_I(ciutil_base64_decode("Zm-v", 4, out, sizeof out, &len), CIUTIL_ERR_BASE64_CHAR);
  CHECK_EQ_I(ciutil_base64_decode("Zm_v", 4, out, sizeof out, &len), CIUTIL_ERR_BASE64_CHAR);
  CHECK_EQ_I(ciutil_base64_decode("Zm 9", 4, out, sizeof out, &len), CIUTIL_ERR_BASE64_CHAR);
  CHECK_EQ_I(ciutil_base64_decode("Zm9\n", 4, out, sizeof out, &len), CIUTIL_ERR_BASE64_CHAR);
  CHECK_EQ_I(ciutil_base64_decode("Zg==Zg==", 8, out, sizeof out, &len),
             CIUTIL_ERR_BASE64_PADDING);
  CHECK_EQ_I(ciutil_base64_decode("Z===", 4, out, sizeof out, &len), CIUTIL_ERR_BASE64_PADDING);
  CHECK_EQ_I(ciutil_base64_decode("Zh==", 4, out, sizeof out, &len),
             CIUTIL_ERR_BASE64_TRAILING_BITS);
  CHECK_EQ_I(ciutil_base64_decode("Zm9=", 4, out, sizeof out, &len),
             CIUTIL_ERR_BASE64_TRAILING_BITS);
}

static void test_embedded_nul_is_rejected(void) {
  static const char text[] = {'Z', 'g', '\0', '='};
  uint8_t out[8];
  size_t len = 0;
  CHECK_EQ_I(ciutil_base64_decode(text, sizeof text, out, sizeof out, &len),
             CIUTIL_ERR_BASE64_CHAR);
}

static void test_round_trip_all_lengths(void) {
  unsigned char data[300];
  size_t n;

  th_fill_lcg(data, sizeof data);
  for (n = 0; n <= sizeof data; n++) {
    char *encoded = NULL;
    uint8_t *decoded = NULL;
    size_t len = 0;

    CHECK_EQ_I(ciutil_base64_encode(data, n, &encoded), CIUTIL_OK);
    if (encoded == NULL) {
      continue;
    }
    CHECK_EQ_U(strlen(encoded), ciutil_base64_encoded_len(n));
    CHECK_EQ_I(ciutil_base64_decode_alloc(encoded, strlen(encoded), &decoded, &len), CIUTIL_OK);
    CHECK_EQ_U(len, n);
    if (decoded != NULL) {
      CHECK_MEM(decoded, data, n);
    }
    ciutil_free(decoded);
    ciutil_free(encoded);
  }
}

static void test_encoded_len(void) {
  CHECK_EQ_U(ciutil_base64_encoded_len(0), 0);
  CHECK_EQ_U(ciutil_base64_encoded_len(1), 4);
  CHECK_EQ_U(ciutil_base64_encoded_len(2), 4);
  CHECK_EQ_U(ciutil_base64_encoded_len(3), 4);
  CHECK_EQ_U(ciutil_base64_encoded_len(4), 8);
  CHECK(ciutil_base64_encoded_len(SIZE_MAX) == SIZE_MAX);
}

static void test_encode_buffer_sizes(void) {
  char out[16];
  size_t written = 0;

  CHECK_EQ_I(ciutil_base64_encode_buf("foo", 3, out, 5, &written), CIUTIL_OK);
  CHECK_EQ_U(written, 4);
  CHECK_STR(out, "Zm9v");
  CHECK_EQ_I(ciutil_base64_encode_buf("foo", 3, out, 4, &written), CIUTIL_ERR_BUFFER_TOO_SMALL);
  CHECK_EQ_U(written, 0);
  CHECK_EQ_I(ciutil_base64_encode_buf("", 0, out, 1, &written), CIUTIL_OK);
  CHECK_STR(out, "");
  CHECK_EQ_I(ciutil_base64_encode_buf("", 0, out, 0, &written), CIUTIL_ERR_BUFFER_TOO_SMALL);
}

static void test_decode_buffer_sizes(void) {
  uint8_t out[8];
  size_t written = 0;

  CHECK_EQ_I(ciutil_base64_decode("Zm9v", 4, out, 3, &written), CIUTIL_OK);
  CHECK_EQ_U(written, 3);
  CHECK_MEM(out, "foo", 3);
  CHECK_EQ_I(ciutil_base64_decode("Zm9v", 4, out, 2, &written), CIUTIL_ERR_BUFFER_TOO_SMALL);
  CHECK_EQ_I(ciutil_base64_decode("Zg==", 4, out, 1, &written), CIUTIL_OK);
  CHECK_EQ_U(written, 1);
  CHECK_EQ_I(ciutil_base64_decode("", 0, NULL, 0, &written), CIUTIL_OK);
  CHECK_EQ_U(written, 0);
}

static void test_invalid_arguments(void) {
  char *encoded = NULL;
  uint8_t *decoded = NULL;
  size_t len = 0;
  char out[8];

  CHECK_EQ_I(ciutil_base64_encode(NULL, 1, &encoded), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_base64_encode("a", 1, NULL), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_base64_encode_buf("a", 1, NULL, 8, &len), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_base64_encode_buf("a", 1, out, sizeof out, NULL), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_base64_decode_alloc(NULL, 4, &decoded, &len), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_base64_decode_alloc("Zg==", 4, NULL, &len), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_base64_decode_alloc("Zg==", 4, &decoded, NULL), CIUTIL_ERR_INVALID_ARG);
  CHECK_EQ_I(ciutil_base64_decode("Zg==", 4, NULL, 4, &len), CIUTIL_ERR_INVALID_ARG);
}

static void test_empty_decode_alloc_is_non_null(void) {
  uint8_t *decoded = NULL;
  size_t len = 1;
  CHECK_EQ_I(ciutil_base64_decode_alloc("", 0, &decoded, &len), CIUTIL_OK);
  CHECK(decoded != NULL);
  CHECK_EQ_U(len, 0);
  ciutil_free(decoded);
}

int main(void) {
  test_encode_vectors();
  test_decode_vectors();
  test_binary_vector();
  test_rejects_invalid_input();
  test_specific_error_codes();
  test_embedded_nul_is_rejected();
  test_round_trip_all_lengths();
  test_encoded_len();
  test_encode_buffer_sizes();
  test_decode_buffer_sizes();
  test_invalid_arguments();
  test_empty_decode_alloc_is_non_null();
  return th_finish();
}
