#include <array>
#include <sstream>
#include <string>
#include <vector>

#include "ciutil/cli.hpp"
#include "harness.hpp"

namespace {

struct Outcome {
  int code;
  std::string out;
  std::string err;
};

Outcome run_cli(const std::vector<std::string>& args) {
  std::ostringstream out;
  std::ostringstream err;
  const int code = ciutil::run(args, out, err);
  return Outcome{code, out.str(), err.str()};
}

struct Success {
  std::vector<std::string> args;
  std::string stdout_text;
};

struct Failure {
  std::vector<std::string> args;
  int code;
};

}  // namespace

TEST(cli_prints_expected_output) {
  const std::vector<Success> cases{
      {{"version"}, "ciutil cpp-meson 1.0.0\n"},
      {{"crc32", ""}, "00000000\n"},
      {{"crc32", "a"}, "e8b7be43\n"},
      {{"crc32", "abc"}, "352441c2\n"},
      {{"crc32", "123456789"}, "cbf43926\n"},
      {{"crc32", "The quick brown fox jumps over the lazy dog"}, "414fa339\n"},
      {{"adler32", ""}, "00000001\n"},
      {{"adler32", "a"}, "00620062\n"},
      {{"adler32", "abc"}, "024d0127\n"},
      {{"adler32", "Wikipedia"}, "11e60398\n"},
      {{"adler32", "123456789"}, "091e01de\n"},
      {{"b64enc", ""}, "\n"},
      {{"b64enc", "f"}, "Zg==\n"},
      {{"b64enc", "fo"}, "Zm8=\n"},
      {{"b64enc", "foobar"}, "Zm9vYmFy\n"},
      {{"b64dec", "Zm9vYmFy"}, "foobar"},
      {{"b64dec", ""}, ""},
      {{"b64dec", "Zg=="}, "f"},
      {{"roman", "1"}, "I\n"},
      {{"roman", "4"}, "IV\n"},
      {{"roman", "1994"}, "MCMXCIV\n"},
      {{"roman", "2024"}, "MMXXIV\n"},
      {{"roman", "3999"}, "MMMCMXCIX\n"},
      {{"unroman", "I"}, "1\n"},
      {{"unroman", "XIV"}, "14\n"},
      {{"unroman", "MCMXCIV"}, "1994\n"},
      {{"unroman", "MMMCMXCIX"}, "3999\n"},
      {{"semver-cmp", "1.0.0-alpha", "1.0.0"}, "-1\n"},
      {{"semver-cmp", "1.0.0", "1.0.0-alpha"}, "1\n"},
      {{"semver-cmp", "1.0.0+a", "1.0.0+b"}, "0\n"},
      {{"semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11"}, "-1\n"},
      {{"semver-cmp", "2.1.1", "2.1.0"}, "1\n"},
  };
  for (const Success& expected : cases) {
    const Outcome outcome = run_cli(expected.args);
    CHECK_EQ(outcome.code, 0);
    CHECK_EQ(outcome.out, expected.stdout_text);
    CHECK(outcome.err.empty());
  }
}

TEST(cli_b64dec_writes_raw_binary_bytes) {
  const Outcome outcome = run_cli({"b64dec", "AP/+"});
  CHECK_EQ(outcome.code, 0);
  CHECK_EQ(outcome.out, std::string("\x00\xff\xfe", 3));
  CHECK(outcome.err.empty());
}

TEST(cli_failures_report_on_stderr_with_exit_code) {
  const std::vector<Failure> cases{
      {{"b64dec", "Zg="}, 2},
      {{"b64dec", "Zh=="}, 2},
      {{"b64dec", "Zm-v"}, 2},
      {{"roman", "0"}, 2},
      {{"roman", "4000"}, 2},
      {{"roman", "-5"}, 2},
      {{"roman", "abc"}, 2},
      {{"roman", ""}, 2},
      {{"roman", "99999999999999999999"}, 2},
      {{"unroman", "IIII"}, 2},
      {{"unroman", ""}, 2},
      {{"unroman", "xiv"}, 2},
      {{"semver-cmp", "1.0", "1.0.0"}, 2},
      {{"semver-cmp", "1.0.0", "v2.0.0"}, 2},
      {{}, 64},
      {{"bogus"}, 64},
      {{"crc32"}, 64},
      {{"crc32", "a", "b"}, 64},
      {{"adler32"}, 64},
      {{"b64enc"}, 64},
      {{"b64dec", "Zg==", "x"}, 64},
      {{"roman"}, 64},
      {{"unroman", "I", "II"}, 64},
      {{"semver-cmp", "1.0.0"}, 64},
      {{"semver-cmp", "1.0.0", "1.0.0", "1.0.0"}, 64},
      {{"version", "extra"}, 64},
  };
  for (const Failure& expected : cases) {
    const Outcome outcome = run_cli(expected.args);
    CHECK_EQ(outcome.code, expected.code);
    CHECK(outcome.out.empty());
    CHECK(!outcome.err.empty());
  }
}

TEST(cli_never_emits_carriage_returns) {
  const std::vector<std::vector<std::string>> invocations{
      {"crc32", "abc"}, {"bogus"}, {"roman", "0"}, {"version"}, {}};
  for (const auto& args : invocations) {
    const Outcome outcome = run_cli(args);
    CHECK(outcome.out.find('\r') == std::string::npos);
    CHECK(outcome.err.find('\r') == std::string::npos);
  }
}

int main() {
  return harness::run_all();
}
