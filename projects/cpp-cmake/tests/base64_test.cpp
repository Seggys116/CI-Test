#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "ciutil/base64.hpp"
#include "test_util.hpp"

namespace {

using ciutil::base64_decode;
using ciutil::base64_encode;
using ciutil::Bytes;
using ciutil::Error;
using ciutil::to_bytes;

struct Vector {
  std::string_view plain;
  std::string_view encoded;
};

class Base64VectorTest : public ::testing::TestWithParam<Vector> {};

TEST_P(Base64VectorTest, Encodes) {
  const Vector& vector = GetParam();
  EXPECT_EQ(base64_encode(to_bytes(vector.plain)), vector.encoded);
}

TEST_P(Base64VectorTest, Decodes) {
  const Vector& vector = GetParam();
  const auto decoded = base64_decode(vector.encoded);
  ASSERT_TRUE(decoded.ok());
  const Bytes& bytes = decoded.value();
  EXPECT_EQ(std::string(bytes.begin(), bytes.end()), std::string(vector.plain));
}

INSTANTIATE_TEST_SUITE_P(Rfc4648, Base64VectorTest,
                         ::testing::Values(Vector{"", ""}, Vector{"f", "Zg=="},
                                           Vector{"fo", "Zm8="}, Vector{"foo", "Zm9v"},
                                           Vector{"foob", "Zm9vYg=="},
                                           Vector{"fooba", "Zm9vYmE="},
                                           Vector{"foobar", "Zm9vYmFy"}));

TEST(Base64, BinaryVector) {
  const Bytes raw{0x00, 0xFF, 0xFE};
  EXPECT_EQ(base64_encode(raw), "AP/+");
  const auto decoded = base64_decode("AP/+");
  ASSERT_TRUE(decoded.ok());
  EXPECT_EQ(decoded.value(), raw);
}

struct Rejection {
  std::string_view text;
  Error error;
};

class Base64RejectTest : public ::testing::TestWithParam<Rejection> {};

TEST_P(Base64RejectTest, IsRejectedWithExpectedError) {
  const Rejection& rejection = GetParam();
  const auto decoded = base64_decode(rejection.text);
  ASSERT_FALSE(decoded.ok());
  EXPECT_EQ(decoded.error(), rejection.error);
}

INSTANTIATE_TEST_SUITE_P(
    Invalid, Base64RejectTest,
    ::testing::Values(
        Rejection{"Z", Error::InvalidLength}, Rejection{"Zg", Error::InvalidLength},
        Rejection{"Zg=", Error::InvalidLength}, Rejection{"Zm9vY", Error::InvalidLength},
        Rejection{"Zg===", Error::InvalidLength}, Rejection{"Zm9v\n", Error::InvalidLength},
        Rejection{"Z===", Error::InvalidPadding}, Rejection{"====", Error::InvalidPadding},
        Rejection{"=Zg=", Error::InvalidPadding}, Rejection{"Zg==Zg==", Error::InvalidPadding},
        Rejection{"Zm=9", Error::InvalidPadding}, Rejection{"Zg=a", Error::InvalidPadding},
        Rejection{"Zm9vYg=\n", Error::InvalidPadding}, Rejection{"Zm-v", Error::InvalidCharacter},
        Rejection{"Zm_v", Error::InvalidCharacter}, Rejection{"Zm 9", Error::InvalidCharacter},
        Rejection{"Zm9\n", Error::InvalidCharacter}, Rejection{"Zm9\t", Error::InvalidCharacter},
        Rejection{"Zm9!", Error::InvalidCharacter}, Rejection{"Zm\xC3\xA9", Error::InvalidCharacter},
        Rejection{std::string_view("Zg\0=", 4), Error::InvalidCharacter},
        Rejection{"Zh==", Error::NonCanonicalBits}, Rejection{"Zm9=", Error::NonCanonicalBits},
        Rejection{"Zm9vZh==", Error::NonCanonicalBits},
        Rejection{"Zm9vZm9=", Error::NonCanonicalBits}));

class Base64RoundTripTest : public ::testing::TestWithParam<std::size_t> {};

TEST_P(Base64RoundTripTest, DecodeOfEncodeIsIdentity) {
  const Bytes data = ciutil::testing_util::lcg_bytes(GetParam());
  const std::string encoded = base64_encode(data);
  EXPECT_EQ(encoded.size(), (data.size() + 2) / 3 * 4);
  EXPECT_EQ(encoded.size() % 4, 0u);
  const auto decoded = base64_decode(encoded);
  ASSERT_TRUE(decoded.ok());
  EXPECT_EQ(decoded.value(), data);
}

INSTANTIATE_TEST_SUITE_P(Lengths0To300, Base64RoundTripTest,
                         ::testing::Range<std::size_t>(0, 301));

TEST(Base64, EncodedOutputOnlyUsesStandardAlphabet) {
  const Bytes data = ciutil::testing_util::lcg_bytes(300);
  for (const char c : base64_encode(data)) {
    const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                    c == '+' || c == '/' || c == '=';
    EXPECT_TRUE(ok) << "character " << static_cast<int>(c);
  }
}

}  // namespace
