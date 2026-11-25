#ifndef CIUTIL_ROMAN_HPP
#define CIUTIL_ROMAN_HPP

#include <string>
#include <string_view>

#include "ciutil/result.hpp"

namespace ciutil {

// 1..3999 only
[[nodiscard]] Result<std::string> to_roman(int value);

// strict: only the canonical spelling of a value is accepted
[[nodiscard]] Result<int> from_roman(std::string_view text);

}  // namespace ciutil

#endif
