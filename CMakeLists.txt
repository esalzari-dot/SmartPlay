cmake_minimum_required(VERSION 3.22)

project(SmartChordArp VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# I binari distribuiti non devono dipendere dalla presenza del VC++ Redistributable
# sulla macchina dell'utente finale.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
option(SMARTCHORD_BUILD_PLUGIN "Build the VST3/Standalone JUCE plugin" ON)

if(SMARTCHORD_BUILD_UI OR SMARTCHORD_BUILD_PLUGIN)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

if(SMARTCHORD_BUILD_UI)
    add_subdirectory(ui)
endif()

if(SMARTCHORD_BUILD_PLUGIN)
    add_subdirectory(plugin)
endif()
