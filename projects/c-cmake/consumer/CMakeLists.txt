cmake_minimum_required(VERSION 3.21)

project(ciutil_consumer LANGUAGES C)

find_package(ciutil 1.0 CONFIG REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE ciutil::ciutil)

# a shared ciutil installs a dll that must sit next to the executable on windows
if(WIN32)
  add_custom_command(TARGET consumer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:consumer>>,copy,true>
            $<TARGET_RUNTIME_DLLS:consumer> $<TARGET_FILE_DIR:consumer>
    COMMAND_EXPAND_LISTS)
endif()
