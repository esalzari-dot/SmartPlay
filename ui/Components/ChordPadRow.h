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

    // Click sinistro: rende attivo lo slot.
    std::function<void (int)> onChordSelected;

    // Click destro: l'utente ha modificato l'accordo contenuto nello slot.
    std::function<void (int, const ChordDefinition&)> onChordEdited;

    void resized() override;

private:
    class ChordPad : public juce::Button
    {
    public:
        ChordPad() : juce::Button ({}) {}

        void paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;
        void mouseDown (const juce::MouseEvent& event) override;

        juce::String rootLabel;
        juce::String qualityLabel;
        int slotIndex = 0;
        bool selected = false;
        juce::Colour accent, accentDark;

        ChordDefinition chord;
        std::function<void()> onEditRequested;
    };

    void showEditMenuFor (int slot);

    std::array<ChordPad, numChordBankSlots> pads;
};

} // namespace smartchord::ui
