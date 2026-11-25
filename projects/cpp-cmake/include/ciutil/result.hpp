#ifndef CIUTIL_RESULT_HPP
#define CIUTIL_RESULT_HPP

#include <utility>
#include <variant>

namespace ciutil {

enum class Error {
  InvalidLength,
  InvalidCharacter,
  InvalidPadding,
  NonCanonicalBits,
  OutOfRange,
  InvalidSyntax,
  Overflow,
};

[[nodiscard]] constexpr const char* error_message(Error error) noexcept {
  switch (error) {
    case Error::InvalidLength:
      return "invalid length";
    case Error::InvalidCharacter:
      return "invalid character";
    case Error::InvalidPadding:
      return "invalid padding";
    case Error::NonCanonicalBits:
      return "non-canonical trailing bits";
    case Error::OutOfRange:
      return "value out of range";
    case Error::InvalidSyntax:
      return "invalid syntax";
    case Error::Overflow:
      return "number too large";
  }
  return "unknown error";
}

// holds either a value or an error code; value() requires ok(), error() requires !ok()
template <typename T>
class Result {
 public:
  Result(T value) : data_(std::in_place_index<0>, std::move(value)) {}
  Result(Error error) noexcept : data_(std::in_place_index<1>, error) {}

  [[nodiscard]] bool ok() const noexcept { return data_.index() == 0; }
  explicit operator bool() const noexcept { return ok(); }

  [[nodiscard]] const T& value() const& { return std::get<0>(data_); }
  [[nodiscard]] T& value() & { return std::get<0>(data_); }
  [[nodiscard]] T&& value() && { return std::get<0>(std::move(data_)); }

  [[nodiscard]] Error error() const { return std::get<1>(data_); }

 private:
  std::variant<T, Error> data_;
};

}  // namespace ciutil

#endif
