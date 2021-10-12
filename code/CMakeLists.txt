cmake_minimum_required(VERSION 3.19)
project(code)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)


add_executable(code main.cpp vector.cpp matrix.cpp)


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

add_executable(
        matrix_test
        matrix_test.cpp
        matrix.cpp
)
target_link_libraries(
        matrix_test
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(matrix_test)