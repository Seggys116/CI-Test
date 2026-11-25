cmake_minimum_required(VERSION 3.21)

project(ciutil VERSION 1.0.0 DESCRIPTION "small checksum and codec utilities" LANGUAGES CXX)

option(CIUTIL_BUILD_TESTS "Build and register the tests (fetches GoogleTest)" ${PROJECT_IS_TOP_LEVEL})
option(CIUTIL_INSTALL "Generate install rules" ${PROJECT_IS_TOP_LEVEL})
option(CIUTIL_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC and Clang)" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

# sanitizers are global so that dependencies such as GoogleTest are instrumented too
if(CIUTIL_SANITIZE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  else()
    message(WARNING "CIUTIL_SANITIZE is only supported for GCC and Clang; ignoring")
  endif()
endif()

# warnings are applied per target (privately) so they never reach dependencies or the exported package
function(ciutil_apply_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

add_library(ciutil STATIC
  src/adler32.cpp
  src/base64.cpp
  src/cli.cpp
  src/crc32.cpp
  src/roman.cpp
  src/semver.cpp)
add_library(ciutil::ciutil ALIAS ciutil)

target_include_directories(ciutil PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_compile_features(ciutil PUBLIC cxx_std_20)
set_target_properties(ciutil PROPERTIES POSITION_INDEPENDENT_CODE ON)
ciutil_apply_warnings(ciutil)

add_executable(ciutil_cli src/main.cpp)
target_link_libraries(ciutil_cli PRIVATE ciutil)
set_target_properties(ciutil_cli PROPERTIES OUTPUT_NAME ciutil)
ciutil_apply_warnings(ciutil_cli)

if(CIUTIL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(CIUTIL_INSTALL)
  install(TARGETS ciutil
    EXPORT ciutilTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(TARGETS ciutil_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(DIRECTORY include/ciutil DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  set(ciutil_config_dir "${CMAKE_INSTALL_LIBDIR}/cmake/ciutil")
  install(EXPORT ciutilTargets
    NAMESPACE ciutil::
    FILE ciutilTargets.cmake
    DESTINATION ${ciutil_config_dir})

  configure_package_config_file(
    cmake/ciutilConfig.cmake.in
    "${PROJECT_BINARY_DIR}/ciutilConfig.cmake"
    INSTALL_DESTINATION ${ciutil_config_dir})
  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/ciutilConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  install(FILES
    "${PROJECT_BINARY_DIR}/ciutilConfig.cmake"
    "${PROJECT_BINARY_DIR}/ciutilConfigVersion.cmake"
    DESTINATION ${ciutil_config_dir})
endif()
