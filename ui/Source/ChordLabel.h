#pragma once

#include "smartchord/ChordDefinition.h"

#include <juce_core/juce_core.h>

namespace smartchord::ui
{

// Nome della nota (C, C#, D, ...) per un rootSemitone 0-11.
juce::String noteNameFor (int rootSemitone);

// Abbreviazione della qualita' dell'accordo, per le etichette dei pad (SPEC.md sezione 3).
juce::String qualityAbbreviationFor (ChordQuality quality);

} // namespace smartchord::ui
