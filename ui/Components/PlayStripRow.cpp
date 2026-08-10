#include "PlayStripRow.h"

namespace smartchord::ui
{

PlayStripRow::PlayStripRow()
{
    setInterceptsMouseClicks (true, false);
}

void PlayStripRow::setChordLabel (const juce::String& root, const juce::String& quality)
{
    rootLabel = root;
    qualityLabel = quality;
    repaint();
}

void PlayStripRow::setNotchCount (int count)
{
    notchCount = juce::jmax (1, count);
    repaint();
}

void PlayStripRow::setAccentColour (juce::Colour colour)
{
    accent = colour;
    repaint();
}

void PlayStripRow::resized()
{
    auto bounds = getLocalBounds();
    labelBounds = bounds.removeFromLeft (44);
    bounds.removeFromLeft (10);
    notchesBounds = bounds;
}

float PlayStripRow::positionForX (int x) const
{
    if (notchesBounds.getWidth() <= 0)
        return 0.0f;

    const float ratio = (static_cast<float> (x) - static_cast<float> (notchesBounds.getX()))
                       / static_cast<float> (notchesBounds.getWidth());
    return juce::jlimit (0.0f, 1.0f, ratio);
}

int PlayStripRow::notchIndexForX (int x) const
{
    if (notchCount <= 1)
        return 0;

    return juce::roundToInt (positionForX (x) * static_cast<float> (notchCount - 1));
}

void PlayStripRow::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (Palette::panelInset);
    g.fillRoundedRectangle (bounds, 7.0f);

    if (labelPressed)
    {
        g.setColour (accent.withAlpha (0.22f));
        g.fillRoundedRectangle (bounds, 7.0f);
    }

    g.setColour (Palette::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (rootLabel, labelBounds.reduced (7, 0).removeFromTop (labelBounds.getHeight() * 2 / 3),
                juce::Justification::bottomLeft);
    g.setColour (Palette::textDim);
    g.setFont (monoFont (8.0f));
    g.drawText (qualityLabel, labelBounds.reduced (7, 0).removeFromBottom (labelBounds.getHeight() / 3),
                juce::Justification::topLeft);

    const int count = juce::jmax (1, notchCount);
    const float notchWidth = static_cast<float> (notchesBounds.getWidth()) / static_cast<float> (count);

    for (int i = 0; i < count; ++i)
    {
        const juce::Rectangle<float> notchArea (
            static_cast<float> (notchesBounds.getX()) + static_cast<float> (i) * notchWidth + 1.0f,
            static_cast<float> (notchesBounds.getY()),
            notchWidth - 2.0f,
            static_cast<float> (notchesBounds.getHeight()));

        if (i == litNotchIndex)
        {
            g.setColour (accent.withAlpha (0.35f));
            g.fillRoundedRectangle (notchArea.expanded (2.0f, 0.0f), 3.0f);
            g.setColour (accent);
            g.fillRoundedRectangle (notchArea, 2.0f);
        }
        else
        {
            g.setColour (Palette::text.withAlpha (0.16f));
            g.fillRoundedRectangle (notchArea, 2.0f);
        }
    }
}

void PlayStripRow::mouseDown (const juce::MouseEvent& event)
{
    if (labelBounds.contains (event.getPosition()))
    {
        labelPressed = true;
        repaint();

        if (onChordGesture != nullptr)
            onChordGesture (slotIndex, true);
        return;
    }

    notchesPressed = true;
    litNotchIndex = notchIndexForX (event.x);
    repaint();

    if (onNotchGesture != nullptr)
        onNotchGesture (slotIndex, StripGesturePhase::Down, positionForX (event.x));
}

void PlayStripRow::mouseDrag (const juce::MouseEvent& event)
{
    if (! notchesPressed)
        return;

    const int idx = notchIndexForX (event.x);
    if (idx != litNotchIndex)
    {
        litNotchIndex = idx;
        repaint();
    }

    if (onNotchGesture != nullptr)
        onNotchGesture (slotIndex, StripGesturePhase::Move, positionForX (event.x));
}

void PlayStripRow::mouseUp (const juce::MouseEvent& event)
{
    if (labelPressed)
    {
        labelPressed = false;
        repaint();

        if (onChordGesture != nullptr)
            onChordGesture (slotIndex, false);
        return;
    }

    if (notchesPressed)
    {
        notchesPressed = false;
        litNotchIndex = -1;
        repaint();

        if (onNotchGesture != nullptr)
            onNotchGesture (slotIndex, StripGesturePhase::Up, positionForX (event.x));
    }
}

} // namespace smartchord::ui
