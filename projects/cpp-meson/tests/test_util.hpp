#ifndef CIUTIL_TEST_UTIL_HPP
#define CIUTIL_TEST_UTIL_HPP

#include <cstddef>
#include <cstdint>

#include "ciutil/bytes.hpp"

namespace testutil {

// deterministic bytes: x = x * 1103515245 + 12345 mod 2^31, byte = (x >> 16) & 0xff, seed 42
inline ciutil::Bytes lcg_bytes(std::size_t count) {
  ciutil::Bytes out;
  out.reserve(count);
  std::uint32_t x = 42u;
  for (std::size_t i = 0; i < count; ++i) {
    x = (x * 1103515245u + 12345u) & 0x7FFFFFFFu;
    out.push_back(static_cast<std::uint8_t>((x >> 16) & 0xFFu));
  }
  return out;
}

}  // namespace testutil

#endif
