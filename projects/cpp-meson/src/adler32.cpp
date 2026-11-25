#include "ciutil/adler32.hpp"

#include <algorithm>
#include <cstddef>

namespace ciutil {

namespace {

constexpr std::uint32_t kModulus = 65521u;
// largest block for which the unreduced sums cannot overflow 32 bits
constexpr std::size_t kNmax = 5552;

}  // namespace

void Adler32::update(ByteView data) noexcept {
  std::uint32_t a = a_;
  std::uint32_t b = b_;
  std::size_t offset = 0;

  while (offset < data.size()) {
    const std::size_t n = std::min(data.size() - offset, kNmax);
    for (std::size_t i = 0; i < n; ++i) {
      a += static_cast<std::uint32_t>(data[offset + i]);
      b += a;
    }
    a %= kModulus;
    b %= kModulus;
    offset += n;
  }
  a_ = a;
  b_ = b;
}

std::uint32_t Adler32::value() const noexcept {
  return (b_ << 16) | a_;
}

void Adler32::reset() noexcept {
  a_ = 1;
  b_ = 0;
}

std::uint32_t adler32(ByteView data) noexcept {
  Adler32 sum;
  sum.update(data);
  return sum.value();
}

}  // namespace ciutil
