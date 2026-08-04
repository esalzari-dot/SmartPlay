cmake_minimum_required(VERSION 3.22)

project(SmartChordArp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

option(SMARTCHORD_BUILD_TESTS "Build unit tests" ON)
if(SMARTCHORD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
