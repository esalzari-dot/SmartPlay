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

    addAndMakeVisible (panel);
    setResizable (false, false);
    setSize (panel.getWidth(), panel.getHeight());

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
    g.fillAll (ui::Palette::background);
}

void SmartChordAudioProcessorEditor::resized()
{
    panel.setBounds (getLocalBounds());
}

} // namespace smartchord
