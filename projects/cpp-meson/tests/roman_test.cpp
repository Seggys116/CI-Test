#include <array>
#include <limits>
#include <string>
#include <string_view>

#include "ciutil/roman.hpp"
#include "harness.hpp"

namespace {

using ciutil::Error;
using ciutil::from_roman;
using ciutil::to_roman;

struct Vector {
  int value;
  std::string_view roman;
};

constexpr std::array<Vector, 20> kVectors{{
    {1, "I"},
    {2, "II"},
    {3, "III"},
    {4, "IV"},
    {5, "V"},
    {9, "IX"},
    {10, "X"},
    {14, "XIV"},
    {40, "XL"},
    {50, "L"},
    {90, "XC"},
    {100, "C"},
    {400, "CD"},
    {500, "D"},
    {1000, "M"},
    {1994, "MCMXCIV"},
    {1999, "MCMXCIX"},
    {2024, "MMXXIV"},
    {3888, "MMMDCCCLXXXVIII"},
    {3999, "MMMCMXCIX"},
}};

constexpr std::array<std::string_view, 36> kInvalid{
    "",     "IIII", "VX",    "IC",   "XM",    "VV",   "IIX",  "xiv",   "Xiv",
    "MMMM", "IL",   "XD",    "VIV",  "CDC",   "IXI",  "MCMC", "XXXX",  "LL",
    "DD",   "IIIII", " X",   "X ",   "A",     "IVI",  "VL",   "LC",    "DM",
    "CMC",  "MMMMCMXCIX", "IIV", "XXXXX", "CCCC", "MDM", "XIIII", "IXX", "MMMDCCCLXXXVIIII"};

}  // namespace

TEST(roman_to_roman_vectors) {
  for (const Vector& vector : kVectors) {
    const auto roman = to_roman(vector.value);
    CHECK(roman.ok());
    if (roman.ok()) {
      CHECK_EQ(roman.value(), std::string(vector.roman));
    }
  }
}

TEST(roman_from_roman_vectors) {
  for (const Vector& vector : kVectors) {
    const auto value = from_roman(vector.roman);
    CHECK(value.ok());
    if (value.ok()) {
      CHECK_EQ(value.value(), vector.value);
    }
  }
}

TEST(roman_to_roman_rejects_out_of_range) {
  const std::array<int, 8> inputs{0,      4000,
                                  -1,     -3999,
                                  4001,   100000,
                                  std::numeric_limits<int>::max(),
                                  std::numeric_limits<int>::min()};
  for (const int input : inputs) {
    const auto roman = to_roman(input);
    CHECK(!roman.ok());
    if (!roman.ok()) {
      CHECK_EQ(roman.error(), Error::OutOfRange);
    }
  }
}

TEST(roman_from_roman_rejects_non_canonical_forms) {
  for (const std::string_view text : kInvalid) {
    CHECK(!from_roman(text).ok());
  }
}

TEST(roman_from_roman_rejects_trailing_newline) {
  CHECK(!from_roman("XIV\n").ok());
}

TEST(roman_exhaustive_round_trip) {
  for (int n = 1; n <= 3999; ++n) {
    const auto roman = to_roman(n);
    CHECK(roman.ok());
    if (!roman.ok()) {
      continue;
    }
    const auto back = from_roman(roman.value());
    CHECK(back.ok());
    if (back.ok()) {
      CHECK_EQ(back.value(), n);
    }
  }
}

TEST(roman_encodings_are_distinct_and_only_use_roman_letters) {
  std::string previous;
  for (int n = 1; n <= 3999; ++n) {
    const auto roman = to_roman(n);
    CHECK(roman.ok());
    if (!roman.ok()) {
      continue;
    }
    CHECK_NE(roman.value(), previous);
    CHECK(roman.value().size() <= 15);
    CHECK(roman.value().find_first_not_of("MDCLXVI") == std::string::npos);
    previous = roman.value();
  }
}

int main() {
  return harness::run_all();
}
