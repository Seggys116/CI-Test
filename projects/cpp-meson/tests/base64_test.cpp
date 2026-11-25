#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "ciutil/base64.hpp"
#include "harness.hpp"
#include "test_util.hpp"

namespace {

using ciutil::base64_decode;
using ciutil::base64_encode;
using ciutil::Bytes;
using ciutil::Error;

struct Vector {
  std::string_view plain;
  std::string_view encoded;
};

constexpr std::array<Vector, 7> kVectors{{
    {"", ""},
    {"f", "Zg=="},
    {"fo", "Zm8="},
    {"foo", "Zm9v"},
    {"foob", "Zm9vYg=="},
    {"fooba", "Zm9vYmE="},
    {"foobar", "Zm9vYmFy"},
}};

struct Rejection {
  std::string_view text;
  Error error;
};

const std::array<Rejection, 24> kRejections{{
    {"Z", Error::InvalidLength},
    {"Zg", Error::InvalidLength},
    {"Zg=", Error::InvalidLength},
    {"Zm9vY", Error::InvalidLength},
    {"Zg===", Error::InvalidLength},
    {"Zm9v\n", Error::InvalidLength},
    {"Z===", Error::InvalidPadding},
    {"====", Error::InvalidPadding},
    {"=Zg=", Error::InvalidPadding},
    {"Zg==Zg==", Error::InvalidPadding},
    {"Zm=9", Error::InvalidPadding},
    {"Zg=a", Error::InvalidPadding},
    {"Zm9vYg=\n", Error::InvalidPadding},
    {"Zm-v", Error::InvalidCharacter},
    {"Zm_v", Error::InvalidCharacter},
    {"Zm 9", Error::InvalidCharacter},
    {"Zm9\n", Error::InvalidCharacter},
    {"Zm9\t", Error::InvalidCharacter},
    {"Zm9!", Error::InvalidCharacter},
    {"Zm\xC3\xA9", Error::InvalidCharacter},
    {"Zh==", Error::NonCanonicalBits},
    {"Zm9=", Error::NonCanonicalBits},
    {"Zm9vZh==", Error::NonCanonicalBits},
    {"Zm9vZm9=", Error::NonCanonicalBits},
}};

std::string to_text(const Bytes& bytes) {
  return std::string(bytes.begin(), bytes.end());
}

}  // namespace

TEST(base64_encodes_rfc4648_vectors) {
  for (const Vector& vector : kVectors) {
    CHECK_EQ(base64_encode(vector.plain), std::string(vector.encoded));
  }
}

TEST(base64_decodes_rfc4648_vectors) {
  for (const Vector& vector : kVectors) {
    const auto decoded = base64_decode(vector.encoded);
    CHECK(decoded.ok());
    if (decoded.ok()) {
      CHECK_EQ(to_text(decoded.value()), std::string(vector.plain));
    }
  }
}

TEST(base64_binary_vector) {
  const Bytes raw{0x00, 0xFF, 0xFE};
  CHECK_EQ(base64_encode(raw), std::string("AP/+"));
  const auto decoded = base64_decode("AP/+");
  CHECK(decoded.ok());
  if (decoded.ok()) {
    CHECK(decoded.value() == raw);
  }
}

TEST(base64_rejects_invalid_input_with_expected_error) {
  for (const Rejection& rejection : kRejections) {
    const auto decoded = base64_decode(rejection.text);
    CHECK(!decoded.ok());
    if (!decoded.ok()) {
      CHECK_EQ(decoded.error(), rejection.error);
    }
  }
}

TEST(base64_rejects_embedded_nul) {
  const auto decoded = base64_decode(std::string_view("Zg\0=", 4));
  CHECK(!decoded.ok());
  if (!decoded.ok()) {
    CHECK_EQ(decoded.error(), Error::InvalidCharacter);
  }
}

TEST(base64_round_trip_lengths_0_to_300) {
  for (std::size_t length = 0; length <= 300; ++length) {
    const Bytes data = testutil::lcg_bytes(length);
    const std::string encoded = base64_encode(data);
    CHECK_EQ(encoded.size(), (length + 2) / 3 * 4);
    const auto decoded = base64_decode(encoded);
    CHECK(decoded.ok());
    if (decoded.ok()) {
      CHECK(decoded.value() == data);
    }
  }
}

TEST(base64_encoded_output_only_uses_standard_alphabet) {
  const Bytes data = testutil::lcg_bytes(300);
  for (const char c : base64_encode(data)) {
    const bool allowed = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                         (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
    CHECK(allowed);
  }
}

int main() {
  return harness::run_all();
}
