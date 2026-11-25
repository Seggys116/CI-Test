#include "ciutil/semver.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

namespace ciutil {

namespace {

constexpr bool is_digit(char c) noexcept {
  return c >= '0' && c <= '9';
}

constexpr bool is_identifier_char(char c) noexcept {
  return is_digit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
}

// consumes a leading non-negative integer without leading zeros
Result<std::uint64_t> parse_number(std::string_view& rest) {
  std::size_t digits = 0;
  while (digits < rest.size() && is_digit(rest[digits])) {
    ++digits;
  }
  if (digits == 0 || (digits > 1 && rest[0] == '0')) {
    return Error::InvalidSyntax;
  }

  std::uint64_t value = 0;
  for (std::size_t i = 0; i < digits; ++i) {
    const auto digit = static_cast<std::uint64_t>(rest[i] - '0');
    if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10u) {
      return Error::Overflow;
    }
    value = value * 10u + digit;
  }
  rest.remove_prefix(digits);
  return value;
}

// splits a dot separated identifier list; prerelease numeric identifiers may not have leading zeros
Result<std::vector<std::string>> parse_identifiers(std::string_view list, bool prerelease) {
  std::vector<std::string> identifiers;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= list.size(); ++i) {
    if (i != list.size() && list[i] != '.') {
      continue;
    }
    const std::string_view identifier = list.substr(start, i - start);
    if (identifier.empty()) {
      return Error::InvalidSyntax;
    }
    bool all_digits = true;
    for (const char c : identifier) {
      if (!is_identifier_char(c)) {
        return Error::InvalidSyntax;
      }
      if (!is_digit(c)) {
        all_digits = false;
      }
    }
    if (prerelease && all_digits && identifier.size() > 1 && identifier[0] == '0') {
      return Error::InvalidSyntax;
    }
    identifiers.emplace_back(identifier);
    start = i + 1;
  }
  return identifiers;
}

bool expect_dot(std::string_view& rest) noexcept {
  if (rest.empty() || rest.front() != '.') {
    return false;
  }
  rest.remove_prefix(1);
  return true;
}

template <typename T>
int three_way(const T& a, const T& b) noexcept {
  return a < b ? -1 : (b < a ? 1 : 0);
}

bool is_numeric(const std::string& identifier) noexcept {
  return std::all_of(identifier.begin(), identifier.end(), [](char c) { return is_digit(c); });
}

int compare_identifier(const std::string& a, const std::string& b) noexcept {
  const bool a_numeric = is_numeric(a);
  const bool b_numeric = is_numeric(b);
  if (a_numeric && b_numeric) {
    if (a.size() != b.size()) {
      return a.size() < b.size() ? -1 : 1;
    }
  } else if (a_numeric) {
    return -1;
  } else if (b_numeric) {
    return 1;
  }
  const int c = a.compare(b);
  return c < 0 ? -1 : (c > 0 ? 1 : 0);
}

void append_identifiers(std::string& out, const std::vector<std::string>& identifiers) {
  for (std::size_t i = 0; i < identifiers.size(); ++i) {
    if (i != 0) {
      out.push_back('.');
    }
    out.append(identifiers[i]);
  }
}

}  // namespace

Result<SemVer> parse_semver(std::string_view text) {
  SemVer version;

  auto major = parse_number(text);
  if (!major.ok()) {
    return major.error();
  }
  version.major = major.value();
  if (!expect_dot(text)) {
    return Error::InvalidSyntax;
  }

  auto minor = parse_number(text);
  if (!minor.ok()) {
    return minor.error();
  }
  version.minor = minor.value();
  if (!expect_dot(text)) {
    return Error::InvalidSyntax;
  }

  auto patch = parse_number(text);
  if (!patch.ok()) {
    return patch.error();
  }
  version.patch = patch.value();

  if (!text.empty() && text.front() == '-') {
    text.remove_prefix(1);
    const std::size_t end = std::min(text.find('+'), text.size());
    auto identifiers = parse_identifiers(text.substr(0, end), true);
    if (!identifiers.ok()) {
      return identifiers.error();
    }
    version.prerelease = std::move(identifiers).value();
    text.remove_prefix(end);
  }

  if (!text.empty() && text.front() == '+') {
    text.remove_prefix(1);
    auto identifiers = parse_identifiers(text, false);
    if (!identifiers.ok()) {
      return identifiers.error();
    }
    version.build = std::move(identifiers).value();
    text.remove_prefix(text.size());
  }

  if (!text.empty()) {
    return Error::InvalidSyntax;
  }
  return version;
}

int compare(const SemVer& a, const SemVer& b) noexcept {
  if (const int c = three_way(a.major, b.major); c != 0) {
    return c;
  }
  if (const int c = three_way(a.minor, b.minor); c != 0) {
    return c;
  }
  if (const int c = three_way(a.patch, b.patch); c != 0) {
    return c;
  }

  if (a.prerelease.empty() || b.prerelease.empty()) {
    if (a.prerelease.empty() && b.prerelease.empty()) {
      return 0;
    }
    return a.prerelease.empty() ? 1 : -1;
  }

  const std::size_t shared = std::min(a.prerelease.size(), b.prerelease.size());
  for (std::size_t i = 0; i < shared; ++i) {
    if (const int c = compare_identifier(a.prerelease[i], b.prerelease[i]); c != 0) {
      return c;
    }
  }
  return three_way(a.prerelease.size(), b.prerelease.size());
}

std::string to_string(const SemVer& version) {
  std::string out = std::to_string(version.major);
  out.push_back('.');
  out.append(std::to_string(version.minor));
  out.push_back('.');
  out.append(std::to_string(version.patch));
  if (!version.prerelease.empty()) {
    out.push_back('-');
    append_identifiers(out, version.prerelease);
  }
  if (!version.build.empty()) {
    out.push_back('+');
    append_identifiers(out, version.build);
  }
  return out;
}

}  // namespace ciutil
