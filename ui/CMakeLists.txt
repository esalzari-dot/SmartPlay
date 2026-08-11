juce_add_gui_app(SmartChordArpUI
    PRODUCT_NAME "SmartPlay"
    COMPANY_NAME "SmartPlay"
)

target_sources(SmartChordArpUI PRIVATE
    Source/Main.cpp
    Source/StandaloneComponent.cpp
    Components/AutoplayGridPanel.cpp
    Components/FamilySwitcher.cpp
    Components/ChordPadRow.cpp
    Components/PlayStripRow.cpp
    Components/AutoplayGridComponent.cpp
    Components/PatternReadout.cpp
    Components/ChordLabel.cpp
    Components/KeyboardChordShortcuts.cpp
)

target_include_directories(SmartChordArpUI PRIVATE Components)

target_compile_features(SmartChordArpUI PRIVATE cxx_std_17)

target_compile_definitions(SmartChordArpUI PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="\"SmartPlay\""
    JUCE_APPLICATION_VERSION_STRING="\"0.1.0\""
    SMARTCHORD_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

target_link_libraries(SmartChordArpUI PRIVATE
    smartchord_core
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
)
