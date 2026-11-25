#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ciutil/crc32.hpp"
#include "harness.hpp"
#include "test_util.hpp"

namespace {

struct Vector {
  std::string_view text;
  std::uint32_t expected;
};

constexpr std::array<Vector, 5> kVectors{{
    {"", 0x00000000u},
    {"a", 0xe8b7be43u},
    {"abc", 0x352441c2u},
    {"123456789", 0xcbf43926u},
    {"The quick brown fox jumps over the lazy dog", 0x414fa339u},
}};

// bitwise reference, independent of the table in the library
std::uint32_t reference_crc32(const ciutil::Bytes& data, std::size_t length) {
  std::uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < length; ++i) {
    crc ^= static_cast<std::uint32_t>(data[i]);
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 1u) != 0u ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

}  // namespace

TEST(crc32_known_vectors) {
  for (const Vector& vector : kVectors) {
    CHECK_EQ(ciutil::crc32(vector.text), vector.expected);
  }
}

TEST(crc32_thirty_two_zero_bytes) {
  const std::array<std::uint8_t, 32> zeros{};
  CHECK_EQ(ciutil::crc32(zeros), 0x190a55adu);
}

TEST(crc32_thirty_two_ff_bytes) {
  std::array<std::uint8_t, 32> ones{};
  ones.fill(0xFF);
  CHECK_EQ(ciutil::crc32(ones), 0xff6cab0bu);
}

TEST(crc32_empty_view) {
  CHECK_EQ(ciutil::crc32(ciutil::ByteView()), 0u);
}

TEST(crc32_chunked_equals_one_shot_at_every_split) {
  const ciutil::Bytes data = testutil::lcg_bytes(1000);
  const std::uint32_t whole = ciutil::crc32(data);
  for (std::size_t split = 0; split <= data.size(); ++split) {
    ciutil::Crc32 crc;
    crc.update(ciutil::ByteView(data.data(), split));
    crc.update(ciutil::ByteView(data.data() + split, data.size() - split));
    CHECK_EQ(crc.value(), whole);
  }
}

TEST(crc32_byte_at_a_time_equals_one_shot) {
  const ciutil::Bytes data = testutil::lcg_bytes(1000);
  ciutil::Crc32 crc;
  for (std::size_t i = 0; i < data.size(); ++i) {
    crc.update(ciutil::ByteView(data.data() + i, 1));
  }
  CHECK_EQ(crc.value(), ciutil::crc32(data));
}

TEST(crc32_value_does_not_consume_state_and_reset_works) {
  const ciutil::Bytes data = testutil::lcg_bytes(64);
  ciutil::Crc32 crc;
  CHECK_EQ(crc.value(), 0u);
  crc.update(data);
  const std::uint32_t first = crc.value();
  CHECK_EQ(crc.value(), first);
  crc.reset();
  CHECK_EQ(crc.value(), 0u);
  crc.update(data);
  CHECK_EQ(crc.value(), first);
}

TEST(crc32_matches_bitwise_reference_for_every_prefix) {
  const ciutil::Bytes data = testutil::lcg_bytes(1000);
  for (std::size_t length = 0; length <= data.size(); ++length) {
    CHECK_EQ(ciutil::crc32(ciutil::ByteView(data.data(), length)), reference_crc32(data, length));
  }
}

TEST(crc32_matches_bitwise_reference_for_every_single_byte) {
  for (unsigned int value = 0; value < 256; ++value) {
    const ciutil::Bytes data{static_cast<std::uint8_t>(value)};
    CHECK_EQ(ciutil::crc32(data), reference_crc32(data, 1));
  }
}

int main() {
  return harness::run_all();
}
