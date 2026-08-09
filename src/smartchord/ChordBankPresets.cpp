#include "smartchord/ChordBankPresets.h"

#include <nlohmann/json.hpp>

namespace smartchord
{

namespace
{
    ChordDefinition parseChordDefinition (const nlohmann::json& obj)
    {
        ChordDefinition chord;
        chord.rootSemitone = obj.value ("rootSemitone", 0);
        chord.quality = chordQualityFromString (obj.value ("quality", std::string ("Maj")));
        chord.inversion = obj.value ("inversion", 0);
        chord.octaveOffset = obj.value ("octaveOffset", 0);
        return chord;
    }

    nlohmann::json serializeChordDefinition (const ChordDefinition& chord)
    {
        return {
            { "rootSemitone", chord.rootSemitone },
            { "quality", toString (chord.quality) },
            { "inversion", chord.inversion },
            { "octaveOffset", chord.octaveOffset },
        };
    }
}

std::vector<ChordBankPreset> parseChordBankPresets (const std::string& json)
{
    std::vector<ChordBankPreset> presets;

    if (json.empty())
        return presets;

    const auto root = nlohmann::json::parse (json);

    for (const auto& presetJson : root)
    {
        ChordBankPreset preset;
        preset.name = presetJson.at ("name").get<std::string>();

        const auto& chordsJson = presetJson.at ("chords");
        for (size_t i = 0; i < preset.chords.size() && i < chordsJson.size(); ++i)
            preset.chords[i] = parseChordDefinition (chordsJson[i]);

        presets.push_back (std::move (preset));
    }

    return presets;
}

std::string serializeChordBankPresets (const std::vector<ChordBankPreset>& presets)
{
    auto root = nlohmann::json::array();

    for (const auto& preset : presets)
    {
        auto chordsJson = nlohmann::json::array();
        for (const auto& chord : preset.chords)
            chordsJson.push_back (serializeChordDefinition (chord));

        root.push_back ({
            { "name", preset.name },
            { "chords", chordsJson },
        });
    }

    return root.dump (2);
}

ChordBankModule chordBankFromPreset (const ChordBankPreset& preset)
{
    ChordBankModule bank;
    for (int slot = 0; slot < numChordBankSlots; ++slot)
        bank.setChord (slot, preset.chords[static_cast<size_t> (slot)]);
    return bank;
}

ChordBankPreset presetFromChordBank (const std::string& name, const ChordBankModule& bank)
{
    ChordBankPreset preset;
    preset.name = name;
    for (int slot = 0; slot < numChordBankSlots; ++slot)
        preset.chords[static_cast<size_t> (slot)] = bank.getChord (slot);
    return preset;
}

} // namespace smartchord
