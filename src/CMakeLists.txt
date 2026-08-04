add_library(smartchord_core STATIC
    smartchord/ChordDefinition.cpp
    smartchord/VoicingEngine.cpp
)

target_include_directories(smartchord_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(smartchord_core PUBLIC cxx_std_17)
