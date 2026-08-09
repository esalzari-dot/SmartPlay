#include "PreviewSynth.h"

#include <cmath>

namespace smartchord
{

namespace
{
    constexpr int numPreviewVoices = 8;
}

void PreviewPianoVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    const double frequencyHz = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    phase = 0.0;
    phaseDelta = juce::MathConstants<double>::twoPi * frequencyHz / getSampleRate();
    level = velocity;

    adsr.setSampleRate (getSampleRate());
    adsr.setParameters ({ 0.005f, 0.25f, 0.4f, 0.5f });
    adsr.noteOn();
}

void PreviewPianoVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
    }
}

void PreviewPianoVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! adsr.isActive())
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float envelope = adsr.getNextSample();
        const float sample = level * envelope * 0.2f *
            static_cast<float> (0.8 * std::sin (phase) + 0.2 * std::sin (2.0 * phase));

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample (ch, startSample + i, sample);

        phase += phaseDelta;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;

        if (! adsr.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

PreviewSynth::PreviewSynth()
{
    for (int i = 0; i < numPreviewVoices; ++i)
        synth.addVoice (new PreviewPianoVoice());
    synth.addSound (new PreviewPianoSound());
}

void PreviewSynth::prepare (double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

void PreviewSynth::renderNextBlock (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiMessages,
                                     int startSample, int numSamples)
{
    synth.renderNextBlock (buffer, midiMessages, startSample, numSamples);
}

} // namespace smartchord
