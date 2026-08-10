#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "smartchord/PatternLibrary.h"

#include <stdexcept>

using namespace smartchord;

namespace
{
    std::string dataFilePath (const char* fileName)
    {
        return std::string (SMARTCHORD_DATA_DIR) + "/" + fileName;
    }
}

TEST_CASE("PatternLibrary loads all 16 reference patterns from data/patterns.json", "[PatternLibrary]")
{
    const auto library = PatternLibrary::fromJsonFile (dataFilePath ("patterns.json"));

    CHECK(library.getAllPatterns().size() == 16);

    const InstrumentFamily families[] = {
        InstrumentFamily::Piano, InstrumentFamily::Bass, InstrumentFamily::Guitar, InstrumentFamily::Strings
    };

    for (auto family : families)
        for (int intensity = 0; intensity < 4; ++intensity)
            CHECK(library.findPattern (family, intensity) != nullptr);
}

TEST_CASE("PatternLibrary reference dataset matches SPEC.md sezione 5.3", "[PatternLibrary]")
{
    const auto library = PatternLibrary::fromJsonFile (dataFilePath ("patterns.json"));

    SECTION("Piano Broken Chord Classic (intensita' 3) sale fino all'ottava e ridiscende")
    {
        const auto* pattern = library.findPattern (InstrumentFamily::Piano, 3);
        REQUIRE(pattern != nullptr);
        CHECK(pattern->id == "piano_broken_chord_classic");
        CHECK(pattern->noteOrderSequence == std::vector<int>{0, 1, 2, 3, 2, 1});
        CHECK(pattern->velocityCurve == std::vector<int>{100, 74, 82, 96, 82, 74});
        CHECK(pattern->octaveSpread == 1);
    }

    SECTION("Basso Walking Sincopato (intensita' 3) ha lo swing atteso")
    {
        const auto* pattern = library.findPattern (InstrumentFamily::Bass, 3);
        REQUIRE(pattern != nullptr);
        CHECK(pattern->noteOrderSequence == std::vector<int>{0, 2, 0, -1, 2});
        CHECK(pattern->swingAmount == Catch::Approx (0.15f));
    }

    SECTION("Chitarra Pad Sostenuto (intensita' 0) ha lo strumOffsetMs atteso")
    {
        const auto* pattern = library.findPattern (InstrumentFamily::Guitar, 0);
        REQUIRE(pattern != nullptr);
        CHECK(pattern->strumOffsetMs == Catch::Approx (4.0f));
    }

    SECTION("Archi Sostenuto Legato (intensita' 0) ha il crescendoCurve attivo")
    {
        const auto* pattern = library.findPattern (InstrumentFamily::Strings, 0);
        REQUIRE(pattern != nullptr);
        CHECK(pattern->crescendoCurve);
    }

    SECTION("Archi Tremolo Pizzicato (intensita' 3) ha l'humanizeVelocity atteso")
    {
        const auto* pattern = library.findPattern (InstrumentFamily::Strings, 3);
        REQUIRE(pattern != nullptr);
        CHECK(pattern->humanizeVelocity == 8);
    }
}

TEST_CASE("PatternLibrary::findPattern restituisce nullptr per combinazioni inesistenti", "[PatternLibrary]")
{
    const auto library = PatternLibrary::fromJsonFile (dataFilePath ("patterns.json"));
    CHECK(library.findPattern (InstrumentFamily::Piano, 4) == nullptr);
    CHECK(library.findPattern (InstrumentFamily::Piano, -1) == nullptr);
}

TEST_CASE("PatternLibrary::fromJson analizza i campi obbligatori e applica i default per quelli opzionali", "[PatternLibrary]")
{
    const std::string json = R"({
        "patterns": [
            {
                "id": "test_minimal",
                "displayName": "Test Minimal",
                "instrumentFamily": "Bass",
                "intensityLevel": 0,
                "noteOrderSequence": [0],
                "rhythmGrid": [1.0],
                "gateLength": [0.9]
            }
        ]
    })";

    const auto library = PatternLibrary::fromJson (json);
    REQUIRE(library.getAllPatterns().size() == 1);

    const auto& pattern = library.getAllPatterns().front();
    CHECK(pattern.id == "test_minimal");
    CHECK(pattern.instrumentFamily == InstrumentFamily::Bass);
    CHECK(pattern.velocityCurve.empty());
    CHECK(pattern.swingAmount == Catch::Approx (0.0f));
    CHECK(pattern.crescendoCurve == false);
}

TEST_CASE("PatternLibrary::fromJson lancia un'eccezione per instrumentFamily sconosciuta", "[PatternLibrary]")
{
    const std::string json = R"({
        "patterns": [
            {
                "id": "bad_family",
                "displayName": "Bad Family",
                "instrumentFamily": "Drums",
                "intensityLevel": 0,
                "noteOrderSequence": [0],
                "rhythmGrid": [1.0],
                "gateLength": [0.9]
            }
        ]
    })";

    CHECK_THROWS_AS(PatternLibrary::fromJson (json), std::runtime_error);
}
