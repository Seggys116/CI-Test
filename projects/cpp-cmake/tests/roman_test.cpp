#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <string_view>

#include "ciutil/roman.hpp"

namespace {

using ciutil::Error;
using ciutil::from_roman;
using ciutil::to_roman;

struct Vector {
  int value;
  std::string_view roman;
};

class RomanVectorTest : public ::testing::TestWithParam<Vector> {};

TEST_P(RomanVectorTest, ToRoman) {
  const Vector& vector = GetParam();
  const auto roman = to_roman(vector.value);
  ASSERT_TRUE(roman.ok());
  EXPECT_EQ(roman.value(), vector.roman);
}

TEST_P(RomanVectorTest, FromRoman) {
  const Vector& vector = GetParam();
  const auto value = from_roman(vector.roman);
  ASSERT_TRUE(value.ok());
  EXPECT_EQ(value.value(), vector.value);
}

INSTANTIATE_TEST_SUITE_P(
    Vectors, RomanVectorTest,
    ::testing::Values(Vector{1, "I"}, Vector{2, "II"}, Vector{3, "III"}, Vector{4, "IV"},
                      Vector{5, "V"}, Vector{9, "IX"}, Vector{10, "X"}, Vector{14, "XIV"},
                      Vector{40, "XL"}, Vector{50, "L"}, Vector{90, "XC"}, Vector{100, "C"},
                      Vector{400, "CD"}, Vector{500, "D"}, Vector{1000, "M"},
                      Vector{1994, "MCMXCIV"}, Vector{1999, "MCMXCIX"}, Vector{2024, "MMXXIV"},
                      Vector{3888, "MMMDCCCLXXXVIII"}, Vector{3999, "MMMCMXCIX"}));

class RomanRejectTest : public ::testing::TestWithParam<std::string_view> {};

TEST_P(RomanRejectTest, FromRomanFails) {
  EXPECT_FALSE(from_roman(GetParam()).ok());
}

INSTANTIATE_TEST_SUITE_P(
    NonCanonical, RomanRejectTest,
    ::testing::Values(std::string_view(""), "IIII", "VX", "IC", "XM", "VV", "IIX", "xiv", "Xiv",
                      "MMMM", "IL", "XD", "VIV", "CDC", "IXI", "MCMC", "XXXX", "LL", "DD", "IIIII",
                      " X", "X ", "A", "IVI", "VL", "LC", "DM", "CMC", "MMMMCMXCIX", "IIV",
                      "XXXXX", "CCCC", "MDM", "XIIII", "IXX", "MMMDCCCLXXXVIIII"));

TEST(Roman, RejectsTrailingNewline) {
  EXPECT_FALSE(from_roman("XIV\n").ok());
}

class RomanRangeTest : public ::testing::TestWithParam<int> {};

TEST_P(RomanRangeTest, ToRomanFailsOutOfRange) {
  const auto roman = to_roman(GetParam());
  ASSERT_FALSE(roman.ok());
  EXPECT_EQ(roman.error(), Error::OutOfRange);
}

INSTANTIATE_TEST_SUITE_P(Bounds, RomanRangeTest,
                         ::testing::Values(0, 4000, -1, -3999, 4001, 100000,
                                           std::numeric_limits<int>::max(),
                                           std::numeric_limits<int>::min()));

TEST(Roman, ExhaustiveRoundTrip) {
  for (int n = 1; n <= 3999; ++n) {
    const auto roman = to_roman(n);
    ASSERT_TRUE(roman.ok()) << n;
    const auto back = from_roman(roman.value());
    ASSERT_TRUE(back.ok()) << n << " " << roman.value();
    EXPECT_EQ(back.value(), n) << roman.value();
  }
}

TEST(Roman, EncodingsAreDistinctAndOnlyUseRomanLetters) {
  std::string previous;
  for (int n = 1; n <= 3999; ++n) {
    const auto roman = to_roman(n);
    ASSERT_TRUE(roman.ok());
    EXPECT_NE(roman.value(), previous);
    EXPECT_LE(roman.value().size(), 15u);
    EXPECT_EQ(roman.value().find_first_not_of("MDCLXVI"), std::string::npos);
    previous = roman.value();
  }
}

}  // namespace
