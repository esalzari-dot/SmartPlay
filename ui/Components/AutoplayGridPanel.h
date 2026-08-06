#pragma once

#include "AutoplayGridComponent.h"
#include "ChordPadRow.h"
#include "FamilySwitcher.h"
#include "PatternReadout.h"

#include "smartchord/AutoplayGridState.h"
#include "smartchord/ChordBankModule.h"
#include "smartchord/PatternLibrary.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace smartchord::ui
{

// Compone lo switcher famiglia, la riga di 8 pad accordo e la griglia Autoplay 8x4
// (SPEC.md sezione 9). Non possiede lo stato: opera per riferimento su ChordBankModule
// e AutoplayGridState fornite dal chiamante, cosi' lo stesso pannello puo' essere
// riusato sia dall'harness standalone (ui/Source/Main.cpp) sia dal vero plugin
// (PluginEditor, dove lo stato e' posseduto da AudioProcessor).
class AutoplayGridPanel : public juce::Component
{
public:
    AutoplayGridPanel (ChordBankModule& chordBank,
                        AutoplayGridState& gridState,
                        const PatternLibrary& patternLibrary,
                        InstrumentFamily initialFamily = InstrumentFamily::Guitar);

    InstrumentFamily getActiveFamily() const noexcept { return activeFamily; }

    // Da chiamare quando lo stato esterno cambia per motivi non originati da questo
    // pannello (es. host che ripristina uno stato salvato).
    void refresh();

    // Chiamata dopo ogni interazione che modifica chordBank/gridState/activeFamily
    // (incluso il refresh() iniziale). Il chiamante che condivide questo stato con un
    // thread audio (es. PluginEditor) puo' usarla per ripropagare la modifica in modo
    // thread-safe verso AudioProcessor.
    std::function<void()> onStateChanged;

    // Interruttore "suona a trasporto fermo": ha senso solo dentro il plugin, dove c'e'
    // un trasporto dell'host, quindi resta nascosto finche' non lo si abilita.
    void setFreeRunControlVisible (bool shouldBeVisible);
    void setFreeRun (bool shouldFreeRun);
    std::function<void (bool)> onFreeRunChanged;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ChordBankModule& chordBank;
    AutoplayGridState& gridState;
    const PatternLibrary& patternLibrary;
    InstrumentFamily activeFamily;

    juce::Label titleLabel;
    juce::ToggleButton freeRunButton { "Suona a trasporto fermo" };
    FamilySwitcher familySwitcher;
    ChordPadRow chordPadRow;
    AutoplayGridComponent autoplayGrid;
    PatternReadout patternReadout;
};

} // namespace smartchord::ui
