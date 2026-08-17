#include "PluginEditor.h"

namespace smartchord
{

SmartChordAudioProcessorEditor::SmartChordAudioProcessorEditor (SmartChordAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      processorRef (processor),
      chordBank (processor.getChordBankSnapshot()),
      gridState (processor.getGridStateSnapshot()),
      panel (chordBank, gridState, processor.getPatternLibrary(), processor.getActiveFamilySnapshot())
{
    panel.onStateChanged = [this] { pushStateToProcessor(); };

    panel.setFreeRunControlVisible (true);
    panel.setFreeRun (processor.getFreeRunWhenStopped());
    panel.onFreeRunChanged = [this] (bool shouldFreeRun)
    {
        processorRef.setFreeRunWhenStopped (shouldFreeRun);
    };

    panel.setVoiceLeading (processor.getVoiceLeading());
    panel.onVoiceLeadingChanged = [this] (bool shouldLead)
    {
        processorRef.setVoiceLeading (shouldLead);
    };

    panel.setPatternRate (processor.getPatternRate());
    panel.onPatternRateChanged = [this] (PatternRate rate)
    {
        processorRef.setPatternRate (rate);
    };

    panel.setChordFromKeyboard (processor.getChordFromKeyboard());
    panel.onChordFromKeyboardChanged = [this] (bool shouldRecognize)
    {
        processorRef.setChordFromKeyboard (shouldRecognize);
    };

    panel.onStopRequested = [this]
    {
        processorRef.requestPanic();
    };

    lastKnownRate = processor.getPatternRate();

    lastKnownSwing = processor.getGlobalSwing();
    panel.setGlobalSwing (lastKnownSwing);
    panel.onGlobalSwingChanged = [this] (float amount01)
    {
        processorRef.setGlobalSwing (amount01);
    };

    lastKnownGate = processor.getGlobalGateLength();
    panel.setGlobalGateLength (lastKnownGate);
    panel.onGlobalGateLengthChanged = [this] (float multiplier)
    {
        processorRef.setGlobalGateLength (multiplier);
    };

    lastKnownOctaveRange = processor.getOctaveRange();
    panel.setOctaveRange (lastKnownOctaveRange);
    panel.onOctaveRangeChanged = [this] (int octaves)
    {
        processorRef.setOctaveRange (octaves);
    };

    panel.setQuantizeChordSwitch (processor.getQuantizeChordSwitch());
    panel.onQuantizeChordSwitchChanged = [this] (bool shouldQuantize)
    {
        processorRef.setQuantizeChordSwitch (shouldQuantize);
    };

    panel.setHumanizeEnabled (processor.getHumanizeEnabled());
    panel.onHumanizeEnabledChanged = [this] (bool shouldHumanize)
    {
        processorRef.setHumanizeEnabled (shouldHumanize);
    };

    panel.setPlayModeActive (processor.getPlayModeEnabled());
    panel.onPlayModeChanged = [this] (bool shouldBeActive)
    {
        processorRef.setPlayModeEnabled (shouldBeActive);
    };

    panel.onPlayStripGesture = [this] (int chordSlot, StripGesturePhase phase, float position)
    {
        processorRef.pushStripNotchGesture (chordSlot, phase, position);
    };

    panel.onPlayStripChordGesture = [this] (int chordSlot, bool down)
    {
        processorRef.pushStripChordGesture (chordSlot, down);
    };

    panel.setAvailablePresets (processor.getChordBankPresetNames());

    panel.onSavePresetRequested = [this] (const juce::String& name)
    {
        processorRef.saveChordBankPreset (name, chordBank);
        panel.setAvailablePresets (processorRef.getChordBankPresetNames());
    };

    panel.onLoadPresetRequested = [this] (const juce::String& name)
    {
        ChordBankPreset preset;
        if (! processorRef.findChordBankPreset (name, preset))
            return;

        // Il preset non porta con se' uno slot attivo (non lo cattura, vedi
        // ChordBankPresets.h): si mantiene quello corrente invece di saltare al primo pad.
        const int activeSlot = chordBank.getActiveSlot();
        chordBank = chordBankFromPreset (preset);
        chordBank.setActiveSlot (activeSlot);
        panel.refresh();
    };

    panel.onDeletePresetRequested = [this] (const juce::String& name)
    {
        processorRef.deleteChordBankPreset (name);
        panel.setAvailablePresets (processorRef.getChordBankPresetNames());
    };

    addAndMakeVisible (panel);
    setResizable (false, false);
    setSize (panel.getWidth(), panel.getHeight());

    // Cattura le cifre 1-9 anche quando il focus e' su un pulsante/combobox della UI:
    // in JUCE un tasto non gestito dal componente che ha il focus risale la gerarchia
    // fino a trovare qualcuno che lo gestisce, e keyPressed() qui e' quel qualcuno.
    setWantsKeyboardFocus (true);
    grabKeyboardFocus();

    startTimerHz (20);
}

void SmartChordAudioProcessorEditor::timerCallback()
{
    resyncFromProcessorIfNeeded();
    panel.setPlayheadPosition (processorRef.getLoopPositionSnapshot());
}

bool SmartChordAudioProcessorEditor::resyncFromProcessorIfNeeded()
{
    // consumeSlotChangedByMidi() e' un evento a se': un keyswitch e' sempre un cambio
    // "da fuori", anche quando lo slot che risulta e' per caso lo stesso di prima (es.
    // ripremuto). Tutto il resto (automazione host di uno qualunque dei parametri apvts)
    // si rileva confrontando lo snapshot con l'ultima copia nota.
    const bool keyswitchFired = processorRef.consumeSlotChangedByMidi();

    bool changed = keyswitchFired;

    const int activeSlot = processorRef.getActiveSlotSnapshot();
    if (activeSlot != chordBank.getActiveSlot())
    {
        chordBank.setActiveSlot (activeSlot);
        changed = true;
    }

    const auto family = processorRef.getActiveFamilySnapshot();
    if (family != panel.getActiveFamily())
        changed = true; // applicato sotto insieme al resto, panel non espone un setter isolato

    const auto freshGrid = processorRef.getGridStateSnapshot();
    const InstrumentFamily families[] = {
        InstrumentFamily::Piano, InstrumentFamily::Bass, InstrumentFamily::Guitar, InstrumentFamily::Strings
    };
    for (auto f : families)
        for (int slot = 0; slot < numChordSlots; ++slot)
            if (freshGrid.getIntensity (f, slot) != gridState.getIntensity (f, slot))
                changed = true;

    const auto rate = processorRef.getPatternRate();
    const auto swing = processorRef.getGlobalSwing();
    const auto gate = processorRef.getGlobalGateLength();
    const auto octaveRange = processorRef.getOctaveRange();

    if (rate != lastKnownRate || swing != lastKnownSwing || gate != lastKnownGate || octaveRange != lastKnownOctaveRange)
        changed = true;

    if (! changed)
        return false;

    const juce::ScopedValueSetter<bool> guard (applyingExternalChange, true);

    gridState = freshGrid;
    lastKnownRate = rate;
    lastKnownSwing = swing;
    lastKnownGate = gate;
    lastKnownOctaveRange = octaveRange;

    panel.setPatternRate (rate);
    panel.setGlobalSwing (swing);
    panel.setGlobalGateLength (gate);
    panel.setOctaveRange (octaveRange);
    panel.setActiveFamily (family);

    panel.refresh();
    return true;
}

void SmartChordAudioProcessorEditor::pushStateToProcessor()
{
    if (applyingExternalChange)
        return;

    processorRef.setActiveFamily (panel.getActiveFamily());
    processorRef.setActiveSlot (chordBank.getActiveSlot());

    for (int slot = 0; slot < numChordBankSlots; ++slot)
        processorRef.setChordAt (slot, chordBank.getChord (slot));

    const InstrumentFamily families[] = {
        InstrumentFamily::Piano, InstrumentFamily::Bass, InstrumentFamily::Guitar, InstrumentFamily::Strings
    };
    for (auto family : families)
        for (int slot = 0; slot < numChordSlots; ++slot)
            processorRef.setIntensityAt (family, slot, gridState.getIntensity (family, slot));
}

void SmartChordAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::Palette::ink);
}

void SmartChordAudioProcessorEditor::resized()
{
    panel.setBounds (getLocalBounds());
}

bool SmartChordAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    const int slot = ui::chordSlotForKeyPress (key, numChordBankSlots);
    if (slot < 0)
        return false;

    processorRef.setActiveSlot (slot);
    return true;
}

} // namespace smartchord
