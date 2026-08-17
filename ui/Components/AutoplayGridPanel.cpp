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

    void styleComboBox (juce::ComboBox& box)
    {
        box.setColour (juce::ComboBox::backgroundColourId, Palette::panelRaised);
        box.setColour (juce::ComboBox::textColourId, Palette::text);
        box.setColour (juce::ComboBox::outlineColourId, Palette::seam);
        box.setColour (juce::ComboBox::arrowColourId, Palette::textDim);
    }

    void styleToggle (juce::ToggleButton& toggle)
    {
        toggle.setColour (juce::ToggleButton::textColourId, Palette::textMuted);
        toggle.setColour (juce::ToggleButton::tickColourId, Palette::text);
        toggle.setColour (juce::ToggleButton::tickDisabledColourId, Palette::textDim);
    }

    // Le due opzioni del toggle Autoplay/Play (SPEC.md sezione 5.5): stessa lettura "in
    // rilievo quando selezionato" dei pad/segmenti del resto del pannello.
    void styleModeButton (juce::TextButton& button, bool selected)
    {
        button.setColour (juce::TextButton::buttonColourId, selected ? Palette::panelRaisedHi : Palette::panelInset);
        button.setColour (juce::TextButton::textColourOffId, selected ? Palette::text : Palette::textDim);
    }

    // Etichetta "eyebrow" in stile pannello hardware: mono, tracciata, minuscola nel
    // peso visivo ma maiuscola nel testo - la stessa lettura di un'etichetta serigrafata
    // accanto a una sezione di manopole.
    void styleSectionLabel (juce::Label& label, const juce::String& text)
    {
        label.setText (text.toUpperCase(), juce::dontSendNotification);
        auto font = monoFont (10.0f, juce::Font::bold);
        font.setExtraKerningFactor (0.12f);
        label.setFont (font);
        label.setColour (juce::Label::textColourId, Palette::textDim);
    }
}

AutoplayGridPanel::AutoplayGridPanel (ChordBankModule& chordBankIn,
                                       AutoplayGridState& gridStateIn,
                                       const PatternLibrary& patternLibraryIn,
                                       InstrumentFamily initialFamily)
    : chordBank (chordBankIn), gridState (gridStateIn), patternLibrary (patternLibraryIn),
      activeFamily (initialFamily)
{
    titleLabel.setText ("SmartPlay", juce::dontSendNotification);
    {
        auto titleFont = monoFont (13.0f, juce::Font::bold);
        titleFont.setExtraKerningFactor (0.06f);
        titleLabel.setFont (titleFont);
    }
    titleLabel.setColour (juce::Label::textColourId, Palette::text);
    addAndMakeVisible (titleLabel);

    styleSectionLabel (instrumentSectionLabel, "Instrument");
    addAndMakeVisible (instrumentSectionLabel);

    addAndMakeVisible (familySwitcher);
    familySwitcher.setSelectedFamily (activeFamily);
    familySwitcher.onFamilyChanged = [this] (InstrumentFamily family)
    {
        activeFamily = family;
        refresh();
    };

    styleSectionLabel (chordBankSectionLabel, "Chord Bank");
    addAndMakeVisible (chordBankSectionLabel);

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

    styleSectionLabel (autoplaySectionLabel, "Autoplay");
    addAndMakeVisible (autoplaySectionLabel);

    addAndMakeVisible (autoplayGrid);
    autoplayGrid.onCellSelected = [this] (int chordSlot, int intensityLevel)
    {
        chordBank.setActiveSlot (chordSlot);
        gridState.setIntensity (activeFamily, chordSlot, intensityLevel);
        refresh();
    };

    autoplayModeButton.onClick = [this]
    {
        if (! playModeActive)
            return;

        setPlayModeActive (false);
        if (onPlayModeChanged != nullptr)
            onPlayModeChanged (false);
    };
    addAndMakeVisible (autoplayModeButton);

    playModeButton.onClick = [this]
    {
        if (playModeActive)
            return;

        setPlayModeActive (true);
        if (onPlayModeChanged != nullptr)
            onPlayModeChanged (true);
    };
    addAndMakeVisible (playModeButton);
    updateModeButtons();

    for (int i = 0; i < numChordBankSlots; ++i)
    {
        auto& row = playStripRows[static_cast<size_t> (i)];
        row.setSlotIndex (i);

        row.onNotchGesture = [this] (int slot, StripGesturePhase phase, float position)
        {
            if (onPlayStripGesture != nullptr)
                onPlayStripGesture (slot, phase, position);
        };

        row.onChordGesture = [this] (int slot, bool down)
        {
            if (onPlayStripChordGesture != nullptr)
                onPlayStripChordGesture (slot, down);
        };

        addChildComponent (row); // partono nascoste: updateModeButtons() sopra decide
    }

    for (auto* caption : { &simpleCaptionLabel, &complexCaptionLabel })
    {
        caption->setFont (monoFont (9.0f));
        caption->setColour (juce::Label::textColourId, Palette::textDim);
    }
    simpleCaptionLabel.setText ("SEMPLICE", juce::dontSendNotification);
    complexCaptionLabel.setText ("COMPLESSO", juce::dontSendNotification);
    complexCaptionLabel.setJustificationType (juce::Justification::right);
    addAndMakeVisible (simpleCaptionLabel);
    addAndMakeVisible (complexCaptionLabel);

    addAndMakeVisible (patternReadout);

    styleToggle (freeRunButton);
    freeRunButton.setVisible (false);
    freeRunButton.onClick = [this]
    {
        if (onFreeRunChanged != nullptr)
            onFreeRunChanged (freeRunButton.getToggleState());
    };
    addChildComponent (freeRunButton);

    styleToggle (voiceLeadingButton);
    voiceLeadingButton.setVisible (false);
    voiceLeadingButton.onClick = [this]
    {
        if (onVoiceLeadingChanged != nullptr)
            onVoiceLeadingChanged (voiceLeadingButton.getToggleState());
    };
    addChildComponent (voiceLeadingButton);

    // Colore d'allarme distinto dagli altri controlli (che sono tutti preferenze neutre):
    // Stop e' un panico, deve leggersi diverso a colpo d'occhio. Pulsante normale, non un
    // interruttore: e' un'azione istantanea (MIDI panic), non uno stato da ricordare.
    stopButton.setColour (juce::TextButton::buttonColourId, Palette::bass.withAlpha (0.18f));
    stopButton.setColour (juce::TextButton::textColourOffId, Palette::bass);
    stopButton.setVisible (false);
    stopButton.onClick = [this]
    {
        if (onStopRequested != nullptr)
            onStopRequested();
    };
    addChildComponent (stopButton);

    rateLabel.setText ("Rate", juce::dontSendNotification);
    rateLabel.setFont (monoFont (11.0f));
    rateLabel.setColour (juce::Label::textColourId, Palette::textDim);
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

    styleToggle (chordFromKeyboardButton);
    chordFromKeyboardButton.setVisible (false);
    chordFromKeyboardButton.onClick = [this]
    {
        if (onChordFromKeyboardChanged != nullptr)
            onChordFromKeyboardChanged (chordFromKeyboardButton.getToggleState());
    };
    addChildComponent (chordFromKeyboardButton);

    styleSectionLabel (bankSectionLabel, "Bank");
    addAndMakeVisible (bankSectionLabel);

    progressionLabel.setText ("Progressione", juce::dontSendNotification);
    progressionLabel.setFont (monoFont (11.0f));
    progressionLabel.setColour (juce::Label::textColourId, Palette::textDim);
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

    presetLabel.setFont (monoFont (11.0f));
    presetLabel.setColour (juce::Label::textColourId, Palette::textDim);
    addAndMakeVisible (presetLabel);

    styleComboBox (presetBox);
    presetBox.setTextWhenNoChoicesAvailable ("(nessuno)");
    presetBox.setTextWhenNothingSelected ("(nessuno)");
    presetBox.onChange = [this]
    {
        const auto name = presetBox.getText();
        if (name.isNotEmpty() && onLoadPresetRequested != nullptr)
            onLoadPresetRequested (name);
    };
    addAndMakeVisible (presetBox);

    savePresetButton.setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
    savePresetButton.setColour (juce::TextButton::textColourOffId, Palette::textMuted);
    savePresetButton.onClick = [this]
    {
        // deleteWhenDismissed=true: la AlertWindow si distrugge da sola alla chiusura,
        // non va cancellata a mano nella callback.
        auto* dialog = new juce::AlertWindow ("Salva preset", "Nome del preset:", juce::AlertWindow::NoIcon);
        dialog->addTextEditor ("name", presetBox.getText());
        dialog->addButton ("Salva", 1, juce::KeyPress (juce::KeyPress::returnKey));
        dialog->addButton ("Annulla", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        dialog->enterModalState (true, juce::ModalCallbackFunction::create ([this, dialog] (int result)
        {
            if (result != 1)
                return;

            const auto name = dialog->getTextEditorContents ("name").trim();
            if (name.isNotEmpty() && onSavePresetRequested != nullptr)
                onSavePresetRequested (name);
        }), true);
    };
    addAndMakeVisible (savePresetButton);

    deletePresetButton.setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
    deletePresetButton.setColour (juce::TextButton::textColourOffId, Palette::textMuted);
    deletePresetButton.onClick = [this]
    {
        const auto name = presetBox.getText();
        if (name.isNotEmpty() && onDeletePresetRequested != nullptr)
            onDeletePresetRequested (name);
    };
    addAndMakeVisible (deletePresetButton);

    styleSectionLabel (performanceSectionLabel, "Performance");
    performanceSectionLabel.setVisible (false);
    addChildComponent (performanceSectionLabel);

    // Swing/gate/ottava globali (SPEC.md sezione 8): automatizzabili solo dentro il
    // plugin, quindi nascosti di default come rate/freeRun/voiceLeading/keyboard.
    for (auto* label : { &swingLabel, &gateLabel, &octaveLabel })
    {
        label->setFont (monoFont (11.0f));
        label->setColour (juce::Label::textColourId, Palette::textDim);
        label->setJustificationType (juce::Justification::centredRight);
        label->setVisible (false);
        addChildComponent (label);
    }

    auto styleSlider = [] (juce::Slider& slider)
    {
        slider.setColour (juce::Slider::backgroundColourId, Palette::panelInset);
        slider.setColour (juce::Slider::trackColourId, Palette::panelRaisedHi);
        slider.setColour (juce::Slider::thumbColourId, Palette::text);
        slider.setColour (juce::Slider::textBoxTextColourId, Palette::text);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, Palette::panelInset);
        slider.setColour (juce::Slider::textBoxOutlineColourId, Palette::seam);
    };

    swingSlider.setRange (0.0, 1.0);
    swingSlider.setTextValueSuffix ("%");
    styleSlider (swingSlider);
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
    styleSlider (gateSlider);
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

    styleToggle (quantizeSwitchButton);
    quantizeSwitchButton.setVisible (false);
    quantizeSwitchButton.onClick = [this]
    {
        if (onQuantizeChordSwitchChanged != nullptr)
            onQuantizeChordSwitchChanged (quantizeSwitchButton.getToggleState());
    };
    addChildComponent (quantizeSwitchButton);

    styleToggle (humanizeButton);
    humanizeButton.setToggleState (true, juce::dontSendNotification);
    humanizeButton.setVisible (false);
    humanizeButton.onClick = [this]
    {
        if (onHumanizeEnabledChanged != nullptr)
            onHumanizeEnabledChanged (humanizeButton.getToggleState());
    };
    addChildComponent (humanizeButton);

    setSize (820, 720);
    refresh();
}

void AutoplayGridPanel::setFreeRunControlVisible (bool shouldBeVisible)
{
    freeRunButton.setVisible (shouldBeVisible);
    voiceLeadingButton.setVisible (shouldBeVisible);
    rateLabel.setVisible (shouldBeVisible);
    rateBox.setVisible (shouldBeVisible);
    chordFromKeyboardButton.setVisible (shouldBeVisible);
    stopButton.setVisible (shouldBeVisible);
    setGlobalControlsVisible (shouldBeVisible);
}

void AutoplayGridPanel::setGlobalControlsVisible (bool shouldBeVisible)
{
    performanceSectionLabel.setVisible (shouldBeVisible);
    swingLabel.setVisible (shouldBeVisible);
    swingSlider.setVisible (shouldBeVisible);
    gateLabel.setVisible (shouldBeVisible);
    gateSlider.setVisible (shouldBeVisible);
    octaveLabel.setVisible (shouldBeVisible);
    octaveBox.setVisible (shouldBeVisible);
    quantizeSwitchButton.setVisible (shouldBeVisible);
    humanizeButton.setVisible (shouldBeVisible);
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

void AutoplayGridPanel::setQuantizeChordSwitch (bool shouldQuantize)
{
    quantizeSwitchButton.setToggleState (shouldQuantize, juce::dontSendNotification);
}

void AutoplayGridPanel::setHumanizeEnabled (bool shouldHumanize)
{
    humanizeButton.setToggleState (shouldHumanize, juce::dontSendNotification);
}

void AutoplayGridPanel::setPlayheadPosition (float normalized)
{
    patternReadout.setPlayheadPosition (normalized);
}

void AutoplayGridPanel::setAvailablePresets (const juce::StringArray& names)
{
    const auto previouslySelected = presetBox.getText();

    presetBox.clear (juce::dontSendNotification);
    int id = 1;
    for (const auto& name : names)
        presetBox.addItem (name, id++);

    const auto index = names.indexOf (previouslySelected);
    if (index >= 0)
        presetBox.setSelectedItemIndex (index, juce::dontSendNotification);
}

void AutoplayGridPanel::setActiveFamily (InstrumentFamily family)
{
    activeFamily = family;
    familySwitcher.setSelectedFamily (activeFamily);
}

void AutoplayGridPanel::setPlayModeActive (bool shouldBeActive)
{
    playModeActive = shouldBeActive;
    updateModeButtons();
    refreshPlayStrips();
}

void AutoplayGridPanel::updateModeButtons()
{
    styleModeButton (autoplayModeButton, ! playModeActive);
    styleModeButton (playModeButton, playModeActive);

    autoplayGrid.setVisible (! playModeActive);
    simpleCaptionLabel.setVisible (! playModeActive);
    complexCaptionLabel.setVisible (! playModeActive);

    for (auto& row : playStripRows)
        row.setVisible (playModeActive);
}

void AutoplayGridPanel::refreshPlayStrips()
{
    // Tutti e 9 gli accordi restano suonabili contemporaneamente (SPEC.md sezione 5.5):
    // niente bisogno di riselezionare un pad sopra per cambiare su quale accordo si suona.
    const auto accent = accentColourFor (activeFamily);
    const int notchCount = notchCountForFamily (activeFamily);

    for (int i = 0; i < numChordBankSlots; ++i)
    {
        const auto& chord = chordBank.getChord (i);
        auto& row = playStripRows[static_cast<size_t> (i)];
        row.setChordLabel (noteNameFor (chord.rootSemitone), qualityAbbreviationFor (chord.quality));
        row.setNotchCount (notchCount);
        row.setAccentColour (accent);
    }
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

    refreshPlayStrips();

    if (onStateChanged != nullptr)
        onStateChanged();
}

void AutoplayGridPanel::paint (juce::Graphics& g)
{
    g.fillAll (Palette::panel);

    // Leggera grana verticale sul corpo del pannello, la stessa idea di
    // docs/mockup-v3-studio-panel.html: fa leggere il fondo come un materiale invece
    // che come un riempimento piatto, senza il costo di una texture vera.
    g.setColour (Palette::seam.withAlpha (0.25f));
    for (int y = 0; y < getHeight(); y += 3)
        g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
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
    topBar.removeFromRight (14);
    stopButton.setBounds (topBar.removeFromRight (60));
    titleLabel.setBounds (topBar);

    bounds.removeFromTop (16);
    instrumentSectionLabel.setBounds (bounds.removeFromTop (16).reduced (20, 0));
    bounds.removeFromTop (6);
    familySwitcher.setBounds (bounds.removeFromTop (44).reduced (20, 0));

    bounds.removeFromTop (14);
    bankSectionLabel.setBounds (bounds.removeFromTop (16).reduced (20, 0));
    bounds.removeFromTop (6);
    auto progressionBar = bounds.removeFromTop (26).reduced (20, 0);
    progressionLabel.setBounds (progressionBar.removeFromLeft (86));
    progressionBox.setBounds (progressionBar.removeFromLeft (230));
    progressionBar.removeFromLeft (8);
    keyBox.setBounds (progressionBar.removeFromLeft (70));

    progressionBar.removeFromLeft (20);
    presetLabel.setBounds (progressionBar.removeFromLeft (46));
    presetBox.setBounds (progressionBar.removeFromLeft (140));
    progressionBar.removeFromLeft (6);
    savePresetButton.setBounds (progressionBar.removeFromLeft (70));
    progressionBar.removeFromLeft (6);
    deletePresetButton.setBounds (progressionBar.removeFromLeft (70));

    bounds.removeFromTop (14);
    performanceSectionLabel.setBounds (bounds.removeFromTop (16).reduced (20, 0));
    bounds.removeFromTop (6);
    auto globalControlsBar = bounds.removeFromTop (26).reduced (20, 0);
    swingLabel.setBounds (globalControlsBar.removeFromLeft (46));
    swingSlider.setBounds (globalControlsBar.removeFromLeft (170));
    globalControlsBar.removeFromLeft (16);
    gateLabel.setBounds (globalControlsBar.removeFromLeft (40));
    gateSlider.setBounds (globalControlsBar.removeFromLeft (170));
    globalControlsBar.removeFromLeft (16);
    octaveLabel.setBounds (globalControlsBar.removeFromLeft (48));
    octaveBox.setBounds (globalControlsBar.removeFromLeft (64));

    bounds.removeFromTop (8);
    auto togglesBar = bounds.removeFromTop (26).reduced (20, 0);
    quantizeSwitchButton.setBounds (togglesBar.removeFromLeft (140));
    togglesBar.removeFromLeft (12);
    humanizeButton.setBounds (togglesBar.removeFromLeft (110));

    bounds.removeFromTop (14);
    chordBankSectionLabel.setBounds (bounds.removeFromTop (16).reduced (20, 0));
    bounds.removeFromTop (6);
    chordPadRow.setBounds (bounds.removeFromTop (56).reduced (20, 0));

    bounds.removeFromTop (14);
    auto autoplayLabelRow = bounds.removeFromTop (16).reduced (20, 0);
    playModeButton.setBounds (autoplayLabelRow.removeFromRight (50));
    autoplayLabelRow.removeFromRight (3);
    autoplayModeButton.setBounds (autoplayLabelRow.removeFromRight (76));
    autoplaySectionLabel.setBounds (autoplayLabelRow);
    bounds.removeFromTop (6);

    // Autoplay Grid e barra Play condividono la stessa area riservata (si escludono a
    // vicenda, mai visibili insieme), cosi' lo switch fra le due non richiede di
    // ridimensionare la finestra.
    auto autoplayArea = bounds.removeFromTop (204).reduced (20, 0);
    autoplayGrid.setBounds (autoplayArea);

    // 9 colonne affiancate, un piccolo gap fra l'una e l'altra (vedi commento sul membro
    // playStripRows in AutoplayGridPanel.h: tutte e 9 restano visibili e suonabili insieme).
    auto playStripArea = autoplayArea;
    const int gap = 4;
    const int columnWidth = (playStripArea.getWidth() - gap * (numChordBankSlots - 1)) / numChordBankSlots;
    for (int i = 0; i < numChordBankSlots; ++i)
    {
        auto& row = playStripRows[static_cast<size_t> (i)];
        auto columnBounds = playStripArea.removeFromLeft (columnWidth);
        row.setBounds (columnBounds);
        playStripArea.removeFromLeft (gap);
    }

    bounds.removeFromTop (5);
    auto captionRow = bounds.removeFromTop (14).reduced (20, 0);
    simpleCaptionLabel.setBounds (captionRow.removeFromLeft (100));
    complexCaptionLabel.setBounds (captionRow.removeFromRight (100));

    bounds.removeFromTop (14);
    patternReadout.setBounds (bounds.removeFromTop (60).reduced (20, 0));
}

} // namespace smartchord::ui
