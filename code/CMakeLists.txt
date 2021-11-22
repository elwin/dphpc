cmake_minimum_required(VERSION 3.19)
project(code)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID}")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "No build type specified; using ${CMAKE_BUILD_TYPE}")
endif()
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)

list(APPEND cxx_base -Wall -Wextra -Wpedantic -Wsuggest-override)
list(APPEND cxx_release -ffast-math -mavx -mavx2 -mfma -ftree-vectorizer-verbose=6)
list(APPEND cxx_debug -DDEBUG -g2)
list(APPEND cxx_sanitize -O0 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls)

list(APPEND cxx_linker_base "")

list(APPEND cxx_flags ${cxx_base})
list(APPEND cxx_linker_flags ${cxx_linker_base})

if (CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
  list(APPEND cxx_flags ${cxx_debug})
elseif (CMAKE_BUILD_TYPE_UPPER STREQUAL "SANITIZE")
  list(APPEND cxx_flags ${cxx_sanitize})
elseif (CMAKE_BUILD_TYPE_UPPER STREQUAL "RELEASE")
  list(APPEND cxx_flags ${cxx_release})
endif()

list(APPEND cxx_linker_flags ${cxx_flags})

set(MAIN_SOURCES
        src/vector.cpp
        src/matrix.cpp
        src/dsop_single.cpp
        src/util.cpp
        src/allreduce/impl.cpp
        src/allreduce_butterfly/impl.cpp
        src/allreduce_rabenseifner/impl.cpp
        src/rabenseifner_gather/impl.cpp
        src/rabenseifner_scatter/impl.cpp
        src/allreduce_ring/impl.cpp
        src/allgather/impl.cpp
        src/allgather_async/impl.cpp
        )

set(INCLUDE_DIRS include)

FetchContent_Declare(json URL https://github.com/nlohmann/json/archive/refs/tags/v3.10.4.zip)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

find_package(MPI REQUIRED)

add_library(dphpc STATIC EXCLUDE_FROM_ALL ${MAIN_SOURCES})
target_compile_options(dphpc PUBLIC ${cxx_flags})
target_link_options(dphpc PUBLIC ${cxx_linker_flags})
target_include_directories(dphpc PUBLIC ${INCLUDE_DIRS})
target_link_libraries(dphpc PUBLIC MPI::MPI_CXX)
target_link_libraries(dphpc PUBLIC nlohmann_json::nlohmann_json)

add_executable(main src/main.cpp)
target_compile_options(main PUBLIC ${cxx_flags})
target_link_options(main PUBLIC ${cxx_linker_flags})
target_link_libraries(main dphpc)

###################
# Testing Framework
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
###################

enable_testing()
include(GoogleTest)

# Compile all unit tests with 'make all_unit_tests'
add_custom_target(all_unit_tests
        COMMENT "Building all unit test")

function(add_unit_test source_file)
  string(REPLACE "/" "_" testname ${source_file})
  set(name "unit_test.${testname}")

  set(fname "tests/${source_file}.cpp")

  add_executable(${name} ${fname})

  # Link against gmock (this automatically links against gtest)
  target_link_libraries(${name} dphpc gmock_main)
  gtest_discover_tests(${name})

  add_dependencies(all_unit_tests ${name})
endfunction()

add_unit_test(matrix_test)
add_unit_test(dsop_test)

file(GLOB_RECURSE ALL_SOURCE_FILES *.c *.cpp *.h *.hpp)
list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX "${CMAKE_BINARY_DIR}/.*")

add_custom_target(
        clangformat
        COMMAND clang-format
        -style=file
        -i
        ${ALL_SOURCE_FILES}
)

# Run make check to build and run all unit tests
add_custom_target(check
        COMMAND GTEST_COLOR=1 ctest --output-on-failure
        DEPENDS all_unit_tests
        )
