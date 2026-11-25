#ifndef CIUTIL_TEST_HARNESS_HPP
#define CIUTIL_TEST_HARNESS_HPP

#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace harness {

struct TestCase {
  const char* name;
  void (*function)();
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> cases;
  return cases;
}

struct Counters {
  int checks = 0;
  int failures = 0;
};

inline Counters& counters() {
  static Counters instance;
  return instance;
}

// registers a test at static initialization time
struct Registrar {
  Registrar(const char* name, void (*function)()) { registry().push_back(TestCase{name, function}); }
};

template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
    : std::true_type {};

template <typename T>
std::string describe(const T& value) {
  if constexpr (std::is_same_v<T, unsigned char> || std::is_same_v<T, signed char>) {
    return std::to_string(static_cast<int>(value));
  } else if constexpr (std::is_enum_v<T>) {
    return std::to_string(static_cast<long long>(static_cast<std::underlying_type_t<T>>(value)));
  } else if constexpr (is_streamable<T>::value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
  } else {
    return "<unprintable>";
  }
}

// equality that never compares mixed signedness integers directly
template <typename A, typename B>
bool equal(const A& a, const B& b) {
  if constexpr (std::is_integral_v<A> && std::is_integral_v<B> &&
                std::is_signed_v<A> != std::is_signed_v<B>) {
    if constexpr (std::is_signed_v<A>) {
      return a >= 0 && static_cast<std::make_unsigned_t<A>>(a) == b;
    } else {
      return b >= 0 && a == static_cast<std::make_unsigned_t<B>>(b);
    }
  } else {
    return a == b;
  }
}

inline void report(const char* file, int line, const std::string& message) {
  ++counters().failures;
  std::cerr << file << ':' << line << ": " << message << '\n';
}

inline void check(bool condition, const char* expression, const char* file, int line) {
  ++counters().checks;
  if (!condition) {
    report(file, line, std::string("check failed: ") + expression);
  }
}

template <typename A, typename B>
void check_eq(const A& actual, const B& expected, const char* actual_text, const char* expected_text,
              const char* file, int line) {
  ++counters().checks;
  if (!equal(actual, expected)) {
    report(file, line,
           std::string("check failed: ") + actual_text + " == " + expected_text + ": got " +
               describe(actual) + ", expected " + describe(expected));
  }
}

template <typename A, typename B>
void check_ne(const A& actual, const B& other, const char* actual_text, const char* other_text,
              const char* file, int line) {
  ++counters().checks;
  if (equal(actual, other)) {
    report(file, line,
           std::string("check failed: ") + actual_text + " != " + other_text + ": both are " +
               describe(actual));
  }
}

// runs every registered test; nonzero when anything failed or nothing ran
inline int run_all() {
  int exit_code = 0;
  for (const TestCase& test : registry()) {
    const int before = counters().failures;
    try {
      test.function();
    } catch (const std::exception& error) {
      report(test.name, 0, std::string("uncaught exception: ") + error.what());
    } catch (...) {
      report(test.name, 0, "uncaught non-standard exception");
    }
    const bool passed = counters().failures == before;
    std::cout << (passed ? "ok   " : "FAIL ") << test.name << '\n';
    if (!passed) {
      exit_code = 1;
    }
  }
  std::cout << registry().size() << " tests, " << counters().checks << " checks, "
            << counters().failures << " failures\n";
  if (registry().empty() || counters().checks == 0) {
    exit_code = 1;
  }
  return exit_code;
}

}  // namespace harness

#define HARNESS_CONCAT_INNER(a, b) a##b
#define HARNESS_CONCAT(a, b) HARNESS_CONCAT_INNER(a, b)

#define TEST(name)                                                                     \
  static void name();                                                                  \
  namespace {                                                                          \
  const ::harness::Registrar HARNESS_CONCAT(registrar_, name)(#name, &name);           \
  }                                                                                    \
  static void name()

#define CHECK(condition) ::harness::check(static_cast<bool>(condition), #condition, __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) \
  ::harness::check_eq((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define CHECK_NE(actual, other) \
  ::harness::check_ne((actual), (other), #actual, #other, __FILE__, __LINE__)

#endif
