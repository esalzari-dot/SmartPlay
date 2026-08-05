#pragma once

#include "Palette.h"
#include "smartchord/ChordBankModule.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace smartchord::ui
{

// Riga di 8 pulsanti accordo, selezionabili singolarmente (SPEC.md sezione 9).
class ChordPadRow : public juce::Component
{
public:
    ChordPadRow();

    // Aggiorna le etichette dei pad e l'evidenziazione dello slot attivo a partire
    // dallo stato corrente di ChordBankModule.
    void refreshFrom (const ChordBankModule& bank, juce::Colour accentColour, juce::Colour accentDarkColour);

    std::function<void (int)> onChordSelected;

    void resized() override;

private:
    class ChordPad : public juce::Button
    {
    public:
        ChordPad() : juce::Button ({}) {}

        void paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

        juce::String rootLabel;
        juce::String qualityLabel;
        bool selected = false;
        juce::Colour accent, accentDark;
    };

    std::array<ChordPad, numChordBankSlots> pads;
};

} // namespace smartchord::ui
