#include "AutoplayGridPanel.h"
#include "ChordLabel.h"

namespace smartchord::ui
{

namespace
{
    // L'id 0 e' riservato da JUCE per "nessuna voce selezionata", quindi gli id partono
    // da 1 e l'enum si ricava sottraendo 1.
    constexpr int rateIdBase = 1;

    int comboIdFor (PatternRate rate) { return static_cast<int> (rate) + rateIdBase; }

    constexpr int progressionIdBase = 1;
    constexpr int keyIdBase = 1;

    // I ComboBox di JUCE nascono scuri: senza questo riallineamento stonerebbero sul
    // pannello chiaro del resto della UI.
    void styleComboBox (juce::ComboBox& box)
    {
        box.setColour (juce::ComboBox::backgroundColourId, Palette::panel);
        box.setColour (juce::ComboBox::textColourId, Palette::text);
        box.setColour (juce::ComboBox::outlineColourId, Palette::panelEdge);
        box.setColour (juce::ComboBox::arrowColourId, Palette::textMuted);
    }
}

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

    voiceLeadingButton.setColour (juce::ToggleButton::textColourId, Palette::textMuted);
    voiceLeadingButton.setColour (juce::ToggleButton::tickColourId, Palette::text);
    voiceLeadingButton.setVisible (false);
    voiceLeadingButton.onClick = [this]
    {
        if (onVoiceLeadingChanged != nullptr)
            onVoiceLeadingChanged (voiceLeadingButton.getToggleState());
    };
    addChildComponent (voiceLeadingButton);

    rateLabel.setText ("Rate", juce::dontSendNotification);
    rateLabel.setFont (juce::FontOptions (12.0f));
    rateLabel.setColour (juce::Label::textColourId, Palette::textMuted);
    rateLabel.setJustificationType (juce::Justification::centredRight);
    rateLabel.setVisible (false);
    addChildComponent (rateLabel);

    rateBox.addItem ("1/2x", comboIdFor (PatternRate::Half));
    rateBox.addItem ("1x", comboIdFor (PatternRate::Normal));
    rateBox.addItem ("Terzine", comboIdFor (PatternRate::Triplet));
    rateBox.addItem ("2x", comboIdFor (PatternRate::Double));
    rateBox.setSelectedId (comboIdFor (PatternRate::Normal), juce::dontSendNotification);
    styleComboBox (rateBox);
    rateBox.setVisible (false);
    rateBox.onChange = [this]
    {
        if (onPatternRateChanged != nullptr && rateBox.getSelectedId() >= rateIdBase)
            onPatternRateChanged (static_cast<PatternRate> (rateBox.getSelectedId() - rateIdBase));
    };
    addChildComponent (rateBox);

    chordFromKeyboardButton.setColour (juce::ToggleButton::textColourId, Palette::textMuted);
    chordFromKeyboardButton.setColour (juce::ToggleButton::tickColourId, Palette::text);
    chordFromKeyboardButton.setVisible (false);
    chordFromKeyboardButton.onClick = [this]
    {
        if (onChordFromKeyboardChanged != nullptr)
            onChordFromKeyboardChanged (chordFromKeyboardButton.getToggleState());
    };
    addChildComponent (chordFromKeyboardButton);

    progressionLabel.setText ("Progressione", juce::dontSendNotification);
    progressionLabel.setFont (juce::FontOptions (12.0f));
    progressionLabel.setColour (juce::Label::textColourId, Palette::textMuted);
    addAndMakeVisible (progressionLabel);

    progressionBox.addItem ("-", progressionIdBase);
    {
        int id = progressionIdBase + 1;
        for (const auto& progression : getChordProgressions())
            progressionBox.addItem (progression.displayName, id++);
    }
    progressionBox.setSelectedId (progressionIdBase, juce::dontSendNotification);
    styleComboBox (progressionBox);
    progressionBox.onChange = [this] { applySelectedProgression(); };
    addAndMakeVisible (progressionBox);

    for (int semitone = 0; semitone < 12; ++semitone)
        keyBox.addItem (noteNameFor (semitone), keyIdBase + semitone);
    keyBox.setSelectedId (keyIdBase, juce::dontSendNotification);
    styleComboBox (keyBox);
    keyBox.onChange = [this] { applySelectedProgression(); };
    addAndMakeVisible (keyBox);

    // Swing/gate/ottava globali (SPEC.md sezione 8): automatizzabili solo dentro il
    // plugin, quindi nascosti di default come rate/freeRun/voiceLeading/keyboard.
    for (auto* label : { &swingLabel, &gateLabel, &octaveLabel })
    {
        label->setFont (juce::FontOptions (12.0f));
        label->setColour (juce::Label::textColourId, Palette::textMuted);
        label->setJustificationType (juce::Justification::centredRight);
        label->setVisible (false);
        addChildComponent (label);
    }

    swingSlider.setRange (0.0, 1.0);
    swingSlider.setTextValueSuffix ("%");
    swingSlider.setColour (juce::Slider::textBoxTextColourId, Palette::text);
    swingSlider.setColour (juce::Slider::textBoxOutlineColourId, Palette::panelEdge);
    swingSlider.textFromValueFunction = [] (double value) { return juce::String (juce::roundToInt (value * 100.0)); };
    swingSlider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue() / 100.0; };
    swingSlider.setValue (0.0, juce::dontSendNotification);
    swingSlider.updateText(); // setValue() salta updateText() se il valore non cambia (vedi setGlobalSwing)
    swingSlider.setVisible (false);
    swingSlider.onValueChange = [this]
    {
        if (onGlobalSwingChanged != nullptr)
            onGlobalSwingChanged (static_cast<float> (swingSlider.getValue()));
    };
    addChildComponent (swingSlider);

    gateSlider.setRange (0.25, 1.5);
    gateSlider.setTextValueSuffix ("%");
    gateSlider.setColour (juce::Slider::textBoxTextColourId, Palette::text);
    gateSlider.setColour (juce::Slider::textBoxOutlineColourId, Palette::panelEdge);
    gateSlider.textFromValueFunction = [] (double value) { return juce::String (juce::roundToInt (value * 100.0)); };
    gateSlider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue() / 100.0; };
    gateSlider.setValue (1.0, juce::dontSendNotification);
    gateSlider.updateText();
    gateSlider.setVisible (false);
    gateSlider.onValueChange = [this]
    {
        if (onGlobalGateLengthChanged != nullptr)
            onGlobalGateLengthChanged (static_cast<float> (gateSlider.getValue()));
    };
    addChildComponent (gateSlider);

    for (int octave = -2; octave <= 2; ++octave)
        octaveBox.addItem (octave == 0 ? "0" : juce::String::formatted ("%+d", octave), octave + 3);
    octaveBox.setSelectedId (3, juce::dontSendNotification);
    styleComboBox (octaveBox);
    octaveBox.setVisible (false);
    octaveBox.onChange = [this]
    {
        if (onOctaveRangeChanged != nullptr)
            onOctaveRangeChanged (octaveBox.getSelectedId() - 3);
    };
    addChildComponent (octaveBox);

    setSize (820, 556);
    refresh();
}

void AutoplayGridPanel::setFreeRunControlVisible (bool shouldBeVisible)
{
    freeRunButton.setVisible (shouldBeVisible);
    voiceLeadingButton.setVisible (shouldBeVisible);
    rateLabel.setVisible (shouldBeVisible);
    rateBox.setVisible (shouldBeVisible);
    chordFromKeyboardButton.setVisible (shouldBeVisible);
    setGlobalControlsVisible (shouldBeVisible);
}

void AutoplayGridPanel::setGlobalControlsVisible (bool shouldBeVisible)
{
    swingLabel.setVisible (shouldBeVisible);
    swingSlider.setVisible (shouldBeVisible);
    gateLabel.setVisible (shouldBeVisible);
    gateSlider.setVisible (shouldBeVisible);
    octaveLabel.setVisible (shouldBeVisible);
    octaveBox.setVisible (shouldBeVisible);
}

void AutoplayGridPanel::setFreeRun (bool shouldFreeRun)
{
    freeRunButton.setToggleState (shouldFreeRun, juce::dontSendNotification);
}

void AutoplayGridPanel::setVoiceLeading (bool shouldLead)
{
    voiceLeadingButton.setToggleState (shouldLead, juce::dontSendNotification);
}

void AutoplayGridPanel::setPatternRate (PatternRate rate)
{
    rateBox.setSelectedId (comboIdFor (rate), juce::dontSendNotification);
}

void AutoplayGridPanel::setChordFromKeyboard (bool shouldRecognize)
{
    chordFromKeyboardButton.setToggleState (shouldRecognize, juce::dontSendNotification);
}

void AutoplayGridPanel::setGlobalSwing (float amount01)
{
    // setValue() non aggiorna la casella di testo se il valore non cambia (es. resta a
    // 0): updateText() esplicito copre anche quel caso.
    swingSlider.setValue (amount01, juce::dontSendNotification);
    swingSlider.updateText();
}

void AutoplayGridPanel::setGlobalGateLength (float multiplier)
{
    gateSlider.setValue (multiplier, juce::dontSendNotification);
    gateSlider.updateText();
}

void AutoplayGridPanel::setOctaveRange (int octaves)
{
    octaveBox.setSelectedId (octaves + 3, juce::dontSendNotification);
}

void AutoplayGridPanel::setPlayheadPosition (float normalized)
{
    patternReadout.setPlayheadPosition (normalized);
}

void AutoplayGridPanel::setActiveFamily (InstrumentFamily family)
{
    activeFamily = family;
    familySwitcher.setSelectedFamily (activeFamily);
}

void AutoplayGridPanel::applySelectedProgression()
{
    // La voce "-" non e' una progressione: e' lo stato in cui il banco resta com'e'.
    const int index = progressionBox.getSelectedId() - progressionIdBase - 1;
    if (index < 0)
        return;

    const auto& all = getChordProgressions();
    if (index >= static_cast<int> (all.size()))
        return;

    const int tonic = keyBox.getSelectedId() - keyIdBase;
    const auto activeSlot = chordBank.getActiveSlot();

    chordBank = buildChordBank (all[static_cast<size_t> (index)], tonic);
    chordBank.setActiveSlot (activeSlot);

    refresh();
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
    rateBox.setBounds (topBar.removeFromRight (90).reduced (0, 3));
    rateLabel.setBounds (topBar.removeFromRight (40));
    freeRunButton.setBounds (topBar.removeFromRight (185));
    voiceLeadingButton.setBounds (topBar.removeFromRight (120));
    chordFromKeyboardButton.setBounds (topBar.removeFromRight (155));
    titleLabel.setBounds (topBar);

    bounds.removeFromTop (16);
    familySwitcher.setBounds (bounds.removeFromTop (44).reduced (20, 0));

    bounds.removeFromTop (12);
    auto progressionBar = bounds.removeFromTop (26).reduced (20, 0);
    progressionLabel.setBounds (progressionBar.removeFromLeft (86));
    progressionBox.setBounds (progressionBar.removeFromLeft (230));
    progressionBar.removeFromLeft (8);
    keyBox.setBounds (progressionBar.removeFromLeft (70));

    bounds.removeFromTop (10);
    auto globalControlsBar = bounds.removeFromTop (26).reduced (20, 0);
    swingLabel.setBounds (globalControlsBar.removeFromLeft (46));
    swingSlider.setBounds (globalControlsBar.removeFromLeft (170));
    globalControlsBar.removeFromLeft (16);
    gateLabel.setBounds (globalControlsBar.removeFromLeft (40));
    gateSlider.setBounds (globalControlsBar.removeFromLeft (170));
    globalControlsBar.removeFromLeft (16);
    octaveLabel.setBounds (globalControlsBar.removeFromLeft (48));
    octaveBox.setBounds (globalControlsBar.removeFromLeft (64));

    bounds.removeFromTop (10);
    chordPadRow.setBounds (bounds.removeFromTop (56).reduced (20, 0));

    bounds.removeFromTop (12);
    autoplayGrid.setBounds (bounds.removeFromTop (190).reduced (20, 0));

    bounds.removeFromTop (14);
    patternReadout.setBounds (bounds.removeFromTop (60).reduced (20, 0));
}

} // namespace smartchord::ui
