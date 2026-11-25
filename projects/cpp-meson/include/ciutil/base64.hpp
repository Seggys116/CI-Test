#ifndef CIUTIL_BASE64_HPP
#define CIUTIL_BASE64_HPP

#include <string>
#include <string_view>

#include "ciutil/bytes.hpp"
#include "ciutil/result.hpp"

namespace ciutil {

// rfc 4648 standard alphabet with padding
[[nodiscard]] std::string base64_encode(ByteView data);

// strict: rejects bad length, characters outside the alphabet, misplaced padding and non-canonical trailing bits
[[nodiscard]] Result<Bytes> base64_decode(std::string_view text);

}  // namespace ciutil

#endif
