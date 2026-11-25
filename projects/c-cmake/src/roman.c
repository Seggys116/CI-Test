#include "ciutil.h"

#include <string.h>

typedef struct roman_entry {
  unsigned int value;
  const char *symbol;
} roman_entry;

static const roman_entry entries[] = {
    {1000u, "M"}, {900u, "CM"}, {500u, "D"}, {400u, "CD"}, {100u, "C"},
    {90u, "XC"},  {50u, "L"},   {40u, "XL"}, {10u, "X"},   {9u, "IX"},
    {5u, "V"},    {4u, "IV"},   {1u, "I"}};

#define ENTRY_COUNT (sizeof entries / sizeof entries[0])

ciutil_status ciutil_roman_encode(int value, char *out, size_t cap, size_t *written) {
  unsigned int rest;
  size_t o = 0;
  size_t e;

  if (out == NULL || written == NULL) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  *written = 0;
  if (value < 1 || value > 3999) {
    return CIUTIL_ERR_ROMAN_RANGE;
  }
  if (cap < CIUTIL_ROMAN_MAX_LEN + 1u) {
    return CIUTIL_ERR_BUFFER_TOO_SMALL;
  }

  rest = (unsigned int)value;
  for (e = 0; e < ENTRY_COUNT; e++) {
    while (rest >= entries[e].value) {
      const char *s = entries[e].symbol;
      while (*s != '\0') {
        out[o++] = *s++;
      }
      rest -= entries[e].value;
    }
  }
  out[o] = '\0';
  *written = o;
  return CIUTIL_OK;
}

/* parses greedily without regard to canonical form, then insists that the
   canonical spelling of the parsed value is exactly the input */
ciutil_status ciutil_roman_decode(const char *text, unsigned int *value) {
  size_t pos = 0;
  size_t e;
  unsigned int total = 0;
  char canonical[CIUTIL_ROMAN_MAX_LEN + 1];
  size_t canonical_len;
  ciutil_status status;

  if (text == NULL || value == NULL) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  if (text[0] == '\0') {
    return CIUTIL_ERR_ROMAN_SYNTAX;
  }

  for (e = 0; e < ENTRY_COUNT; e++) {
    size_t n = strlen(entries[e].symbol);
    while (strncmp(text + pos, entries[e].symbol, n) == 0) {
      total += entries[e].value;
      if (total > 3999u) {
        return CIUTIL_ERR_ROMAN_RANGE;
      }
      pos += n;
    }
  }
  if (text[pos] != '\0') {
    return CIUTIL_ERR_ROMAN_SYNTAX;
  }

  status = ciutil_roman_encode((int)total, canonical, sizeof canonical, &canonical_len);
  if (status != CIUTIL_OK) {
    return CIUTIL_ERR_ROMAN_SYNTAX;
  }
  if (canonical_len != pos || memcmp(canonical, text, pos) != 0) {
    return CIUTIL_ERR_ROMAN_SYNTAX;
  }
  *value = total;
  return CIUTIL_OK;
}
