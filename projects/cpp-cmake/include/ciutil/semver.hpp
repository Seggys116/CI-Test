#ifndef CIUTIL_SEMVER_HPP
#define CIUTIL_SEMVER_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ciutil/result.hpp"

namespace ciutil {

struct SemVer {
  std::uint64_t major = 0;
  std::uint64_t minor = 0;
  std::uint64_t patch = 0;
  std::vector<std::string> prerelease;
  std::vector<std::string> build;

  bool operator==(const SemVer&) const = default;
};

[[nodiscard]] Result<SemVer> parse_semver(std::string_view text);

// semver precedence: -1, 0 or 1; build metadata is ignored
[[nodiscard]] int compare(const SemVer& a, const SemVer& b) noexcept;

[[nodiscard]] std::string to_string(const SemVer& version);

}  // namespace ciutil

#endif
