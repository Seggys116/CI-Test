#ifndef CIUTIL_CRC32_HPP
#define CIUTIL_CRC32_HPP

#include <cstdint>

#include "ciutil/bytes.hpp"

namespace ciutil {

// crc-32/ieee, reflected, polynomial 0xedb88320
class Crc32 {
 public:
  void update(ByteSpan data) noexcept;
  [[nodiscard]] std::uint32_t value() const noexcept;
  void reset() noexcept;

 private:
  std::uint32_t state_ = 0xFFFFFFFFu;
};

[[nodiscard]] std::uint32_t crc32(ByteSpan data) noexcept;

}  // namespace ciutil

#endif
