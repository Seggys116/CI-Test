#include "ciutil.h"

/* crc of a single byte is linear in the byte, so each table entry is the xor of
   the entries for its set bits; these are the entries for 1, 2, 4, ... 128 */
#define CRC_K0 0x77073096u
#define CRC_K1 0xEE0E612Cu
#define CRC_K2 0x076DC419u
#define CRC_K3 0x0EDB8832u
#define CRC_K4 0x1DB71064u
#define CRC_K5 0x3B6E20C8u
#define CRC_K6 0x76DC4190u
#define CRC_K7 0xEDB88320u

#define CRC_L1(x) (x), ((x) ^ CRC_K0)
#define CRC_L2(x) CRC_L1(x), CRC_L1((x) ^ CRC_K1)
#define CRC_L3(x) CRC_L2(x), CRC_L2((x) ^ CRC_K2)
#define CRC_L4(x) CRC_L3(x), CRC_L3((x) ^ CRC_K3)
#define CRC_L5(x) CRC_L4(x), CRC_L4((x) ^ CRC_K4)
#define CRC_L6(x) CRC_L5(x), CRC_L5((x) ^ CRC_K5)
#define CRC_L7(x) CRC_L6(x), CRC_L6((x) ^ CRC_K6)
#define CRC_L8(x) CRC_L7(x), CRC_L7((x) ^ CRC_K7)

static const uint32_t crc_table[256] = {CRC_L8(0u)};

uint32_t ciutil_crc32_init(void) {
  return 0xFFFFFFFFu;
}

uint32_t ciutil_crc32_update(uint32_t state, const void *data, size_t len) {
  const unsigned char *p = (const unsigned char *)data;
  size_t i;
  for (i = 0; i < len; i++) {
    state = crc_table[(state ^ (uint32_t)p[i]) & 0xFFu] ^ (state >> 8);
  }
  return state;
}

uint32_t ciutil_crc32_final(uint32_t state) {
  return state ^ 0xFFFFFFFFu;
}

uint32_t ciutil_crc32(const void *data, size_t len) {
  return ciutil_crc32_final(ciutil_crc32_update(ciutil_crc32_init(), data, len));
}
