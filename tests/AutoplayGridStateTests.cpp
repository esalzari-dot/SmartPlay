#include <catch2/catch_test_macros.hpp>

#include "smartchord/AutoplayGridState.h"

#include <stdexcept>

using namespace smartchord;

TEST_CASE("AutoplayGridState starts with every cell at the minimum intensity level", "[AutoplayGridState]")
{
    const AutoplayGridState state;

    for (int slot = 0; slot < numChordSlots; ++slot)
    {
        CHECK(state.getIntensity(InstrumentFamily::Piano, slot) == minIntensityLevel);
        CHECK(state.getIntensity(InstrumentFamily::Bass, slot) == minIntensityLevel);
        CHECK(state.getIntensity(InstrumentFamily::Guitar, slot) == minIntensityLevel);
        CHECK(state.getIntensity(InstrumentFamily::Strings, slot) == minIntensityLevel);
    }
}

TEST_CASE("AutoplayGridState stores an intensity level per (family, chordSlot) independently", "[AutoplayGridState]")
{
    AutoplayGridState state;

    state.setIntensity(InstrumentFamily::Guitar, 2, 3);
    state.setIntensity(InstrumentFamily::Piano, 2, 1);
    state.setIntensity(InstrumentFamily::Guitar, 5, 2);

    CHECK(state.getIntensity(InstrumentFamily::Guitar, 2) == 3);
    CHECK(state.getIntensity(InstrumentFamily::Piano, 2) == 1);
    CHECK(state.getIntensity(InstrumentFamily::Guitar, 5) == 2);

    // Le celle non toccate restano al default.
    CHECK(state.getIntensity(InstrumentFamily::Bass, 2) == minIntensityLevel);
    CHECK(state.getIntensity(InstrumentFamily::Guitar, 0) == minIntensityLevel);
}

TEST_CASE("AutoplayGridState::setIntensity overwrites a previously set cell", "[AutoplayGridState]")
{
    AutoplayGridState state;
    state.setIntensity(InstrumentFamily::Strings, 7, 1);
    state.setIntensity(InstrumentFamily::Strings, 7, 3);
    CHECK(state.getIntensity(InstrumentFamily::Strings, 7) == 3);
}

TEST_CASE("AutoplayGridState rejects an out-of-range chordSlot", "[AutoplayGridState]")
{
    AutoplayGridState state;
    CHECK_THROWS_AS(state.getIntensity(InstrumentFamily::Piano, -1), std::out_of_range);
    CHECK_THROWS_AS(state.getIntensity(InstrumentFamily::Piano, numChordSlots), std::out_of_range);
    CHECK_THROWS_AS(state.setIntensity(InstrumentFamily::Piano, numChordSlots, 0), std::out_of_range);
}

TEST_CASE("AutoplayGridState rejects an out-of-range intensityLevel", "[AutoplayGridState]")
{
    AutoplayGridState state;
    CHECK_THROWS_AS(state.setIntensity(InstrumentFamily::Piano, 0, minIntensityLevel - 1), std::out_of_range);
    CHECK_THROWS_AS(state.setIntensity(InstrumentFamily::Piano, 0, maxIntensityLevel + 1), std::out_of_range);
}

TEST_CASE("resolvePattern looks up the PatternDefinition matching (family, intensityLevel)", "[AutoplayGridState]")
{
    const std::string json = R"({
        "patterns": [
            {
                "id": "guitar_test_pattern",
                "displayName": "Guitar Test Pattern",
                "instrumentFamily": "Guitar",
                "intensityLevel": 2,
                "noteOrderSequence": [0, 1],
                "rhythmGrid": [0.5, 0.5],
                "gateLength": [0.8, 0.8]
            }
        ]
    })";
    const auto library = PatternLibrary::fromJson(json);

    const auto* found = resolvePattern(library, InstrumentFamily::Guitar, 2);
    REQUIRE(found != nullptr);
    CHECK(found->id == "guitar_test_pattern");

    CHECK(resolvePattern(library, InstrumentFamily::Guitar, 0) == nullptr);
    CHECK(resolvePattern(library, InstrumentFamily::Piano, 2) == nullptr);
}

TEST_CASE("AutoplayGridState combined with resolvePattern resolves the pattern selected on the grid", "[AutoplayGridState]")
{
    const auto library = PatternLibrary::fromJsonFile(std::string(SMARTCHORD_DATA_DIR) + "/patterns.json");

    AutoplayGridState state;
    state.setIntensity(InstrumentFamily::Strings, 3, 3);

    const int selectedIntensity = state.getIntensity(InstrumentFamily::Strings, 3);
    const auto* pattern = resolvePattern(library, InstrumentFamily::Strings, selectedIntensity);

    REQUIRE(pattern != nullptr);
    CHECK(pattern->id == "strings_tremolo_pizzicato");
}
