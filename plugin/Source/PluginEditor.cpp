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

    addAndMakeVisible (panel);
    setResizable (false, false);
    setSize (panel.getWidth(), panel.getHeight());
}

void SmartChordAudioProcessorEditor::pushStateToProcessor()
{
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
