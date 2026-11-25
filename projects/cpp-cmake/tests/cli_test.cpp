#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "ciutil/cli.hpp"

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

class CliSuccessTest : public ::testing::TestWithParam<Success> {};

TEST_P(CliSuccessTest, PrintsExpectedOutput) {
  const Success& expected = GetParam();
  const Outcome outcome = run_cli(expected.args);
  EXPECT_EQ(outcome.code, 0);
  EXPECT_EQ(outcome.out, expected.stdout_text);
  EXPECT_TRUE(outcome.err.empty()) << outcome.err;
}

INSTANTIATE_TEST_SUITE_P(
    Commands, CliSuccessTest,
    ::testing::Values(
        Success{{"version"}, "ciutil cpp-cmake 1.0.0\n"},
        Success{{"crc32", ""}, "00000000\n"}, Success{{"crc32", "a"}, "e8b7be43\n"},
        Success{{"crc32", "abc"}, "352441c2\n"}, Success{{"crc32", "123456789"}, "cbf43926\n"},
        Success{{"crc32", "The quick brown fox jumps over the lazy dog"}, "414fa339\n"},
        Success{{"adler32", ""}, "00000001\n"}, Success{{"adler32", "a"}, "00620062\n"},
        Success{{"adler32", "abc"}, "024d0127\n"}, Success{{"adler32", "Wikipedia"}, "11e60398\n"},
        Success{{"adler32", "123456789"}, "091e01de\n"}, Success{{"b64enc", ""}, "\n"},
        Success{{"b64enc", "f"}, "Zg==\n"}, Success{{"b64enc", "fo"}, "Zm8=\n"},
        Success{{"b64enc", "foobar"}, "Zm9vYmFy\n"},
        Success{{"b64dec", "Zm9vYmFy"}, "foobar"}, Success{{"b64dec", ""}, ""},
        Success{{"b64dec", "Zg=="}, "f"}, Success{{"roman", "1"}, "I\n"},
        Success{{"roman", "4"}, "IV\n"}, Success{{"roman", "1994"}, "MCMXCIV\n"},
        Success{{"roman", "2024"}, "MMXXIV\n"}, Success{{"roman", "3999"}, "MMMCMXCIX\n"},
        Success{{"unroman", "I"}, "1\n"}, Success{{"unroman", "XIV"}, "14\n"},
        Success{{"unroman", "MCMXCIV"}, "1994\n"}, Success{{"unroman", "MMMCMXCIX"}, "3999\n"},
        Success{{"semver-cmp", "1.0.0-alpha", "1.0.0"}, "-1\n"},
        Success{{"semver-cmp", "1.0.0", "1.0.0-alpha"}, "1\n"},
        Success{{"semver-cmp", "1.0.0+a", "1.0.0+b"}, "0\n"},
        Success{{"semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11"}, "-1\n"},
        Success{{"semver-cmp", "2.1.1", "2.1.0"}, "1\n"}));

TEST(Cli, B64decWritesRawBinaryBytes) {
  const Outcome outcome = run_cli({"b64dec", "AP/+"});
  EXPECT_EQ(outcome.code, 0);
  EXPECT_EQ(outcome.out, std::string("\x00\xff\xfe", 3));
  EXPECT_TRUE(outcome.err.empty());
}

struct Failure {
  std::vector<std::string> args;
  int code;
};

class CliFailureTest : public ::testing::TestWithParam<Failure> {};

TEST_P(CliFailureTest, ReportsOnStderrWithExitCode) {
  const Failure& expected = GetParam();
  const Outcome outcome = run_cli(expected.args);
  EXPECT_EQ(outcome.code, expected.code);
  EXPECT_TRUE(outcome.out.empty()) << outcome.out;
  EXPECT_FALSE(outcome.err.empty());
}

INSTANTIATE_TEST_SUITE_P(
    Errors, CliFailureTest,
    ::testing::Values(
        Failure{{"b64dec", "Zg="}, 2}, Failure{{"b64dec", "Zh=="}, 2},
        Failure{{"b64dec", "Zm-v"}, 2}, Failure{{"roman", "0"}, 2}, Failure{{"roman", "4000"}, 2},
        Failure{{"roman", "-5"}, 2}, Failure{{"roman", "abc"}, 2}, Failure{{"roman", ""}, 2},
        Failure{{"roman", "99999999999999999999"}, 2}, Failure{{"unroman", "IIII"}, 2},
        Failure{{"unroman", ""}, 2}, Failure{{"unroman", "xiv"}, 2},
        Failure{{"semver-cmp", "1.0", "1.0.0"}, 2},
        Failure{{"semver-cmp", "1.0.0", "v2.0.0"}, 2}, Failure{{}, 64},
        Failure{{"bogus"}, 64}, Failure{{"crc32"}, 64}, Failure{{"crc32", "a", "b"}, 64},
        Failure{{"adler32"}, 64}, Failure{{"b64enc"}, 64}, Failure{{"b64dec", "Zg==", "x"}, 64},
        Failure{{"roman"}, 64}, Failure{{"unroman", "I", "II"}, 64},
        Failure{{"semver-cmp", "1.0.0"}, 64}, Failure{{"semver-cmp", "1.0.0", "1.0.0", "1.0.0"}, 64},
        Failure{{"version", "extra"}, 64}));

TEST(Cli, NeverEmitsCarriageReturns) {
  const std::vector<std::vector<std::string>> invocations{
      {"crc32", "abc"}, {"bogus"}, {"roman", "0"}, {"version"}, {}};
  for (const auto& args : invocations) {
    const Outcome outcome = run_cli(args);
    EXPECT_EQ(outcome.out.find('\r'), std::string::npos);
    EXPECT_EQ(outcome.err.find('\r'), std::string::npos);
  }
}

}  // namespace
