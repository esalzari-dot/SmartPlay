cmake_minimum_required(VERSION 3.22)

project(SmartChordArp VERSION 0.1.0 LANGUAGES CXX C)

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

option(SMARTCHORD_BUILD_UI "Build the JUCE-based UI test harness" ON)
if(SMARTCHORD_BUILD_UI)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)

    add_subdirectory(ui)
endif()
