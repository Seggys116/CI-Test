#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "ciutil/semver.hpp"

namespace {

using ciutil::compare;
using ciutil::Error;
using ciutil::parse_semver;
using ciutil::SemVer;
using ciutil::to_string;

using Identifiers = std::vector<std::string>;

SemVer parse_ok(std::string_view text) {
  auto parsed = parse_semver(text);
  EXPECT_TRUE(parsed.ok()) << text;
  return parsed.ok() ? std::move(parsed).value() : SemVer{};
}

struct Accepted {
  std::string_view text;
  std::uint64_t major;
  std::uint64_t minor;
  std::uint64_t patch;
  Identifiers prerelease;
  Identifiers build;
};

class SemVerAcceptTest : public ::testing::TestWithParam<Accepted> {};

TEST_P(SemVerAcceptTest, ParsesFields) {
  const Accepted& expected = GetParam();
  const auto parsed = parse_semver(expected.text);
  ASSERT_TRUE(parsed.ok());
  const SemVer& version = parsed.value();
  EXPECT_EQ(version.major, expected.major);
  EXPECT_EQ(version.minor, expected.minor);
  EXPECT_EQ(version.patch, expected.patch);
  EXPECT_EQ(version.prerelease, expected.prerelease);
  EXPECT_EQ(version.build, expected.build);
}

TEST_P(SemVerAcceptTest, ToStringRoundTrips) {
  const Accepted& expected = GetParam();
  const SemVer version = parse_ok(expected.text);
  EXPECT_EQ(to_string(version), expected.text);
  EXPECT_EQ(parse_ok(to_string(version)), version);
}

constexpr std::uint64_t kMax = 18446744073709551615ull;

INSTANTIATE_TEST_SUITE_P(
    Valid, SemVerAcceptTest,
    ::testing::Values(
        Accepted{"0.0.0", 0, 0, 0, {}, {}}, Accepted{"1.2.3", 1, 2, 3, {}, {}},
        Accepted{"10.20.30", 10, 20, 30, {}, {}},
        Accepted{"1.0.0-0", 1, 0, 0, {"0"}, {}}, Accepted{"1.0.0-0a", 1, 0, 0, {"0a"}, {}},
        Accepted{"1.0.0-x-y-z.-", 1, 0, 0, {"x-y-z", "-"}, {}},
        Accepted{"1.0.0+001", 1, 0, 0, {}, {"001"}},
        Accepted{"1.0.0-rc.1+build.5", 1, 0, 0, {"rc", "1"}, {"build", "5"}},
        Accepted{"1.0.0-alpha", 1, 0, 0, {"alpha"}, {}},
        Accepted{"1.0.0-0.3.7", 1, 0, 0, {"0", "3", "7"}, {}},
        Accepted{"1.0.0-x.7.z.92", 1, 0, 0, {"x", "7", "z", "92"}, {}},
        Accepted{"1.0.0-beta+exp.sha.5114f85", 1, 0, 0, {"beta"}, {"exp", "sha", "5114f85"}},
        Accepted{"1.0.0+0.0", 1, 0, 0, {}, {"0", "0"}},
        Accepted{"18446744073709551615.18446744073709551615.18446744073709551615", kMax, kMax,
                 kMax, {}, {}}));

class SemVerRejectTest : public ::testing::TestWithParam<std::string_view> {};

TEST_P(SemVerRejectTest, IsRejectedAsSyntaxError) {
  const auto parsed = parse_semver(GetParam());
  ASSERT_FALSE(parsed.ok());
  EXPECT_EQ(parsed.error(), Error::InvalidSyntax);
}

INSTANTIATE_TEST_SUITE_P(
    Invalid, SemVerRejectTest,
    ::testing::Values(std::string_view(""), "1", "1.0", "1.0.0.0", "v1.0.0", "01.0.0", "1.00.0",
                      "1.0.00", "1.0.0-", "1.0.0+", "1.0.0-01", "1.0.0-a..b", "1.0.0-a_b",
                      "-1.0.0", " 1.0.0", "1.0.0 ", "1.0.0-alpha+", "1.0.0-.a", "1.0.0-a.",
                      "1.0.0-a.01", "1.0.0-01.a", "1.0.0+a..b", "1.0.0+.a", "1.0.0+a.",
                      "1.0.0+a+b", "1.0.0-a+b+c", "1.0.0-+a", "1.0.0++a", "1..0", "1.0.",
                      ".1.0.0", "a.b.c", "1.0.0-\xC3\xA9", "1.x.0", "1.0.0\n", "+1.0.0"));

class SemVerOverflowTest : public ::testing::TestWithParam<std::string_view> {};

TEST_P(SemVerOverflowTest, IsRejectedAsOverflow) {
  const auto parsed = parse_semver(GetParam());
  ASSERT_FALSE(parsed.ok());
  EXPECT_EQ(parsed.error(), Error::Overflow);
}

INSTANTIATE_TEST_SUITE_P(TooLarge, SemVerOverflowTest,
                         ::testing::Values(std::string_view("18446744073709551616.0.0"),
                                           "0.18446744073709551616.0", "0.0.18446744073709551616",
                                           "99999999999999999999.0.0",
                                           "0.0.184467440737095516150"));

TEST(SemVer, AcceptsValuesAbove63Bits) {
  const SemVer version = parse_ok("9223372036854775808.0.0");
  EXPECT_EQ(version.major, std::uint64_t{9223372036854775808ull});
}

constexpr std::array<std::string_view, 8> kPrereleaseChain{
    "1.0.0-alpha",  "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta",
    "1.0.0-beta.2", "1.0.0-beta.11", "1.0.0-rc.1",       "1.0.0"};

constexpr std::array<std::string_view, 6> kCoreChain{"0.9.9", "1.0.0", "2.0.0",
                                                     "2.1.0", "2.1.1", "10.0.0"};

class PrereleaseChainTest
    : public ::testing::TestWithParam<std::tuple<std::size_t, std::size_t>> {};

TEST_P(PrereleaseChainTest, OrderingMatchesChainPosition) {
  const auto [i, j] = GetParam();
  const int expected = i < j ? -1 : (i > j ? 1 : 0);
  EXPECT_EQ(compare(parse_ok(kPrereleaseChain[i]), parse_ok(kPrereleaseChain[j])), expected);
}

INSTANTIATE_TEST_SUITE_P(AllPairs, PrereleaseChainTest,
                         ::testing::Combine(::testing::Range<std::size_t>(0, 8),
                                            ::testing::Range<std::size_t>(0, 8)));

class CoreChainTest : public ::testing::TestWithParam<std::tuple<std::size_t, std::size_t>> {};

TEST_P(CoreChainTest, OrderingMatchesChainPosition) {
  const auto [i, j] = GetParam();
  const int expected = i < j ? -1 : (i > j ? 1 : 0);
  EXPECT_EQ(compare(parse_ok(kCoreChain[i]), parse_ok(kCoreChain[j])), expected);
}

INSTANTIATE_TEST_SUITE_P(AllPairs, CoreChainTest,
                         ::testing::Combine(::testing::Range<std::size_t>(0, 6),
                                            ::testing::Range<std::size_t>(0, 6)));

struct Ordering {
  std::string_view lesser;
  std::string_view greater;
};

class OrderingTest : public ::testing::TestWithParam<Ordering> {};

TEST_P(OrderingTest, LesserIsStrictlyLessAndAntisymmetric) {
  const Ordering& ordering = GetParam();
  const SemVer lesser = parse_ok(ordering.lesser);
  const SemVer greater = parse_ok(ordering.greater);
  EXPECT_EQ(compare(lesser, greater), -1);
  EXPECT_EQ(compare(greater, lesser), 1);
  EXPECT_EQ(compare(lesser, lesser), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Rules, OrderingTest,
    ::testing::Values(Ordering{"1.0.0-beta.2", "1.0.0-beta.11"}, Ordering{"1.0.0-1", "1.0.0-a"},
                      Ordering{"1.0.0-9", "1.0.0-10"},
                      Ordering{"1.0.0-99999999999999999999", "1.0.0-100000000000000000000"},
                      Ordering{"1.0.0-B", "1.0.0-a"}, Ordering{"1.0.0-a", "1.0.0-a-"},
                      Ordering{"1.0.0-1", "1.0.0-0a"}, Ordering{"1.0.0-alpha", "1.0.0-alpha.1"},
                      Ordering{"1.0.0-alpha.1", "1.0.0-alpha.beta"},
                      Ordering{"1.0.0-rc.1", "1.0.0"}, Ordering{"1.9.9", "2.0.0-alpha"},
                      Ordering{"18446744073709551614.9.9", "18446744073709551615.0.0"},
                      Ordering{"1.0.0-alpha+z", "1.0.0-beta+a"}));

struct Equality {
  std::string_view a;
  std::string_view b;
};

class BuildMetadataTest : public ::testing::TestWithParam<Equality> {};

TEST_P(BuildMetadataTest, IsIgnoredByCompare) {
  const Equality& pair = GetParam();
  EXPECT_EQ(compare(parse_ok(pair.a), parse_ok(pair.b)), 0);
  EXPECT_EQ(compare(parse_ok(pair.b), parse_ok(pair.a)), 0);
}

INSTANTIATE_TEST_SUITE_P(Equal, BuildMetadataTest,
                         ::testing::Values(Equality{"1.0.0+a", "1.0.0+b"},
                                           Equality{"1.0.0", "1.0.0+b"},
                                           Equality{"1.0.0-rc.1+x", "1.0.0-rc.1+y.z"},
                                           Equality{"0.0.0+1", "0.0.0+2"}));

TEST(SemVer, BuildMetadataStillDistinguishesStructuralEquality) {
  EXPECT_NE(parse_ok("1.0.0+a"), parse_ok("1.0.0+b"));
  EXPECT_EQ(parse_ok("1.0.0+a"), parse_ok("1.0.0+a"));
}

}  // namespace
