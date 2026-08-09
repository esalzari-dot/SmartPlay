#pragma once

#include "PluginProcessor.h"

#include "AutoplayGridPanel.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace smartchord
{

// Editor grafico del plugin: possiede una copia "lato UI" di ChordBankModule e
// AutoplayGridState (come fa l'harness standalone), e la ripropaga verso
// AudioProcessor in modo thread-safe ad ogni modifica tramite le API dedicate
// (SPEC.md sezione 8). Riusa AutoplayGridPanel, lo stesso componente dell'harness
// standalone in /ui.
class SmartChordAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit SmartChordAudioProcessorEditor (SmartChordAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void pushStateToProcessor();
    void timerCallback() override;

    // Rileva se qualcosa e' cambiato "da fuori" (keyswitch MIDI o automazione host di uno
    // dei parametri apvts di SPEC.md sezione 8) e riallinea la copia locale usata dalla
    // UI, cosi' la griglia riflette anche i cambi che non sono passati da un click.
    bool resyncFromProcessorIfNeeded();

    SmartChordAudioProcessor& processorRef;

    ChordBankModule chordBank;
    AutoplayGridState gridState;
    float lastKnownSwing = -1.0f;
    float lastKnownGate = -1.0f;
    int lastKnownOctaveRange = 0;
    PatternRate lastKnownRate = PatternRate::Normal;

    // Evita che il riallineamento a una modifica proveniente dal thread audio venga
    // rimandato indietro al processor come se fosse un'interazione dell'utente.
    bool applyingExternalChange = false;

    ui::AutoplayGridPanel panel;
};

} // namespace smartchord
