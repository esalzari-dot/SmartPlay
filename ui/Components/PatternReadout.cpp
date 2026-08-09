#include "PatternReadout.h"

namespace smartchord::ui
{

PatternReadout::PatternReadout()
{
    nameLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, Palette::text);
    addAndMakeVisible (nameLabel);

    subLabel.setFont (monoFont (10.5f));
    subLabel.setColour (juce::Label::textColourId, Palette::textDim);
    addAndMakeVisible (subLabel);

    routeLabel.setFont (monoFont (10.5f, juce::Font::bold));
    routeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (routeLabel);
}

void PatternReadout::setContent (const PatternDefinition* pattern, InstrumentFamily family,
                                  int intensityLevel, const juce::String& chordLabel, juce::Colour accentColour)
{
    routeAccent = accentColour;

    if (pattern != nullptr)
    {
        nameLabel.setText (pattern->displayName, juce::dontSendNotification);
        subLabel.setText ((displayNameFor (family) + "  \xc2\xb7  INT " + juce::String (intensityLevel + 1)
                               + "/4  \xc2\xb7  " + chordLabel).toUpperCase(),
                           juce::dontSendNotification);
    }
    else
    {
        nameLabel.setText ("Nessun pattern", juce::dontSendNotification);
        subLabel.setText ((displayNameFor (family) + "  \xc2\xb7  INT " + juce::String (intensityLevel + 1) + "/4").toUpperCase(),
                           juce::dontSendNotification);
    }

    routeLabel.setText (("-> VST " + displayNameFor (family)).toUpperCase(), juce::dontSendNotification);
    routeLabel.setColour (juce::Label::textColourId, accentColour);
    routeLabel.setColour (juce::Label::backgroundColourId, Palette::text.withAlpha (0.05f));

    repaint();
}

void PatternReadout::setPlayheadPosition (float normalized)
{
    if (normalized < 0.0f || normalized > 1.0f)
        normalized = -1.0f;

    if (normalized == playheadPosition)
        return;

    playheadPosition = normalized;
    repaint();
}

void PatternReadout::paint (juce::Graphics& g)
{
    g.setColour (Palette::seam);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);

    if (playheadPosition < 0.0f)
        return;

    // Striscia sottile in fondo al riquadro: la stessa posizione, ridotta modulo la
    // lunghezza del loop, che scheduleEventsInWindow usa per decidere quali eventi
    // suonano in questo blocco (SPEC.md sezione 9).
    constexpr float trackHeight = 3.0f;
    auto track = getLocalBounds().toFloat().removeFromBottom (trackHeight + 4.0f).removeFromBottom (trackHeight)
                     .reduced (16.0f, 0.0f);

    g.setColour (Palette::text.withAlpha (0.06f));
    g.fillRoundedRectangle (track, trackHeight * 0.5f);

    auto fill = track.removeFromLeft (track.getWidth() * playheadPosition);
    g.setColour (routeAccent.withAlpha (0.4f));
    g.fillRoundedRectangle (fill.expanded (0.0f, 1.5f), trackHeight);
    g.setColour (routeAccent);
    g.fillRoundedRectangle (fill, trackHeight * 0.5f);
}

void PatternReadout::resized()
{
    auto bounds = getLocalBounds().reduced (16, 12);
    auto routeBounds = bounds.removeFromRight (140);
    routeLabel.setBounds (routeBounds);

    nameLabel.setBounds (bounds.removeFromTop (bounds.getHeight() / 2));
    subLabel.setBounds (bounds);
}

} // namespace smartchord::ui
