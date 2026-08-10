#include "smartchord/ChordDefinition.h"

#include <stdexcept>

namespace smartchord
{

std::vector<int> getChordTones (ChordQuality quality)
{
    switch (quality)
    {
        case ChordQuality::Maj:    return { 0, 4, 7 };
        case ChordQuality::Min:    return { 0, 3, 7 };
        case ChordQuality::Dim:    return { 0, 3, 6 };
        case ChordQuality::Aug:    return { 0, 4, 8 };
        case ChordQuality::Sus2:   return { 0, 2, 7 };
        case ChordQuality::Sus4:   return { 0, 5, 7 };
        case ChordQuality::Maj7:   return { 0, 4, 7, 11 };
        case ChordQuality::Min7:   return { 0, 3, 7, 10 };
        case ChordQuality::Dom7:   return { 0, 4, 7, 10 };
        case ChordQuality::Min7b5: return { 0, 3, 6, 10 };
        case ChordQuality::Dim7:   return { 0, 3, 6, 9 };
        case ChordQuality::Add9:   return { 0, 4, 7, 14 };
        case ChordQuality::Six:    return { 0, 4, 7, 9 };
        case ChordQuality::Nine:   return { 0, 4, 7, 10, 14 };
    }

    return {};
}

std::vector<int> getChordScaleTones (ChordQuality quality)
{
    switch (quality)
    {
        case ChordQuality::Maj:
        case ChordQuality::Sus2:
        case ChordQuality::Sus4:
        case ChordQuality::Maj7:
        case ChordQuality::Add9:
        case ChordQuality::Six:
            return { 0, 2, 4, 5, 7, 9, 11 }; // Ionian (maggiore)

        case ChordQuality::Min:
            return { 0, 2, 3, 5, 7, 8, 10 }; // Aeolian (minore naturale)

        case ChordQuality::Dim:
        case ChordQuality::Min7b5:
        case ChordQuality::Dim7: // approssimazione: un accordo diminuito di settima e'
                                  // simmetrico (0,3,6,9) e non entra esattamente in una
                                  // scala diatonica di 7 gradi; Locrian e' la piu' vicina.
            return { 0, 1, 3, 5, 6, 8, 10 }; // Locrian

        case ChordQuality::Aug:
            return { 0, 2, 4, 5, 8, 9, 11 }; // Ionian con la quinta aumentata

        case ChordQuality::Min7:
            return { 0, 2, 3, 5, 7, 9, 10 }; // Dorian

        case ChordQuality::Dom7:
        case ChordQuality::Nine:
            return { 0, 2, 4, 5, 7, 9, 10 }; // Mixolydian
    }

    return {};
}

const char* toString (ChordQuality quality)
{
    switch (quality)
    {
        case ChordQuality::Maj:    return "Maj";
        case ChordQuality::Min:    return "Min";
        case ChordQuality::Dim:    return "Dim";
        case ChordQuality::Aug:    return "Aug";
        case ChordQuality::Sus2:   return "Sus2";
        case ChordQuality::Sus4:   return "Sus4";
        case ChordQuality::Maj7:   return "Maj7";
        case ChordQuality::Min7:   return "Min7";
        case ChordQuality::Dom7:   return "Dom7";
        case ChordQuality::Min7b5: return "Min7b5";
        case ChordQuality::Dim7:   return "Dim7";
        case ChordQuality::Add9:   return "Add9";
        case ChordQuality::Six:    return "Six";
        case ChordQuality::Nine:   return "Nine";
    }

    return "Maj";
}

ChordQuality chordQualityFromString (const std::string& name)
{
    if (name == "Maj")    return ChordQuality::Maj;
    if (name == "Min")    return ChordQuality::Min;
    if (name == "Dim")    return ChordQuality::Dim;
    if (name == "Aug")    return ChordQuality::Aug;
    if (name == "Sus2")   return ChordQuality::Sus2;
    if (name == "Sus4")   return ChordQuality::Sus4;
    if (name == "Maj7")   return ChordQuality::Maj7;
    if (name == "Min7")   return ChordQuality::Min7;
    if (name == "Dom7")   return ChordQuality::Dom7;
    if (name == "Min7b5") return ChordQuality::Min7b5;
    if (name == "Dim7")   return ChordQuality::Dim7;
    if (name == "Add9")   return ChordQuality::Add9;
    if (name == "Six")    return ChordQuality::Six;
    if (name == "Nine")   return ChordQuality::Nine;

    throw std::runtime_error ("ChordDefinition: qualita' sconosciuta: " + name);
}

} // namespace smartchord
