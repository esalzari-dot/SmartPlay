#include "ChordLabel.h"

namespace smartchord::ui
{

juce::String noteNameFor (int rootSemitone)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int wrapped = ((rootSemitone % 12) + 12) % 12;
    return names[wrapped];
}

juce::String qualityAbbreviationFor (ChordQuality quality)
{
    switch (quality)
    {
        case ChordQuality::Maj:    return "maj";
        case ChordQuality::Min:    return "min";
        case ChordQuality::Dim:    return "dim";
        case ChordQuality::Aug:    return "aug";
        case ChordQuality::Sus2:   return "sus2";
        case ChordQuality::Sus4:   return "sus4";
        case ChordQuality::Maj7:   return "maj7";
        case ChordQuality::Min7:   return "min7";
        case ChordQuality::Dom7:   return "7";
        case ChordQuality::Min7b5: return "m7b5";
        case ChordQuality::Dim7:   return "dim7";
        case ChordQuality::Add9:   return "add9";
        case ChordQuality::Six:    return "6";
        case ChordQuality::Nine:   return "9";
    }
    return {};
}

} // namespace smartchord::ui
