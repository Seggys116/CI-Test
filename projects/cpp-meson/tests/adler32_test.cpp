#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ciutil/adler32.hpp"
#include "harness.hpp"
#include "test_util.hpp"

namespace {

struct Vector {
  std::string_view text;
  std::uint32_t expected;
};

constexpr std::array<Vector, 5> kVectors{{
    {"", 0x00000001u},
    {"a", 0x00620062u},
    {"abc", 0x024d0127u},
    {"Wikipedia", 0x11e60398u},
    {"123456789", 0x091e01deu},
}};

// naive reference: modulo after every byte
std::uint32_t reference_adler32(const ciutil::Bytes& data, std::size_t length) {
  std::uint32_t a = 1;
  std::uint32_t b = 0;
  for (std::size_t i = 0; i < length; ++i) {
    a = (a + static_cast<std::uint32_t>(data[i])) % 65521u;
    b = (b + a) % 65521u;
  }
  return (b << 16) | a;
}

}  // namespace

TEST(adler32_known_vectors) {
  for (const Vector& vector : kVectors) {
    CHECK_EQ(ciutil::adler32(vector.text), vector.expected);
  }
}

TEST(adler32_empty_view) {
  CHECK_EQ(ciutil::adler32(ciutil::ByteView()), 1u);
}

TEST(adler32_long_run_of_ff_matches_reference) {
  const ciutil::Bytes data(100000, 0xFF);
  CHECK_EQ(ciutil::adler32(data), reference_adler32(data, data.size()));
}

TEST(adler32_lengths_around_nmax_match_reference) {
  const std::array<std::size_t, 9> lengths{0, 1, 5551, 5552, 5553, 11103, 11104, 11105, 100000};
  const ciutil::Bytes ones(100000, 0xFF);
  const ciutil::Bytes random = testutil::lcg_bytes(100000);
  for (const std::size_t length : lengths) {
    CHECK_EQ(ciutil::adler32(ciutil::ByteView(ones.data(), length)),
             reference_adler32(ones, length));
    CHECK_EQ(ciutil::adler32(ciutil::ByteView(random.data(), length)),
             reference_adler32(random, length));
  }
}

TEST(adler32_chunked_equals_one_shot_at_every_split) {
  const ciutil::Bytes data = testutil::lcg_bytes(1000);
  const std::uint32_t whole = ciutil::adler32(data);
  CHECK_EQ(whole, reference_adler32(data, data.size()));
  for (std::size_t split = 0; split <= data.size(); ++split) {
    ciutil::Adler32 sum;
    sum.update(ciutil::ByteView(data.data(), split));
    sum.update(ciutil::ByteView(data.data() + split, data.size() - split));
    CHECK_EQ(sum.value(), whole);
  }
}

TEST(adler32_incremental_across_nmax_boundary) {
  const ciutil::Bytes data(100000, 0xFF);
  const std::uint32_t whole = ciutil::adler32(data);
  const std::array<std::size_t, 7> splits{1, 5551, 5552, 5553, 11104, 50000, 99999};
  for (const std::size_t split : splits) {
    ciutil::Adler32 sum;
    sum.update(ciutil::ByteView(data.data(), split));
    sum.update(ciutil::ByteView(data.data() + split, data.size() - split));
    CHECK_EQ(sum.value(), whole);
  }
}

TEST(adler32_value_does_not_consume_state_and_reset_works) {
  const ciutil::Bytes data = testutil::lcg_bytes(64);
  ciutil::Adler32 sum;
  CHECK_EQ(sum.value(), 1u);
  sum.update(data);
  const std::uint32_t first = sum.value();
  CHECK_EQ(sum.value(), first);
  sum.reset();
  CHECK_EQ(sum.value(), 1u);
  sum.update(data);
  CHECK_EQ(sum.value(), first);
}

int main() {
  return harness::run_all();
}
