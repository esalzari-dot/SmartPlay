#include "smartchord/PatternLibrary.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace smartchord
{

namespace
{
    InstrumentFamily parseInstrumentFamily (const std::string& name)
    {
        if (name == "Piano")   return InstrumentFamily::Piano;
        if (name == "Bass")    return InstrumentFamily::Bass;
        if (name == "Guitar")  return InstrumentFamily::Guitar;
        if (name == "Strings") return InstrumentFamily::Strings;

        throw std::runtime_error ("PatternLibrary: instrumentFamily sconosciuta: " + name);
    }

    template <typename T>
    std::vector<T> readArrayOr (const nlohmann::json& obj, const char* key, std::vector<T> fallback = {})
    {
        if (! obj.contains (key))
            return fallback;
        return obj.at (key).get<std::vector<T>>();
    }

    // noteOrderSequence accetta "null" al posto di un indice per indicare una pausa.
    std::vector<int> readNoteOrderSequence (const nlohmann::json& obj)
    {
        std::vector<int> sequence;
        if (! obj.contains ("noteOrderSequence"))
            return sequence;

        for (const auto& entry : obj.at ("noteOrderSequence"))
            sequence.push_back (entry.is_null() ? restNoteIndex : entry.get<int>());

        return sequence;
    }

    StrumDirection parseStrumDirection (const std::string& name)
    {
        if (name == "up")        return StrumDirection::Up;
        if (name == "down")      return StrumDirection::Down;
        if (name == "alternate") return StrumDirection::Alternate;

        throw std::runtime_error ("PatternLibrary: strumDirection sconosciuta: " + name);
    }

    PatternDefinition parsePattern (const nlohmann::json& obj)
    {
        PatternDefinition pattern;

        pattern.id = obj.at ("id").get<std::string>();
        pattern.displayName = obj.at ("displayName").get<std::string>();
        pattern.instrumentFamily = parseInstrumentFamily (obj.at ("instrumentFamily").get<std::string>());
        pattern.intensityLevel = obj.at ("intensityLevel").get<int>();

        pattern.noteOrderSequence = readNoteOrderSequence (obj);
        pattern.rhythmGrid = readArrayOr<float> (obj, "rhythmGrid");
        pattern.gateLength = readArrayOr<float> (obj, "gateLength");
        pattern.velocityCurve = readArrayOr<int> (obj, "velocityCurve");
        pattern.palmMute = readArrayOr<bool> (obj, "palmMute");

        pattern.octaveSpread = obj.value ("octaveSpread", 0);
        pattern.loopLength = obj.value ("loopLength", 0);
        pattern.humanizeTiming = obj.value ("humanizeTiming", 0.0f);
        pattern.humanizeVelocity = obj.value ("humanizeVelocity", 0);
        pattern.swingAmount = obj.value ("swingAmount", 0.0f);
        pattern.strumOffsetMs = obj.value ("strumOffsetMs", 0.0f);
        pattern.strumDirection = parseStrumDirection (obj.value ("strumDirection", std::string ("up")));
        pattern.crescendoCurve = obj.value ("crescendoCurve", false);

        return pattern;
    }
}

PatternLibrary PatternLibrary::fromJson (const std::string& json)
{
    const auto root = nlohmann::json::parse (json);

    PatternLibrary library;
    for (const auto& entry : root.at ("patterns"))
        library.patterns.push_back (parsePattern (entry));

    return library;
}

PatternLibrary PatternLibrary::fromJsonFile (const std::string& path)
{
    std::ifstream file (path);
    if (! file.is_open())
        throw std::runtime_error ("PatternLibrary: impossibile aprire il file " + path);

    std::stringstream buffer;
    buffer << file.rdbuf();

    return fromJson (buffer.str());
}

const PatternDefinition* PatternLibrary::findPattern (InstrumentFamily family, int intensityLevel) const
{
    for (const auto& pattern : patterns)
        if (pattern.instrumentFamily == family && pattern.intensityLevel == intensityLevel)
            return &pattern;

    return nullptr;
}

} // namespace smartchord
