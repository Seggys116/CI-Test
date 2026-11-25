#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ciutil/crc32.hpp"
#include "test_util.hpp"

namespace {

using ciutil::Bytes;
using ciutil::crc32;
using ciutil::to_bytes;

struct Vector {
  std::string_view text;
  std::uint32_t expected;
};

class Crc32VectorTest : public ::testing::TestWithParam<Vector> {};

TEST_P(Crc32VectorTest, MatchesKnownValue) {
  const Vector& vector = GetParam();
  EXPECT_EQ(crc32(to_bytes(vector.text)), vector.expected);
}

INSTANTIATE_TEST_SUITE_P(Vectors, Crc32VectorTest,
                         ::testing::Values(Vector{"", 0x00000000u}, Vector{"a", 0xe8b7be43u},
                                           Vector{"abc", 0x352441c2u},
                                           Vector{"123456789", 0xcbf43926u},
                                           Vector{"The quick brown fox jumps over the lazy dog",
                                                  0x414fa339u}));

// bitwise reference, independent of the table in the library
std::uint32_t reference_crc32(const Bytes& data, std::size_t length) {
  std::uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < length; ++i) {
    crc ^= static_cast<std::uint32_t>(data[i]);
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 1u) != 0u ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

TEST(Crc32, ThirtyTwoZeroBytes) {
  const std::array<std::uint8_t, 32> zeros{};
  EXPECT_EQ(crc32(zeros), 0x190a55adu);
}

TEST(Crc32, ThirtyTwoOnesBytes) {
  std::array<std::uint8_t, 32> ones{};
  ones.fill(0xFF);
  EXPECT_EQ(crc32(ones), 0xff6cab0bu);
}

TEST(Crc32, EmptySpan) {
  EXPECT_EQ(crc32(ciutil::ByteSpan{}), 0u);
}

TEST(Crc32, MatchesBitwiseReferenceForEveryPrefix) {
  const Bytes data = ciutil::testing_util::lcg_bytes(1000);
  for (std::size_t length = 0; length <= data.size(); ++length) {
    EXPECT_EQ(crc32(ciutil::ByteSpan(data.data(), length)), reference_crc32(data, length))
        << "length " << length;
  }
}

TEST(Crc32, MatchesBitwiseReferenceForEverySingleByte) {
  for (unsigned int value = 0; value < 256; ++value) {
    const Bytes data{static_cast<std::uint8_t>(value)};
    EXPECT_EQ(crc32(data), reference_crc32(data, 1)) << "byte " << value;
  }
}

}  // namespace
