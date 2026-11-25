#ifndef CIUTIL_TEST_UTIL_HPP
#define CIUTIL_TEST_UTIL_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "ciutil/bytes.hpp"

namespace ciutil::testing_util {

// deterministic bytes: x = x * 1103515245 + 12345 mod 2^31, byte = (x >> 16) & 0xff, seed 42
inline Bytes lcg_bytes(std::size_t count) {
  Bytes out;
  out.reserve(count);
  std::uint32_t x = 42u;
  for (std::size_t i = 0; i < count; ++i) {
    x = (x * 1103515245u + 12345u) & 0x7FFFFFFFu;
    out.push_back(static_cast<std::uint8_t>((x >> 16) & 0xFFu));
  }
  return out;
}

inline std::string hex8(std::uint32_t value) {
  constexpr std::string_view digits = "0123456789abcdef";
  std::string out(8, '0');
  for (std::size_t i = out.size(); i-- > 0;) {
    out[i] = digits[value & 0xFu];
    value >>= 4;
  }
  return out;
}

}  // namespace ciutil::testing_util

#endif
