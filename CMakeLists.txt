cmake_minimum_required(VERSION 3.22)

project(SmartChordArp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(src)

option(SMARTCHORD_BUILD_TESTS "Build unit tests" ON)
if(SMARTCHORD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
