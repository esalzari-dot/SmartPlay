#include "AutoplayGridPanel.h"
#include "ChordLabel.h"

namespace smartchord::ui
{

AutoplayGridPanel::AutoplayGridPanel (ChordBankModule& chordBankIn,
                                       AutoplayGridState& gridStateIn,
                                       const PatternLibrary& patternLibraryIn,
                                       InstrumentFamily initialFamily)
    : chordBank (chordBankIn), gridState (gridStateIn), patternLibrary (patternLibraryIn),
      activeFamily (initialFamily)
{
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

    chordPadRow.onChordEdited = [this] (int slot, const ChordDefinition& chord)
    {
        chordBank.setChord (slot, chord);
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

    freeRunButton.setColour (juce::ToggleButton::textColourId, Palette::textMuted);
    freeRunButton.setColour (juce::ToggleButton::tickColourId, Palette::text);
    freeRunButton.setVisible (false);
    freeRunButton.onClick = [this]
    {
        if (onFreeRunChanged != nullptr)
            onFreeRunChanged (freeRunButton.getToggleState());
    };
    addChildComponent (freeRunButton);

    setSize (820, 480);
    refresh();
}

void AutoplayGridPanel::setFreeRunControlVisible (bool shouldBeVisible)
{
    freeRunButton.setVisible (shouldBeVisible);
}

void AutoplayGridPanel::setFreeRun (bool shouldFreeRun)
{
    freeRunButton.setToggleState (shouldFreeRun, juce::dontSendNotification);
}

void AutoplayGridPanel::refresh()
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

    if (onStateChanged != nullptr)
        onStateChanged();
}

void AutoplayGridPanel::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);
}

void AutoplayGridPanel::resized()
{
    auto bounds = getLocalBounds();

    auto topBar = bounds.removeFromTop (48).reduced (20, 8);
    freeRunButton.setBounds (topBar.removeFromRight (220));
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
