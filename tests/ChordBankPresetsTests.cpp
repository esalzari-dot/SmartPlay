#include "smartchord/ChordBankPresets.h"

#include <catch2/catch_test_macros.hpp>

using namespace smartchord;

namespace
{
    ChordBankModule makeTestBank()
    {
        ChordBankModule bank;
        bank.setChord (0, { 0, ChordQuality::Maj, 0, 0 });
        bank.setChord (1, { 7, ChordQuality::Dom7, 1, -1 });
        bank.setChord (8, { 9, ChordQuality::Min7, 2, 1 });
        return bank;
    }
}

TEST_CASE ("parseChordBankPresets su stringa vuota restituisce un vettore vuoto", "[ChordBankPresets]")
{
    CHECK (parseChordBankPresets ("").empty());
}

TEST_CASE ("presetFromChordBank e chordBankFromPreset fanno un round-trip esatto", "[ChordBankPresets]")
{
    const auto bank = makeTestBank();
    const auto preset = presetFromChordBank ("La mia progressione", bank);

    CHECK (preset.name == "La mia progressione");

    const auto rebuilt = chordBankFromPreset (preset);
    for (int slot = 0; slot < numChordBankSlots; ++slot)
    {
        INFO ("slot " << slot);
        CHECK (rebuilt.getChord (slot) == bank.getChord (slot));
    }
}

TEST_CASE ("serializeChordBankPresets e parseChordBankPresets fanno un round-trip esatto", "[ChordBankPresets]")
{
    const std::vector<ChordBankPreset> original {
        presetFromChordBank ("Verse", makeTestBank()),
        presetFromChordBank ("Chorus", ChordBankModule{}),
    };

    const auto json = serializeChordBankPresets (original);
    const auto parsed = parseChordBankPresets (json);

    REQUIRE (parsed.size() == 2);
    CHECK (parsed[0].name == "Verse");
    CHECK (parsed[1].name == "Chorus");

    for (size_t p = 0; p < original.size(); ++p)
    {
        INFO ("preset " << p);
        for (size_t slot = 0; slot < original[p].chords.size(); ++slot)
        {
            INFO ("slot " << slot);
            CHECK (parsed[p].chords[slot] == original[p].chords[slot]);
        }
    }
}

TEST_CASE ("parseChordBankPresets completa un preset con meno di 9 accordi", "[ChordBankPresets]")
{
    const std::string json = R"([
        {
            "name": "Corto",
            "chords": [
                { "rootSemitone": 2, "quality": "Min", "inversion": 0, "octaveOffset": 0 }
            ]
        }
    ])";

    const auto presets = parseChordBankPresets (json);
    REQUIRE (presets.size() == 1);
    CHECK (presets[0].chords[0] == ChordDefinition { 2, ChordQuality::Min, 0, 0 });

    // Gli slot mancanti restano al default, non lasciati indeterminati.
    for (size_t slot = 1; slot < presets[0].chords.size(); ++slot)
    {
        INFO ("slot " << slot);
        CHECK (presets[0].chords[slot] == ChordDefinition{});
    }
}

TEST_CASE ("parseChordBankPresets tronca un preset con piu' di 9 accordi", "[ChordBankPresets]")
{
    std::string chordsJson = "[";
    for (int i = 0; i < 12; ++i)
        chordsJson += (i > 0 ? "," : "") + std::string (R"({ "rootSemitone": )") + std::to_string (i)
                    + R"(, "quality": "Maj", "inversion": 0, "octaveOffset": 0 })";
    chordsJson += "]";

    const std::string json = R"([{ "name": "Lungo", "chords": )" + chordsJson + "}]";

    const auto presets = parseChordBankPresets (json);
    REQUIRE (presets.size() == 1);
    CHECK (presets[0].chords.size() == static_cast<size_t> (numChordBankSlots));
    CHECK (presets[0].chords[0].rootSemitone == 0);
    CHECK (presets[0].chords[static_cast<size_t> (numChordBankSlots - 1)].rootSemitone == numChordBankSlots - 1);
}

TEST_CASE ("parseChordBankPresets lancia su JSON non valido", "[ChordBankPresets]")
{
    CHECK_THROWS (parseChordBankPresets ("{ non e' json valido"));
}

TEST_CASE ("chordBankFromPreset non tocca lo slot attivo", "[ChordBankPresets]")
{
    ChordBankPreset preset;
    preset.name = "Test";

    const auto bank = chordBankFromPreset (preset);
    CHECK (bank.getActiveSlot() == 0); // default di ChordBankModule, non deciso dal preset
}
