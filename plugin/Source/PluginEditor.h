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
class SmartChordAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SmartChordAudioProcessorEditor (SmartChordAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void pushStateToProcessor();

    SmartChordAudioProcessor& processorRef;

    ChordBankModule chordBank;
    AutoplayGridState gridState;

    ui::AutoplayGridPanel panel;
};

} // namespace smartchord
