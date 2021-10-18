cmake_minimum_required(VERSION 3.19)
project(code)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# Only add GCC flags if using GCC
if(CMAKE_COMPILER_IS_GNUCXX)
    set(cxx_flags -Wall -Wextra -Wpedantic -Wsuggest-override)
    set(cxx_linker_flags ${cxx_flags})
endif(CMAKE_COMPILER_IS_GNUCXX)

set(MAIN_SOURCES
    src/vector.cpp
    src/matrix.cpp
    src/dsop_single.cpp
    src/util.cpp
)

set(INCLUDE_DIRS include)

add_library(dphpc STATIC EXCLUDE_FROM_ALL ${MAIN_SOURCES})
target_compile_options(dphpc PUBLIC ${cxx_flags})
target_link_options(dphpc PUBLIC ${cxx_linker_flags})
target_include_directories(dphpc PUBLIC ${INCLUDE_DIRS})

add_executable(main src/main.cpp)
target_compile_options(main PUBLIC ${cxx_flags})
target_link_options(main PUBLIC ${cxx_linker_flags})
target_link_libraries(main dphpc)

###################
# Testing Framework
include(FetchContent)
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
        COMMAND /usr/bin/clang-format
        -style=file
        -i
        ${ALL_SOURCE_FILES}
        )

# Run make check to build and run all unit tests
add_custom_target(check
  COMMAND GTEST_COLOR=1 ctest --output-on-failure
  DEPENDS all_unit_tests
  )
