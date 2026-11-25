include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(ciutil_tests
  adler32_test.cpp
  base64_test.cpp
  checksum_test.cpp
  cli_test.cpp
  crc32_test.cpp
  roman_test.cpp
  semver_test.cpp)
target_link_libraries(ciutil_tests PRIVATE ciutil GTest::gtest_main)
ciutil_apply_warnings(ciutil_tests)
gtest_discover_tests(ciutil_tests DISCOVERY_TIMEOUT 120)

add_test(NAME cli.version COMMAND ciutil_cli version)
set_tests_properties(cli.version PROPERTIES
  PASS_REGULAR_EXPRESSION "^ciutil cpp-cmake 1\\.0\\.0\n$"
  TIMEOUT 60)
