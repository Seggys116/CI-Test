#ifndef CIUTIL_BYTES_HPP
#define CIUTIL_BYTES_HPP

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ciutil {

using ByteSpan = std::span<const std::uint8_t>;
using Bytes = std::vector<std::uint8_t>;

// views the characters of text as raw bytes, no encoding conversion
[[nodiscard]] inline ByteSpan to_bytes(std::string_view text) noexcept {
  return ByteSpan(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
}

}  // namespace ciutil

#endif
