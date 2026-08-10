add_library(smartchord_core STATIC
    smartchord/ChordDefinition.cpp
    smartchord/ChordRecognizer.cpp
    smartchord/ChordProgressions.cpp
    smartchord/ChordBankPresets.cpp
    smartchord/VoicingEngine.cpp
    smartchord/PatternLibrary.cpp
    smartchord/AutoplayGridState.cpp
    smartchord/ArpeggiatorEngine.cpp
    smartchord/PlayStripEngine.cpp
    smartchord/MidiOutputManager.cpp
    smartchord/ChordBankModule.cpp
    smartchord/LoopClock.cpp
)

target_include_directories(smartchord_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(smartchord_core PUBLIC cxx_std_17)
target_link_libraries(smartchord_core PRIVATE nlohmann_json::nlohmann_json)
