#include "ciutil/cli.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>
#include <string_view>

#include "ciutil/adler32.hpp"
#include "ciutil/base64.hpp"
#include "ciutil/crc32.hpp"
#include "ciutil/roman.hpp"
#include "ciutil/semver.hpp"

namespace ciutil {

namespace {

constexpr int kExitOk = 0;
constexpr int kExitError = 2;
constexpr int kExitUsage = 64;

constexpr std::string_view kProjectName = "cpp-cmake";
constexpr std::string_view kVersion = "1.0.0";

int usage(std::ostream& err) {
  err << "usage: ciutil <command> [arguments]\n"
         "  crc32 STRING\n"
         "  adler32 STRING\n"
         "  b64enc STRING\n"
         "  b64dec BASE64\n"
         "  roman NUMBER\n"
         "  unroman ROMAN\n"
         "  semver-cmp A B\n"
         "  version\n";
  return kExitUsage;
}

int fail(std::ostream& err, std::string_view what, Error error) {
  err << "ciutil: " << what << ": " << error_message(error) << '\n';
  return kExitError;
}

std::string hex8(std::uint32_t value) {
  constexpr std::string_view digits = "0123456789abcdef";
  std::string out(8, '0');
  for (std::size_t i = out.size(); i-- > 0;) {
    out[i] = digits[value & 0xFu];
    value >>= 4;
  }
  return out;
}

// strict decimal with optional leading minus; nullopt on junk or int overflow
std::optional<int> parse_int(std::string_view text) {
  bool negative = false;
  if (!text.empty() && text.front() == '-') {
    negative = true;
    text.remove_prefix(1);
  }
  if (text.empty()) {
    return std::nullopt;
  }
  int value = 0;
  for (const char c : text) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    const int digit = c - '0';
    if (value > (std::numeric_limits<int>::max() - digit) / 10) {
      return std::nullopt;
    }
    value = value * 10 + digit;
  }
  return negative ? -value : value;
}

int cmd_b64dec(std::string_view text, std::ostream& out, std::ostream& err) {
  const Result<Bytes> decoded = base64_decode(text);
  if (!decoded.ok()) {
    return fail(err, "b64dec", decoded.error());
  }
  const Bytes& bytes = decoded.value();
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return kExitOk;
}

int cmd_roman(std::string_view text, std::ostream& out, std::ostream& err) {
  const std::optional<int> value = parse_int(text);
  if (!value) {
    err << "ciutil: roman: invalid number: " << text << '\n';
    return kExitError;
  }
  const Result<std::string> roman = to_roman(*value);
  if (!roman.ok()) {
    return fail(err, "roman", roman.error());
  }
  out << roman.value() << '\n';
  return kExitOk;
}

int cmd_unroman(std::string_view text, std::ostream& out, std::ostream& err) {
  const Result<int> value = from_roman(text);
  if (!value.ok()) {
    return fail(err, "unroman", value.error());
  }
  out << value.value() << '\n';
  return kExitOk;
}

int cmd_semver_cmp(std::string_view a_text, std::string_view b_text, std::ostream& out,
                   std::ostream& err) {
  const Result<SemVer> a = parse_semver(a_text);
  if (!a.ok()) {
    return fail(err, "semver-cmp", a.error());
  }
  const Result<SemVer> b = parse_semver(b_text);
  if (!b.ok()) {
    return fail(err, "semver-cmp", b.error());
  }
  out << compare(a.value(), b.value()) << '\n';
  return kExitOk;
}

}  // namespace

int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) {
  if (args.empty()) {
    return usage(err);
  }
  const std::string_view command = args[0];
  const std::size_t operands = args.size() - 1;

  if (command == "version" && operands == 0) {
    out << "ciutil " << kProjectName << ' ' << kVersion << '\n';
    return kExitOk;
  }
  if (command == "crc32" && operands == 1) {
    out << hex8(crc32(to_bytes(args[1]))) << '\n';
    return kExitOk;
  }
  if (command == "adler32" && operands == 1) {
    out << hex8(adler32(to_bytes(args[1]))) << '\n';
    return kExitOk;
  }
  if (command == "b64enc" && operands == 1) {
    out << base64_encode(to_bytes(args[1])) << '\n';
    return kExitOk;
  }
  if (command == "b64dec" && operands == 1) {
    return cmd_b64dec(args[1], out, err);
  }
  if (command == "roman" && operands == 1) {
    return cmd_roman(args[1], out, err);
  }
  if (command == "unroman" && operands == 1) {
    return cmd_unroman(args[1], out, err);
  }
  if (command == "semver-cmp" && operands == 2) {
    return cmd_semver_cmp(args[1], args[2], out, err);
  }
  return usage(err);
}

}  // namespace ciutil
