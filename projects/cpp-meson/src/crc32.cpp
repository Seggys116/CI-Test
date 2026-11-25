#include "ciutil/crc32.hpp"

#include <array>
#include <cstddef>

namespace ciutil {

namespace {

constexpr std::uint32_t kPolynomial = 0xEDB88320u;
constexpr std::uint32_t kInitial = 0xFFFFFFFFu;

constexpr std::array<std::uint32_t, 256> make_table() noexcept {
  std::array<std::uint32_t, 256> table{};
  for (std::size_t i = 0; i < table.size(); ++i) {
    std::uint32_t c = static_cast<std::uint32_t>(i);
    for (int bit = 0; bit < 8; ++bit) {
      c = (c & 1u) != 0u ? (kPolynomial ^ (c >> 1)) : (c >> 1);
    }
    table[i] = c;
  }
  return table;
}

constexpr std::array<std::uint32_t, 256> kTable = make_table();

}  // namespace

void Crc32::update(ByteView data) noexcept {
  std::uint32_t state = state_;
  for (const std::uint8_t byte : data) {
    state = kTable[(state ^ static_cast<std::uint32_t>(byte)) & 0xFFu] ^ (state >> 8);
  }
  state_ = state;
}

std::uint32_t Crc32::value() const noexcept {
  return state_ ^ kInitial;
}

void Crc32::reset() noexcept {
  state_ = kInitial;
}

std::uint32_t crc32(ByteView data) noexcept {
  Crc32 crc;
  crc.update(data);
  return crc.value();
}

}  // namespace ciutil
