cmake_minimum_required(VERSION 3.21)

project(ciutil VERSION 1.0.0 DESCRIPTION "small checksum and codec utilities" LANGUAGES C)

option(CIUTIL_BUILD_TESTS "Build and register the tests" ${PROJECT_IS_TOP_LEVEL})
option(CIUTIL_INSTALL "Generate install rules" ${PROJECT_IS_TOP_LEVEL})
option(CIUTIL_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC and Clang)" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# keep executables and dlls side by side so tests find a shared library on windows
if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

# warnings and sanitizers are applied per target (privately) so they never leak into the exported package
function(ciutil_apply_flags target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion)
  endif()

  if(CIUTIL_SANITIZE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND NOT MSVC)
      target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    else()
      message(WARNING "CIUTIL_SANITIZE is only supported for GCC and Clang; ignoring for ${target}")
    endif()
  endif()
endfunction()

add_library(ciutil
  src/adler32.c
  src/base64.c
  src/cli.c
  src/common.c
  src/crc32.c
  src/roman.c
  src/semver.c)
add_library(ciutil::ciutil ALIAS ciutil)

target_include_directories(ciutil PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_compile_definitions(ciutil PRIVATE CIUTIL_BUILDING)

get_target_property(ciutil_type ciutil TYPE)
if(ciutil_type STREQUAL "SHARED_LIBRARY")
  target_compile_definitions(ciutil PUBLIC CIUTIL_SHARED)
endif()

set_target_properties(ciutil PROPERTIES
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})
ciutil_apply_flags(ciutil)

add_executable(ciutil_cli src/main.c)
target_link_libraries(ciutil_cli PRIVATE ciutil)
set_target_properties(ciutil_cli PROPERTIES
  OUTPUT_NAME ciutil
  PDB_NAME ciutil_cli)
ciutil_apply_flags(ciutil_cli)

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
  install(FILES include/ciutil.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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
