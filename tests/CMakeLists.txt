include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_executable(smartchord_tests
    ChordDefinitionTests.cpp
    VoicingEngineTests.cpp
    PatternLibraryTests.cpp
    AutoplayGridStateTests.cpp
    ArpeggiatorEngineTests.cpp
    MidiOutputManagerTests.cpp
    ChordBankModuleTests.cpp
)

target_link_libraries(smartchord_tests PRIVATE smartchord_core Catch2::Catch2WithMain)
target_compile_definitions(smartchord_tests PRIVATE SMARTCHORD_DATA_DIR="${CMAKE_SOURCE_DIR}/data")

include(CTest)
include(Catch)
catch_discover_tests(smartchord_tests)
