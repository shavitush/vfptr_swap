cmake_minimum_required (VERSION 3.8)

set (CMAKE_CXX_STANDARD 20)

project ("vfptr_swap")

find_package(Catch2 CONFIG REQUIRED)
add_executable (tests tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(tests)
