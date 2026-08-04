#include "smartchord/ChordDefinition.h"

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

} // namespace smartchord
