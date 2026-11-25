#include "ciutil/base64.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ciutil {

namespace {

constexpr std::string_view kAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// returns the 6 bit value of a base64 character, or -1
constexpr int sextet(char c) noexcept {
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

constexpr std::uint32_t widen(std::uint8_t byte) noexcept {
  return static_cast<std::uint32_t>(byte);
}

}  // namespace

std::string base64_encode(ByteSpan data) {
  std::string out;
  out.reserve((data.size() + 2) / 3 * 4);

  std::size_t i = 0;
  for (; data.size() - i >= 3; i += 3) {
    const std::uint32_t n = (widen(data[i]) << 16) | (widen(data[i + 1]) << 8) | widen(data[i + 2]);
    out.push_back(kAlphabet[(n >> 18) & 63u]);
    out.push_back(kAlphabet[(n >> 12) & 63u]);
    out.push_back(kAlphabet[(n >> 6) & 63u]);
    out.push_back(kAlphabet[n & 63u]);
  }

  const std::size_t rest = data.size() - i;
  if (rest == 1) {
    const std::uint32_t n = widen(data[i]) << 16;
    out.push_back(kAlphabet[(n >> 18) & 63u]);
    out.push_back(kAlphabet[(n >> 12) & 63u]);
    out.append("==");
  } else if (rest == 2) {
    const std::uint32_t n = (widen(data[i]) << 16) | (widen(data[i + 1]) << 8);
    out.push_back(kAlphabet[(n >> 18) & 63u]);
    out.push_back(kAlphabet[(n >> 12) & 63u]);
    out.push_back(kAlphabet[(n >> 6) & 63u]);
    out.push_back('=');
  }
  return out;
}

Result<Bytes> base64_decode(std::string_view text) {
  if (text.size() % 4 != 0) {
    return Error::InvalidLength;
  }

  Bytes out;
  out.reserve(text.size() / 4 * 3);

  for (std::size_t i = 0; i < text.size(); i += 4) {
    const bool last = text.size() - i == 4;
    std::array<std::uint32_t, 4> v{};
    std::size_t padding = 0;

    for (std::size_t k = 0; k < 4; ++k) {
      const char c = text[i + k];
      if (c == '=') {
        if (!last || k < 2) {
          return Error::InvalidPadding;
        }
        ++padding;
      } else {
        if (padding != 0) {
          return Error::InvalidPadding;
        }
        const int d = sextet(c);
        if (d < 0) {
          return Error::InvalidCharacter;
        }
        v[k] = static_cast<std::uint32_t>(d);
      }
    }

    if ((padding == 1 && (v[2] & 3u) != 0) || (padding == 2 && (v[1] & 15u) != 0)) {
      return Error::NonCanonicalBits;
    }

    const std::uint32_t n = (v[0] << 18) | (v[1] << 12) | (v[2] << 6) | v[3];
    out.push_back(static_cast<std::uint8_t>((n >> 16) & 0xFFu));
    if (padding < 2) {
      out.push_back(static_cast<std::uint8_t>((n >> 8) & 0xFFu));
    }
    if (padding < 1) {
      out.push_back(static_cast<std::uint8_t>(n & 0xFFu));
    }
  }
  return out;
}

}  // namespace ciutil
