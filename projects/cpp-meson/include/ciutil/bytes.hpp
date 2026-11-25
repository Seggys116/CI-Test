#ifndef CIUTIL_BYTES_HPP
#define CIUTIL_BYTES_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace ciutil {

using Bytes = std::vector<std::uint8_t>;

// non-owning view of a contiguous byte range
class ByteView {
 public:
  constexpr ByteView() noexcept = default;
  constexpr ByteView(const std::uint8_t* data, std::size_t size) noexcept
      : data_(data), size_(size) {}
  ByteView(const Bytes& bytes) noexcept : data_(bytes.data()), size_(bytes.size()) {}
  template <std::size_t N>
  constexpr ByteView(const std::array<std::uint8_t, N>& bytes) noexcept
      : data_(bytes.data()), size_(N) {}
  // views the characters of text as raw bytes, no encoding conversion
  ByteView(std::string_view text) noexcept
      : data_(reinterpret_cast<const std::uint8_t*>(text.data())), size_(text.size()) {}

  [[nodiscard]] constexpr const std::uint8_t* data() const noexcept { return data_; }
  [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] constexpr const std::uint8_t* begin() const noexcept { return data_; }
  [[nodiscard]] constexpr const std::uint8_t* end() const noexcept { return data_ + size_; }
  [[nodiscard]] constexpr std::uint8_t operator[](std::size_t index) const noexcept {
    return data_[index];
  }

 private:
  const std::uint8_t* data_ = nullptr;
  std::size_t size_ = 0;
};

}  // namespace ciutil

#endif
