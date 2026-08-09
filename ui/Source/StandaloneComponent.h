#pragma once

#include "AutoplayGridPanel.h"
#include "KeyboardChordShortcuts.h"

#include "smartchord/AutoplayGridState.h"
#include "smartchord/ChordBankModule.h"
#include "smartchord/PatternLibrary.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace smartchord::ui
{

// Possiede lo stato core (ChordBankModule, AutoplayGridState, PatternLibrary) per
// l'harness di sviluppo standalone e lo espone ad AutoplayGridPanel. Nel plugin vero
// (PluginProcessor/PluginEditor) questo stesso stato e' posseduto dall'AudioProcessor,
// cosi' da persistere ed essere automatizzabile dall'host (SPEC.md sezione 8).
class StandaloneComponent : public juce::Component
{
public:
    StandaloneComponent();

    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    PatternLibrary patternLibrary;
    ChordBankModule chordBank;
    AutoplayGridState gridState;
    AutoplayGridPanel panel;
};

} // namespace smartchord::ui
