#pragma once

#include "smartchord/VoicingEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace smartchord::ui
{

// Palette colore, replica dei valori CSS in docs/mockup-v2-garageband-style.html.
namespace Palette
{
    inline const juce::Colour background   { 0xffEDEFF2 };
    inline const juce::Colour panel        { 0xffFFFFFF };
    inline const juce::Colour panelEdge    { 0xffDADFE5 };
    inline const juce::Colour text         { 0xff1C1E21 };
    inline const juce::Colour textMuted    { 0xff8A909B };

    inline const juce::Colour piano        { 0xff4F9DDE };
    inline const juce::Colour pianoDark    { 0xff2E75B6 };
    inline const juce::Colour bass         { 0xffE8734A };
    inline const juce::Colour bassDark     { 0xffC24E27 };
    inline const juce::Colour guitar       { 0xff3FBF8F };
    inline const juce::Colour guitarDark   { 0xff249A6E };
    inline const juce::Colour strings      { 0xff9B7FE0 };
    inline const juce::Colour stringsDark  { 0xff7857C4 };
}

inline juce::Colour accentColourFor (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return Palette::piano;
        case InstrumentFamily::Bass:    return Palette::bass;
        case InstrumentFamily::Guitar:  return Palette::guitar;
        case InstrumentFamily::Strings: return Palette::strings;
    }
    return Palette::guitar;
}

inline juce::Colour accentDarkColourFor (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return Palette::pianoDark;
        case InstrumentFamily::Bass:    return Palette::bassDark;
        case InstrumentFamily::Guitar:  return Palette::guitarDark;
        case InstrumentFamily::Strings: return Palette::stringsDark;
    }
    return Palette::guitarDark;
}

inline juce::String displayNameFor (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return "Piano";
        case InstrumentFamily::Bass:    return "Bass";
        case InstrumentFamily::Guitar:  return "Guitar";
        case InstrumentFamily::Strings: return "Strings";
    }
    return {};
}

} // namespace smartchord::ui
