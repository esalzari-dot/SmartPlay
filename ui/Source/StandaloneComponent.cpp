#include "StandaloneComponent.h"

namespace smartchord::ui
{

namespace
{
    // Accordi dimostrativi per gli 8 slot, cosi' l'harness UI mostra pad distinguibili
    // (SPEC.md non prescrive un banco di default: la scelta e' lasciata all'utente).
    ChordBankModule makeDemoChordBank()
    {
        static const ChordDefinition demoChords[numChordBankSlots] = {
            { 0, ChordQuality::Maj,  0, 0 },  // C
            { 7, ChordQuality::Maj,  0, 0 },  // G
            { 9, ChordQuality::Min,  0, 0 },  // A min
            { 5, ChordQuality::Maj,  0, 0 },  // F
            { 2, ChordQuality::Min7, 0, 0 },  // D min7
            { 4, ChordQuality::Min,  0, 0 },  // E min
            { 11, ChordQuality::Dim, 0, 0 },  // B dim
            { 0, ChordQuality::Dom7, 0, 0 },  // C7
            { 7, ChordQuality::Dom7, 0, 0 },  // G7 (nono slot, tastierino: tasto 9)
        };

        ChordBankModule bank;
        for (int slot = 0; slot < numChordBankSlots; ++slot)
            bank.setChord (slot, demoChords[static_cast<size_t> (slot)]);
        return bank;
    }
}

StandaloneComponent::StandaloneComponent()
    : patternLibrary (PatternLibrary::fromJsonFile (std::string (SMARTCHORD_DATA_DIR) + "/patterns.json")),
      chordBank (makeDemoChordBank()),
      panel (chordBank, gridState, patternLibrary, InstrumentFamily::Guitar)
{
    addAndMakeVisible (panel);
    setSize (panel.getWidth(), panel.getHeight());

    setWantsKeyboardFocus (true);
    grabKeyboardFocus();
}

void StandaloneComponent::resized()
{
    panel.setBounds (getLocalBounds());
}

bool StandaloneComponent::keyPressed (const juce::KeyPress& key)
{
    const int slot = chordSlotForKeyPress (key, numChordBankSlots);
    if (slot < 0)
        return false;

    chordBank.setActiveSlot (slot);
    panel.refresh();
    return true;
}

} // namespace smartchord::ui
