#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include "ciutil/adler32.hpp"
#include "ciutil/crc32.hpp"
#include "test_util.hpp"

namespace {

// typed tests: the incremental interface behaves the same for both checksums

struct Crc32Traits {
  using Hasher = ciutil::Crc32;
  static std::uint32_t one_shot(ciutil::ByteSpan data) { return ciutil::crc32(data); }
  static constexpr std::uint32_t kEmpty = 0x00000000u;
};

struct Adler32Traits {
  using Hasher = ciutil::Adler32;
  static std::uint32_t one_shot(ciutil::ByteSpan data) { return ciutil::adler32(data); }
  static constexpr std::uint32_t kEmpty = 0x00000001u;
};

template <typename Traits>
class IncrementalChecksumTest : public ::testing::Test {};

using Checksums = ::testing::Types<Crc32Traits, Adler32Traits>;
TYPED_TEST_SUITE(IncrementalChecksumTest, Checksums);

TYPED_TEST(IncrementalChecksumTest, FreshHasherIsTheEmptyChecksum) {
  const typename TypeParam::Hasher hasher{};
  EXPECT_EQ(hasher.value(), TypeParam::kEmpty);
}

TYPED_TEST(IncrementalChecksumTest, ChunkedEqualsOneShotAtEverySplit) {
  const ciutil::Bytes data = ciutil::testing_util::lcg_bytes(1000);
  const std::uint32_t whole = TypeParam::one_shot(data);
  for (std::size_t split = 0; split <= data.size(); ++split) {
    typename TypeParam::Hasher hasher;
    hasher.update(ciutil::ByteSpan(data.data(), split));
    hasher.update(ciutil::ByteSpan(data.data() + split, data.size() - split));
    EXPECT_EQ(hasher.value(), whole) << "split " << split;
  }
}

TYPED_TEST(IncrementalChecksumTest, ThreeWaySplitEqualsOneShot) {
  const ciutil::Bytes data = ciutil::testing_util::lcg_bytes(1000);
  const std::uint32_t whole = TypeParam::one_shot(data);
  for (std::size_t first = 0; first <= data.size(); first += 37) {
    for (std::size_t second = first; second <= data.size(); second += 101) {
      typename TypeParam::Hasher hasher;
      hasher.update(ciutil::ByteSpan(data.data(), first));
      hasher.update(ciutil::ByteSpan(data.data() + first, second - first));
      hasher.update(ciutil::ByteSpan(data.data() + second, data.size() - second));
      EXPECT_EQ(hasher.value(), whole) << first << "," << second;
    }
  }
}

TYPED_TEST(IncrementalChecksumTest, ByteAtATimeEqualsOneShot) {
  const ciutil::Bytes data = ciutil::testing_util::lcg_bytes(1000);
  typename TypeParam::Hasher hasher;
  for (std::size_t i = 0; i < data.size(); ++i) {
    hasher.update(ciutil::ByteSpan(data.data() + i, 1));
  }
  EXPECT_EQ(hasher.value(), TypeParam::one_shot(data));
}

TYPED_TEST(IncrementalChecksumTest, ValueDoesNotConsumeState) {
  const ciutil::Bytes data = ciutil::testing_util::lcg_bytes(64);
  typename TypeParam::Hasher hasher;
  hasher.update(data);
  const std::uint32_t first = hasher.value();
  EXPECT_EQ(hasher.value(), first);
  hasher.update(data);
  EXPECT_NE(hasher.value(), first);
}

TYPED_TEST(IncrementalChecksumTest, ResetReturnsToEmpty) {
  const ciutil::Bytes data = ciutil::testing_util::lcg_bytes(64);
  typename TypeParam::Hasher hasher;
  hasher.update(data);
  hasher.reset();
  EXPECT_EQ(hasher.value(), TypeParam::kEmpty);
  hasher.update(data);
  EXPECT_EQ(hasher.value(), TypeParam::one_shot(data));
}

TYPED_TEST(IncrementalChecksumTest, EmptyUpdateIsANoOp) {
  typename TypeParam::Hasher hasher;
  hasher.update(ciutil::ByteSpan{});
  EXPECT_EQ(hasher.value(), TypeParam::kEmpty);
}

}  // namespace
