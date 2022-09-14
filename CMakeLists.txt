# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

message("Build type: ${CMAKE_BUILD_TYPE}")

project ("tasklib")
add_library(tasklib STATIC "src/tasklib.cpp" "src/simple_flag.cpp")
target_include_directories(tasklib PUBLIC "include")

project("tasklib_test")
add_executable (tasklib_test "test/tasklib_test.cpp")
target_include_directories(tasklib_test PUBLIC "include")
target_link_libraries(tasklib_test PUBLIC tasklib)
