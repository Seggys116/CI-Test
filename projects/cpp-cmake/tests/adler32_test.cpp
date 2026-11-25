#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ciutil/adler32.hpp"
#include "test_util.hpp"

namespace {

using ciutil::adler32;
using ciutil::Bytes;
using ciutil::to_bytes;

struct Vector {
  std::string_view text;
  std::uint32_t expected;
};

class Adler32VectorTest : public ::testing::TestWithParam<Vector> {};

TEST_P(Adler32VectorTest, MatchesKnownValue) {
  const Vector& vector = GetParam();
  EXPECT_EQ(adler32(to_bytes(vector.text)), vector.expected);
}

INSTANTIATE_TEST_SUITE_P(Vectors, Adler32VectorTest,
                         ::testing::Values(Vector{"", 0x00000001u}, Vector{"a", 0x00620062u},
                                           Vector{"abc", 0x024d0127u},
                                           Vector{"Wikipedia", 0x11e60398u},
                                           Vector{"123456789", 0x091e01deu}));

// naive reference: modulo after every byte
std::uint32_t reference_adler32(const Bytes& data, std::size_t length) {
  std::uint32_t a = 1;
  std::uint32_t b = 0;
  for (std::size_t i = 0; i < length; ++i) {
    a = (a + static_cast<std::uint32_t>(data[i])) % 65521u;
    b = (b + a) % 65521u;
  }
  return (b << 16) | a;
}

class Adler32LengthTest : public ::testing::TestWithParam<std::size_t> {};

TEST_P(Adler32LengthTest, FfBytesMatchReference) {
  const Bytes data(GetParam(), 0xFF);
  EXPECT_EQ(adler32(data), reference_adler32(data, data.size()));
}

TEST_P(Adler32LengthTest, PseudoRandomBytesMatchReference) {
  const Bytes data = ciutil::testing_util::lcg_bytes(GetParam());
  EXPECT_EQ(adler32(data), reference_adler32(data, data.size()));
}

INSTANTIATE_TEST_SUITE_P(AroundNmax, Adler32LengthTest,
                         ::testing::Values(std::size_t{0}, std::size_t{1}, std::size_t{5551},
                                           std::size_t{5552}, std::size_t{5553},
                                           std::size_t{11103}, std::size_t{11104},
                                           std::size_t{11105}, std::size_t{100000}));

TEST(Adler32, EmptySpan) {
  EXPECT_EQ(adler32(ciutil::ByteSpan{}), 1u);
}

TEST(Adler32, LongRunOfFfBytesExercisesModuloReduction) {
  const Bytes data(100000, 0xFF);
  EXPECT_EQ(adler32(data), reference_adler32(data, data.size()));
}

TEST(Adler32, IncrementalAcrossNmaxBoundary) {
  const Bytes data(100000, 0xFF);
  const std::uint32_t whole = adler32(data);
  for (const std::size_t split : {std::size_t{1}, std::size_t{5551}, std::size_t{5552},
                                  std::size_t{5553}, std::size_t{11104}, std::size_t{50000},
                                  std::size_t{99999}}) {
    ciutil::Adler32 sum;
    sum.update(ciutil::ByteSpan(data.data(), split));
    sum.update(ciutil::ByteSpan(data.data() + split, data.size() - split));
    EXPECT_EQ(sum.value(), whole) << "split " << split;
  }
}

}  // namespace
