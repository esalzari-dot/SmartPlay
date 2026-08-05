#pragma once

#include "Palette.h"
#include "smartchord/AutoplayGridState.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace smartchord::ui
{

// Griglia Autoplay 8x4 (SPEC.md sezioni 5.1 e 9): 8 colonne (accordo) x 4 righe
// (intensita', dal basso=semplice all'alto=complesso). Il click su una cella
// seleziona sia l'accordo (colonna) sia l'intensita' (riga) contemporaneamente.
class AutoplayGridComponent : public juce::Component
{
public:
    AutoplayGridComponent();

    // Aggiorna l'evidenziazione delle celle a partire dallo stato corrente.
    void refreshFrom (const AutoplayGridState& gridState, InstrumentFamily family,
                       int activeChordSlot, juce::Colour accentColour);

    // Chiamato con (chordSlot, intensityLevel) quando l'utente clicca una cella.
    std::function<void (int, int)> onCellSelected;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    class GridCell : public juce::Button
    {
    public:
        GridCell() : juce::Button ({}) {}
        void paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

        bool isActiveColumn = false;
        bool isSelected = false;
        juce::Colour accent;
    };

    // cells[row][col]; row 0 = intensita' massima (in alto), row 3 = minima (in basso).
    std::array<std::array<GridCell, numChordSlots>, 4> cells;
};

} // namespace smartchord::ui
