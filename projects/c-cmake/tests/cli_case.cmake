# run with: cmake -DCLI=<exe> -DARGS=<a|b|c> -DEXPECT_RC=<n> [-DEXPECT_OUT=<text>] [-DNO_NEWLINE=ON] -P cli_case.cmake
# expect_out is compared against stdout including the trailing newline unless no_newline is set

foreach(required IN ITEMS CLI ARGS EXPECT_RC)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "cli_case: ${required} is not defined")
  endif()
endforeach()

string(REPLACE "|" ";" cli_args "${ARGS}")
execute_process(
  COMMAND "${CLI}" ${cli_args}
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err)

if(NOT "${rc}" STREQUAL "${EXPECT_RC}")
  message(FATAL_ERROR "exit code ${rc}, expected ${EXPECT_RC}\nstdout: [${out}]\nstderr: [${err}]")
endif()

if(EXPECT_RC STREQUAL "0")
  if(NO_NEWLINE)
    set(want "${EXPECT_OUT}")
  else()
    set(want "${EXPECT_OUT}\n")
  endif()
  if(NOT out STREQUAL want)
    message(FATAL_ERROR "stdout [${out}], expected [${want}]")
  endif()
  if(NOT err STREQUAL "")
    message(FATAL_ERROR "unexpected stderr [${err}]")
  endif()
else()
  if(NOT out STREQUAL "")
    message(FATAL_ERROR "stdout should be empty on failure, got [${out}]")
  endif()
  if(err STREQUAL "")
    message(FATAL_ERROR "stderr should describe the failure")
  endif()
endif()
