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

    // Posizione normalizzata (0-1) nel loop in esecuzione: disegna il playhead in fondo
    // al riquadro (SPEC.md sezione 9). Fuori da [0,1] (es. -1 quando non sta suonando)
    // nasconde l'indicatore invece di clampare a un bordo che sarebbe fuorviante.
    void setPlayheadPosition (float normalized);

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    juce::Label nameLabel, subLabel, routeLabel;
    juce::Colour routeAccent = Palette::guitar;
    float playheadPosition = -1.0f;
};

} // namespace smartchord::ui
