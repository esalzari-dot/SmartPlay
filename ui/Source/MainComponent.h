#pragma once

#include "AutoplayGridComponent.h"
#include "ChordPadRow.h"
#include "FamilySwitcher.h"
#include "PatternReadout.h"

#include "smartchord/AutoplayGridState.h"
#include "smartchord/ChordBankModule.h"
#include "smartchord/PatternLibrary.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace smartchord::ui
{

// Compone lo switcher famiglia, la riga di 8 pad accordo e la griglia Autoplay 8x4
// (SPEC.md sezione 9), legandoli allo stato core (ChordBankModule, AutoplayGridState,
// PatternLibrary). E' un harness JUCE standalone di test/anteprima: l'integrazione nel
// vero plugin VST3/AU (AudioProcessorEditor) e' compito dello step 8.
class MainComponent : public juce::Component
{
public:
    MainComponent();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void refresh();

    PatternLibrary patternLibrary;
    ChordBankModule chordBank;
    AutoplayGridState gridState;
    InstrumentFamily activeFamily = InstrumentFamily::Guitar;

    juce::Label titleLabel;
    FamilySwitcher familySwitcher;
    ChordPadRow chordPadRow;
    AutoplayGridComponent autoplayGrid;
    PatternReadout patternReadout;
};

} // namespace smartchord::ui
