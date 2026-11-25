#include "ciutil/roman.hpp"

#include <array>
#include <cstddef>

namespace ciutil {

namespace {

struct Entry {
  int value;
  std::string_view symbol;
};

constexpr std::array<Entry, 13> kEntries{{
    {1000, "M"},
    {900, "CM"},
    {500, "D"},
    {400, "CD"},
    {100, "C"},
    {90, "XC"},
    {50, "L"},
    {40, "XL"},
    {10, "X"},
    {9, "IX"},
    {5, "V"},
    {4, "IV"},
    {1, "I"},
}};

constexpr int kMax = 3999;

}  // namespace

Result<std::string> to_roman(int value) {
  if (value < 1 || value > kMax) {
    return Error::OutOfRange;
  }
  std::string out;
  for (const Entry& entry : kEntries) {
    while (value >= entry.value) {
      out.append(entry.symbol);
      value -= entry.value;
    }
  }
  return out;
}

// parses greedily without regard to canonical form, then insists that the
// canonical spelling of the parsed value is exactly the input
Result<int> from_roman(std::string_view text) {
  if (text.empty()) {
    return Error::InvalidSyntax;
  }

  std::size_t pos = 0;
  int total = 0;
  for (const Entry& entry : kEntries) {
    while (text.substr(pos, entry.symbol.size()) == entry.symbol) {
      total += entry.value;
      if (total > kMax) {
        return Error::OutOfRange;
      }
      pos += entry.symbol.size();
    }
  }
  if (pos != text.size()) {
    return Error::InvalidSyntax;
  }

  const Result<std::string> canonical = to_roman(total);
  if (!canonical.ok() || canonical.value() != text) {
    return Error::InvalidSyntax;
  }
  return total;
}

}  // namespace ciutil
