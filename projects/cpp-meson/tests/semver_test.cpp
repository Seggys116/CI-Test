#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ciutil/semver.hpp"
#include "harness.hpp"

namespace {

using ciutil::compare;
using ciutil::Error;
using ciutil::parse_semver;
using ciutil::SemVer;
using ciutil::to_string;

using Identifiers = std::vector<std::string>;

SemVer parse_ok(std::string_view text) {
  auto parsed = parse_semver(text);
  CHECK(parsed.ok());
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

constexpr std::uint64_t kMax = 18446744073709551615ull;

const std::vector<Accepted>& accepted() {
  static const std::vector<Accepted> cases{
      {"0.0.0", 0, 0, 0, {}, {}},
      {"1.2.3", 1, 2, 3, {}, {}},
      {"10.20.30", 10, 20, 30, {}, {}},
      {"1.0.0-0", 1, 0, 0, {"0"}, {}},
      {"1.0.0-0a", 1, 0, 0, {"0a"}, {}},
      {"1.0.0-x-y-z.-", 1, 0, 0, {"x-y-z", "-"}, {}},
      {"1.0.0+001", 1, 0, 0, {}, {"001"}},
      {"1.0.0-rc.1+build.5", 1, 0, 0, {"rc", "1"}, {"build", "5"}},
      {"1.0.0-alpha", 1, 0, 0, {"alpha"}, {}},
      {"1.0.0-0.3.7", 1, 0, 0, {"0", "3", "7"}, {}},
      {"1.0.0-x.7.z.92", 1, 0, 0, {"x", "7", "z", "92"}, {}},
      {"1.0.0-beta+exp.sha.5114f85", 1, 0, 0, {"beta"}, {"exp", "sha", "5114f85"}},
      {"1.0.0+0.0", 1, 0, 0, {}, {"0", "0"}},
      {"18446744073709551615.18446744073709551615.18446744073709551615", kMax, kMax, kMax, {}, {}},
  };
  return cases;
}

constexpr std::array<std::string_view, 36> kInvalid{
    "",           "1",          "1.0",        "1.0.0.0",      "v1.0.0",      "01.0.0",
    "1.00.0",     "1.0.00",     "1.0.0-",     "1.0.0+",       "1.0.0-01",    "1.0.0-a..b",
    "1.0.0-a_b",  "-1.0.0",     " 1.0.0",     "1.0.0 ",       "1.0.0-alpha+", "1.0.0-.a",
    "1.0.0-a.",   "1.0.0-a.01", "1.0.0-01.a", "1.0.0+a..b",   "1.0.0+.a",    "1.0.0+a.",
    "1.0.0+a+b",  "1.0.0-a+b+c", "1.0.0-+a",  "1.0.0++a",     "1..0",        "1.0.",
    ".1.0.0",     "a.b.c",      "1.x.0",      "1.0.0\n",      "+1.0.0",      "1.0.0-\xC3\xA9"};

constexpr std::array<std::string_view, 5> kOverflow{
    "18446744073709551616.0.0", "0.18446744073709551616.0", "0.0.18446744073709551616",
    "99999999999999999999.0.0", "0.0.184467440737095516150"};

constexpr std::array<std::string_view, 8> kPrereleaseChain{
    "1.0.0-alpha",  "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta",
    "1.0.0-beta.2", "1.0.0-beta.11", "1.0.0-rc.1",       "1.0.0"};

constexpr std::array<std::string_view, 6> kCoreChain{"0.9.9", "1.0.0", "2.0.0",
                                                     "2.1.0", "2.1.1", "10.0.0"};

template <std::size_t N>
void check_chain_pairwise(const std::array<std::string_view, N>& chain) {
  for (std::size_t i = 0; i < N; ++i) {
    for (std::size_t j = 0; j < N; ++j) {
      const int expected = i < j ? -1 : (i > j ? 1 : 0);
      CHECK_EQ(compare(parse_ok(chain[i]), parse_ok(chain[j])), expected);
    }
  }
}

}  // namespace

TEST(semver_parses_fields) {
  for (const Accepted& expected : accepted()) {
    const auto parsed = parse_semver(expected.text);
    CHECK(parsed.ok());
    if (!parsed.ok()) {
      continue;
    }
    const SemVer& version = parsed.value();
    CHECK_EQ(version.major, expected.major);
    CHECK_EQ(version.minor, expected.minor);
    CHECK_EQ(version.patch, expected.patch);
    CHECK(version.prerelease == expected.prerelease);
    CHECK(version.build == expected.build);
  }
}

TEST(semver_to_string_round_trips) {
  for (const Accepted& expected : accepted()) {
    const SemVer version = parse_ok(expected.text);
    CHECK_EQ(to_string(version), std::string(expected.text));
    CHECK(parse_ok(to_string(version)) == version);
  }
}

TEST(semver_rejects_invalid_as_syntax_error) {
  for (const std::string_view text : kInvalid) {
    const auto parsed = parse_semver(text);
    CHECK(!parsed.ok());
    if (!parsed.ok()) {
      CHECK_EQ(parsed.error(), Error::InvalidSyntax);
    }
  }
}

TEST(semver_rejects_numeric_overflow) {
  for (const std::string_view text : kOverflow) {
    const auto parsed = parse_semver(text);
    CHECK(!parsed.ok());
    if (!parsed.ok()) {
      CHECK_EQ(parsed.error(), Error::Overflow);
    }
  }
}

TEST(semver_accepts_values_above_63_bits) {
  const SemVer version = parse_ok("9223372036854775808.0.0");
  CHECK(version.major == std::uint64_t{9223372036854775808ull});
}

TEST(semver_prerelease_chain_is_strictly_ordered_for_all_pairs) {
  check_chain_pairwise(kPrereleaseChain);
}

TEST(semver_core_chain_is_strictly_ordered_for_all_pairs) {
  check_chain_pairwise(kCoreChain);
}

TEST(semver_identifier_rules) {
  const std::array<std::pair<std::string_view, std::string_view>, 13> orderings{{
      {"1.0.0-beta.2", "1.0.0-beta.11"},
      {"1.0.0-1", "1.0.0-a"},
      {"1.0.0-9", "1.0.0-10"},
      {"1.0.0-99999999999999999999", "1.0.0-100000000000000000000"},
      {"1.0.0-B", "1.0.0-a"},
      {"1.0.0-a", "1.0.0-a-"},
      {"1.0.0-1", "1.0.0-0a"},
      {"1.0.0-alpha", "1.0.0-alpha.1"},
      {"1.0.0-alpha.1", "1.0.0-alpha.beta"},
      {"1.0.0-rc.1", "1.0.0"},
      {"1.9.9", "2.0.0-alpha"},
      {"18446744073709551614.9.9", "18446744073709551615.0.0"},
      {"1.0.0-alpha+z", "1.0.0-beta+a"},
  }};
  for (const auto& ordering : orderings) {
    const SemVer lesser = parse_ok(ordering.first);
    const SemVer greater = parse_ok(ordering.second);
    CHECK_EQ(compare(lesser, greater), -1);
    CHECK_EQ(compare(greater, lesser), 1);
    CHECK_EQ(compare(lesser, lesser), 0);
  }
}

TEST(semver_build_metadata_is_ignored_by_compare) {
  const std::array<std::pair<std::string_view, std::string_view>, 4> equal{{
      {"1.0.0+a", "1.0.0+b"},
      {"1.0.0", "1.0.0+b"},
      {"1.0.0-rc.1+x", "1.0.0-rc.1+y.z"},
      {"0.0.0+1", "0.0.0+2"},
  }};
  for (const auto& pair : equal) {
    CHECK_EQ(compare(parse_ok(pair.first), parse_ok(pair.second)), 0);
    CHECK_EQ(compare(parse_ok(pair.second), parse_ok(pair.first)), 0);
  }
}

TEST(semver_structural_equality_still_sees_build_metadata) {
  CHECK(parse_ok("1.0.0+a") != parse_ok("1.0.0+b"));
  CHECK(parse_ok("1.0.0+a") == parse_ok("1.0.0+a"));
}

int main() {
  return harness::run_all();
}
