#pragma once

#include "Palette.h"
#include "smartchord/PatternLibrary.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace smartchord::ui
{

// Readout in basso: nome del pattern risolto, famiglia, intensita' corrente,
// accordo attivo e routing MIDI verso il VST esterno (SPEC.md sezione 9).
class PatternReadout : public juce::Component
{
public:
    PatternReadout();

    void setContent (const PatternDefinition* pattern, InstrumentFamily family,
                      int intensityLevel, const juce::String& chordLabel, juce::Colour accentColour);

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    juce::Label nameLabel, subLabel, routeLabel;
    juce::Colour routeAccent = Palette::guitar;
};

} // namespace smartchord::ui
