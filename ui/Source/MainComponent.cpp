#include "MainComponent.h"
#include "ChordLabel.h"

namespace smartchord::ui
{

namespace
{
    // Accordi dimostrativi per gli 8 slot, cosi' l'harness UI mostra pad distinguibili
    // (SPEC.md non prescrive un banco di default: la scelta e' lasciata all'utente).
    const ChordDefinition demoChords[numChordBankSlots] = {
        { 0, ChordQuality::Maj,  0, 0 },  // C
        { 7, ChordQuality::Maj,  0, 0 },  // G
        { 9, ChordQuality::Min,  0, 0 },  // A min
        { 5, ChordQuality::Maj,  0, 0 },  // F
        { 2, ChordQuality::Min7, 0, 0 },  // D min7
        { 4, ChordQuality::Min,  0, 0 },  // E min
        { 11, ChordQuality::Dim, 0, 0 },  // B dim
        { 0, ChordQuality::Dom7, 0, 0 },  // C7
    };
}

MainComponent::MainComponent()
{
    patternLibrary = PatternLibrary::fromJsonFile (std::string (SMARTCHORD_DATA_DIR) + "/patterns.json");

    for (int slot = 0; slot < numChordBankSlots; ++slot)
        chordBank.setChord (slot, demoChords[static_cast<size_t> (slot)]);

    titleLabel.setText ("Smart Chord & Arpeggiator", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, Palette::text);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (familySwitcher);
    familySwitcher.setSelectedFamily (activeFamily);
    familySwitcher.onFamilyChanged = [this] (InstrumentFamily family)
    {
        activeFamily = family;
        refresh();
    };

    addAndMakeVisible (chordPadRow);
    chordPadRow.onChordSelected = [this] (int slot)
    {
        chordBank.setActiveSlot (slot);
        refresh();
    };

    addAndMakeVisible (autoplayGrid);
    autoplayGrid.onCellSelected = [this] (int chordSlot, int intensityLevel)
    {
        chordBank.setActiveSlot (chordSlot);
        gridState.setIntensity (activeFamily, chordSlot, intensityLevel);
        refresh();
    };

    addAndMakeVisible (patternReadout);

    setSize (820, 480);
    refresh();
}

void MainComponent::refresh()
{
    const auto accent = accentColourFor (activeFamily);
    const auto accentDark = accentDarkColourFor (activeFamily);

    familySwitcher.setSelectedFamily (activeFamily);
    chordPadRow.refreshFrom (chordBank, accent, accentDark);
    autoplayGrid.refreshFrom (gridState, activeFamily, chordBank.getActiveSlot(), accent);

    const int intensityLevel = gridState.getIntensity (activeFamily, chordBank.getActiveSlot());
    const auto* pattern = resolvePattern (patternLibrary, activeFamily, intensityLevel);

    const auto& activeChord = chordBank.getActiveChord();
    const auto chordLabel = noteNameFor (activeChord.rootSemitone) + " " + qualityAbbreviationFor (activeChord.quality);

    patternReadout.setContent (pattern, activeFamily, intensityLevel, chordLabel, accent);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    auto topBar = bounds.removeFromTop (48).reduced (20, 8);
    titleLabel.setBounds (topBar);

    bounds.removeFromTop (16);
    familySwitcher.setBounds (bounds.removeFromTop (44).reduced (20, 0));

    bounds.removeFromTop (18);
    chordPadRow.setBounds (bounds.removeFromTop (56).reduced (20, 0));

    bounds.removeFromTop (14);
    autoplayGrid.setBounds (bounds.removeFromTop (200).reduced (20, 0));

    bounds.removeFromTop (14);
    patternReadout.setBounds (bounds.removeFromTop (60).reduced (20, 0));
}

} // namespace smartchord::ui
