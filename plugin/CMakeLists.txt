# AU e' un formato Apple: buildabile solo su macOS (richiede il SDK Audio Unit di sistema).
if(APPLE)
    set(SMARTCHORD_PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(SMARTCHORD_PLUGIN_FORMATS VST3 Standalone)
endif()

# Il dataset dei pattern viene embeddato nel binario (SPEC.md sezione 5.4, CLAUDE.md
# "JSON esterno o BinaryData embeddato"): un plugin distribuito non puo' dipendere da un
# percorso dell'albero sorgente della macchina che lo ha compilato.
juce_add_binary_data(SmartChordArpData
    SOURCES ${CMAKE_SOURCE_DIR}/data/patterns.json
)

set(SMARTCHORD_PLUGIN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ../ui/Components/AutoplayGridPanel.cpp
    ../ui/Components/FamilySwitcher.cpp
    ../ui/Components/ChordPadRow.cpp
    ../ui/Components/AutoplayGridComponent.cpp
    ../ui/Components/PatternReadout.cpp
    ../ui/Components/ChordLabel.cpp
    ../ui/Components/KeyboardChordShortcuts.cpp
)

# Impostazioni condivise dalle due varianti del plugin (MIDI FX e strumento).
function(smartchord_configure_plugin target)
    target_sources(${target} PRIVATE ${SMARTCHORD_PLUGIN_SOURCES})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ui/Components)
    target_compile_features(${target} PRIVATE cxx_std_17)

    target_compile_definitions(${target} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_ALSA=1
        JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(${target} PRIVATE
        smartchord_core
        SmartChordArpData
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
    )
endfunction()

# Variante MIDI FX: quella descritta da SPEC.md sezione 1 (isMidiEffect() == true, nessun
# bus audio). Supportata da Cubase, Reaper, Studio One, FL Studio, Logic (AU).
juce_add_plugin(SmartChordArp
    PRODUCT_NAME "Smart Chord Arpeggiator"
    COMPANY_NAME "SmartPlay"
    BUNDLE_ID "com.smartplay.smartchordarp"
    PLUGIN_MANUFACTURER_CODE Splr
    PLUGIN_CODE Scap
    FORMATS ${SMARTCHORD_PLUGIN_FORMATS}
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
smartchord_configure_plugin(SmartChordArp)

# Variante strumento: stesso motore, ma dichiarato come VSTi con un'uscita audio (che
# resta silenziosa). Serve per gli host che non ospitano i MIDI FX VST3 - Ableton Live in
# particolare: li' si carica su una traccia MIDI e si preleva il suo MIDI da un'altra
# traccia con "MIDI From".
juce_add_plugin(SmartChordArpInst
    PRODUCT_NAME "Smart Chord Arpeggiator Inst"
    COMPANY_NAME "SmartPlay"
    BUNDLE_ID "com.smartplay.smartchordarpinst"
    PLUGIN_MANUFACTURER_CODE Splr
    PLUGIN_CODE Scai
    FORMATS ${SMARTCHORD_PLUGIN_FORMATS}
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
smartchord_configure_plugin(SmartChordArpInst)

# Synth di anteprima (vedi Source/PreviewSynth.h): usato solo dal vero standalone di
# questa variante, non dalla MIDI FX. Compilarlo solo qui tiene il binario MIDI FX privo
# di qualunque codice di sintesi, coerente con SPEC.md sezione 1.
target_sources(SmartChordArpInst PRIVATE Source/PreviewSynth.cpp)
