#include "ciutil.h"

#include <stdlib.h>

const char *ciutil_status_message(ciutil_status status) {
  switch (status) {
    case CIUTIL_OK:
      return "ok";
    case CIUTIL_ERR_INVALID_ARG:
      return "invalid argument";
    case CIUTIL_ERR_NO_MEMORY:
      return "out of memory";
    case CIUTIL_ERR_BUFFER_TOO_SMALL:
      return "output buffer too small";
    case CIUTIL_ERR_BASE64_LENGTH:
      return "invalid base64 length";
    case CIUTIL_ERR_BASE64_CHAR:
      return "invalid base64 character";
    case CIUTIL_ERR_BASE64_PADDING:
      return "invalid base64 padding";
    case CIUTIL_ERR_BASE64_TRAILING_BITS:
      return "non-canonical base64 trailing bits";
    case CIUTIL_ERR_ROMAN_RANGE:
      return "roman numeral out of range (1..3999)";
    case CIUTIL_ERR_ROMAN_SYNTAX:
      return "invalid roman numeral";
    case CIUTIL_ERR_SEMVER_SYNTAX:
      return "invalid semantic version";
    case CIUTIL_ERR_SEMVER_OVERFLOW:
      return "semantic version number too large";
  }
  return "unknown error";
}

void ciutil_free(void *ptr) {
  free(ptr);
}
