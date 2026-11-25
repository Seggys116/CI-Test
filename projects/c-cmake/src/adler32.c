#include "ciutil.h"

#define ADLER_MOD 65521u
/* largest n such that 255*n*(n+1)/2 + (n+1)*(ADLER_MOD-1) fits in 32 bits */
#define ADLER_NMAX 5552u

uint32_t ciutil_adler32_init(void) {
  return 1u;
}

uint32_t ciutil_adler32_update(uint32_t state, const void *data, size_t len) {
  const unsigned char *p = (const unsigned char *)data;
  uint32_t a = state & 0xFFFFu;
  uint32_t b = (state >> 16) & 0xFFFFu;

  while (len > 0) {
    size_t n = len < ADLER_NMAX ? len : ADLER_NMAX;
    size_t i;
    for (i = 0; i < n; i++) {
      a += (uint32_t)p[i];
      b += a;
    }
    a %= ADLER_MOD;
    b %= ADLER_MOD;
    p += n;
    len -= n;
  }
  return (b << 16) | a;
}

uint32_t ciutil_adler32(const void *data, size_t len) {
  return ciutil_adler32_update(ciutil_adler32_init(), data, len);
}
