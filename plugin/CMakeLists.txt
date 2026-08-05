juce_add_plugin(SmartChordArp
    PRODUCT_NAME "Smart Chord Arpeggiator"
    COMPANY_NAME "SmartPlay"
    BUNDLE_ID "com.smartplay.smartchordarp"
    PLUGIN_MANUFACTURER_CODE Splr
    PLUGIN_CODE Scap
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(SmartChordArp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ../ui/Components/AutoplayGridPanel.cpp
    ../ui/Components/FamilySwitcher.cpp
    ../ui/Components/ChordPadRow.cpp
    ../ui/Components/AutoplayGridComponent.cpp
    ../ui/Components/PatternReadout.cpp
    ../ui/Components/ChordLabel.cpp
)

target_include_directories(SmartChordArp PRIVATE ../ui/Components)

target_compile_features(SmartChordArp PRIVATE cxx_std_17)

target_compile_definitions(SmartChordArp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_ALSA=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    SMARTCHORD_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

target_link_libraries(SmartChordArp PRIVATE
    smartchord_core
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
)
