#ifndef CIUTIL_ADLER32_HPP
#define CIUTIL_ADLER32_HPP

#include <cstdint>

#include "ciutil/bytes.hpp"

namespace ciutil {

// adler-32 as in rfc 1950
class Adler32 {
 public:
  void update(ByteSpan data) noexcept;
  [[nodiscard]] std::uint32_t value() const noexcept;
  void reset() noexcept;

 private:
  std::uint32_t a_ = 1;
  std::uint32_t b_ = 0;
};

[[nodiscard]] std::uint32_t adler32(ByteSpan data) noexcept;

}  // namespace ciutil

#endif
