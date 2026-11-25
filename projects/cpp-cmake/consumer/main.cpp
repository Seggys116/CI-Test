#include <cstdio>

#include <ciutil/crc32.hpp>

int main() {
  const unsigned int crc = ciutil::crc32(ciutil::to_bytes("123456789"));
  std::printf("%08x\n", crc);
  return 0;
}
