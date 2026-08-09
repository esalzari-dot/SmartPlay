#include "KeyboardChordShortcuts.h"

namespace smartchord::ui
{

int chordSlotForKeyPress (const juce::KeyPress& key, int maxSlots)
{
    static const int numpadCodes[9] = {
        juce::KeyPress::numberPad1, juce::KeyPress::numberPad2, juce::KeyPress::numberPad3,
        juce::KeyPress::numberPad4, juce::KeyPress::numberPad5, juce::KeyPress::numberPad6,
        juce::KeyPress::numberPad7, juce::KeyPress::numberPad8, juce::KeyPress::numberPad9
    };

    for (int i = 0; i < maxSlots && i < 9; ++i)
    {
        if (key.getKeyCode() == numpadCodes[i])
            return i;

        // Riga in alto: il testo del tasto e' l'unico modo portabile di riconoscere una
        // cifra, perche' il keycode dei tasti numerici in alto cambia a seconda del
        // layout di tastiera (a differenza del tastierino numerico, sempre uguale).
        if (key.getTextCharacter() == static_cast<juce::juce_wchar> ('1' + i))
            return i;
    }

    return -1;
}

} // namespace smartchord::ui
