#include "ciutil.h"

#include <stdlib.h>

static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* largest input whose padded encoding plus a nul still fits in size_t */
#define MAX_ENCODE_INPUT ((SIZE_MAX - 8u) / 4u * 3u)

size_t ciutil_base64_encoded_len(size_t len) {
  if (len > MAX_ENCODE_INPUT) {
    return SIZE_MAX;
  }
  return (len / 3u + (len % 3u != 0u ? 1u : 0u)) * 4u;
}

ciutil_status ciutil_base64_encode_buf(const void *data, size_t len, char *out, size_t cap,
                                       size_t *written) {
  const unsigned char *p = (const unsigned char *)data;
  size_t need;
  size_t i;
  size_t o = 0;
  size_t rest;
  uint32_t n;

  if (written == NULL || out == NULL || (data == NULL && len != 0)) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  *written = 0;
  need = ciutil_base64_encoded_len(len);
  if (need == SIZE_MAX) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  if (cap <= need) {
    return CIUTIL_ERR_BUFFER_TOO_SMALL;
  }

  for (i = 0; len - i >= 3u; i += 3u) {
    n = ((uint32_t)p[i] << 16) | ((uint32_t)p[i + 1u] << 8) | (uint32_t)p[i + 2u];
    out[o++] = alphabet[(n >> 18) & 63u];
    out[o++] = alphabet[(n >> 12) & 63u];
    out[o++] = alphabet[(n >> 6) & 63u];
    out[o++] = alphabet[n & 63u];
  }

  rest = len - i;
  if (rest == 1u) {
    n = (uint32_t)p[i] << 16;
    out[o++] = alphabet[(n >> 18) & 63u];
    out[o++] = alphabet[(n >> 12) & 63u];
    out[o++] = '=';
    out[o++] = '=';
  } else if (rest == 2u) {
    n = ((uint32_t)p[i] << 16) | ((uint32_t)p[i + 1u] << 8);
    out[o++] = alphabet[(n >> 18) & 63u];
    out[o++] = alphabet[(n >> 12) & 63u];
    out[o++] = alphabet[(n >> 6) & 63u];
    out[o++] = '=';
  }

  out[o] = '\0';
  *written = o;
  return CIUTIL_OK;
}

ciutil_status ciutil_base64_encode(const void *data, size_t len, char **out) {
  size_t need;
  size_t written;
  char *buf;
  ciutil_status status;

  if (out == NULL) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  *out = NULL;
  need = ciutil_base64_encoded_len(len);
  if (need == SIZE_MAX) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  buf = (char *)malloc(need + 1u);
  if (buf == NULL) {
    return CIUTIL_ERR_NO_MEMORY;
  }
  status = ciutil_base64_encode_buf(data, len, buf, need + 1u, &written);
  if (status != CIUTIL_OK) {
    free(buf);
    return status;
  }
  *out = buf;
  return CIUTIL_OK;
}

static int sextet(unsigned char c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A';
  }
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 26;
  }
  if (c >= '0' && c <= '9') {
    return c - '0' + 52;
  }
  if (c == '+') {
    return 62;
  }
  if (c == '/') {
    return 63;
  }
  return -1;
}

ciutil_status ciutil_base64_decode(const char *text, size_t len, uint8_t *out, size_t cap,
                                   size_t *written) {
  size_t pad = 0;
  size_t needed;
  size_t i;
  size_t k;
  size_t o = 0;

  if (written == NULL || (text == NULL && len != 0) || (out == NULL && cap != 0)) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  *written = 0;
  if (len % 4u != 0u) {
    return CIUTIL_ERR_BASE64_LENGTH;
  }
  if (len == 0) {
    return CIUTIL_OK;
  }

  if (text[len - 1u] == '=') {
    pad = (text[len - 2u] == '=') ? 2u : 1u;
  }
  needed = len / 4u * 3u - pad;
  if (cap < needed) {
    return CIUTIL_ERR_BUFFER_TOO_SMALL;
  }

  for (i = 0; i < len; i += 4u) {
    int last = (len - i == 4u);
    uint32_t v[4] = {0u, 0u, 0u, 0u};
    size_t npad = 0;
    uint32_t n;

    for (k = 0; k < 4u; k++) {
      unsigned char c = (unsigned char)text[i + k];
      if (c == '=') {
        if (!last || k < 2u) {
          return CIUTIL_ERR_BASE64_PADDING;
        }
        npad++;
      } else {
        int d;
        if (npad != 0) {
          return CIUTIL_ERR_BASE64_PADDING;
        }
        d = sextet(c);
        if (d < 0) {
          return CIUTIL_ERR_BASE64_CHAR;
        }
        v[k] = (uint32_t)d;
      }
    }

    if ((npad == 1u && (v[2] & 3u) != 0u) || (npad == 2u && (v[1] & 15u) != 0u)) {
      return CIUTIL_ERR_BASE64_TRAILING_BITS;
    }

    n = (v[0] << 18) | (v[1] << 12) | (v[2] << 6) | v[3];
    out[o++] = (uint8_t)((n >> 16) & 0xFFu);
    if (npad < 2u) {
      out[o++] = (uint8_t)((n >> 8) & 0xFFu);
    }
    if (npad < 1u) {
      out[o++] = (uint8_t)(n & 0xFFu);
    }
  }

  *written = o;
  return CIUTIL_OK;
}

ciutil_status ciutil_base64_decode_alloc(const char *text, size_t len, uint8_t **out,
                                         size_t *out_len) {
  size_t cap;
  size_t written;
  uint8_t *buf;
  ciutil_status status;

  if (out == NULL || out_len == NULL || (text == NULL && len != 0)) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  *out = NULL;
  *out_len = 0;
  cap = len / 4u * 3u;
  buf = (uint8_t *)malloc(cap > 0 ? cap : 1u);
  if (buf == NULL) {
    return CIUTIL_ERR_NO_MEMORY;
  }
  status = ciutil_base64_decode(text, len, buf, cap, &written);
  if (status != CIUTIL_OK) {
    free(buf);
    return status;
  }
  *out = buf;
  *out_len = written;
  return CIUTIL_OK;
}
