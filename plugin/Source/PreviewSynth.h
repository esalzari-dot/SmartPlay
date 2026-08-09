#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace smartchord
{

// Sintesi minima per poter ascoltare SmartPlay dal vero standalone senza appoggiarsi ad
// un altro synth: due sinusoidi (fondamentale + seconda armonica piu' debole) sagomate da
// un inviluppo ADSR con decadimento anche a nota tenuta, in stile "toy piano". Non e'
// pensata per un uso musicale serio, solo per provare accordi e pattern - percio' resta
// un dettaglio del plugin, non del motore MIDI-only del core (SPEC.md sezione 1): esiste
// solo nella variante SmartChordArpInst, e anche li' viene usata solo quando gira come
// vero standalone (vedi PluginProcessor::processBlock), mai quando e' ospitata da una DAW.
class PreviewPianoSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class PreviewPianoVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<PreviewPianoSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    double phase = 0.0;
    double phaseDelta = 0.0;
    float level = 0.0f;
    juce::ADSR adsr;
};

class PreviewSynth
{
public:
    PreviewSynth();

    void prepare (double sampleRate);
    void renderNextBlock (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiMessages,
                           int startSample, int numSamples);

private:
    juce::Synthesiser synth;
};

} // namespace smartchord
