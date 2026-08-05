#include <catch2/catch_test_macros.hpp>

#include "smartchord/ChordBankModule.h"

#include <stdexcept>

using namespace smartchord;

TEST_CASE("ChordBankModule starts with 8 default C major slots and slot 0 active", "[ChordBankModule]")
{
    const ChordBankModule bank;

    CHECK(bank.getActiveSlot() == 0);

    for (int slot = 0; slot < numChordBankSlots; ++slot)
    {
        const auto& chord = bank.getChord(slot);
        CHECK(chord.rootSemitone == 0);
        CHECK(chord.quality == ChordQuality::Maj);
        CHECK(chord.inversion == 0);
        CHECK(chord.octaveOffset == 0);
    }
}

TEST_CASE("ChordBankModule stores a distinct ChordDefinition per slot", "[ChordBankModule]")
{
    ChordBankModule bank;

    const ChordDefinition gMinor7{7, ChordQuality::Min7, 1, -1};
    bank.setChord(3, gMinor7);

    const auto& stored = bank.getChord(3);
    CHECK(stored.rootSemitone == 7);
    CHECK(stored.quality == ChordQuality::Min7);
    CHECK(stored.inversion == 1);
    CHECK(stored.octaveOffset == -1);

    // Gli altri slot restano al default.
    CHECK(bank.getChord(0).quality == ChordQuality::Maj);
}

TEST_CASE("ChordBankModule tracks the active slot and exposes its chord", "[ChordBankModule]")
{
    ChordBankModule bank;

    const ChordDefinition dMinor{2, ChordQuality::Min, 0, 0};
    bank.setChord(5, dMinor);
    bank.setActiveSlot(5);

    CHECK(bank.getActiveSlot() == 5);
    CHECK(bank.getActiveChord().rootSemitone == 2);
    CHECK(bank.getActiveChord().quality == ChordQuality::Min);
}

TEST_CASE("ChordBankModule rejects an out-of-range slot", "[ChordBankModule]")
{
    ChordBankModule bank;
    CHECK_THROWS_AS(bank.getChord(-1), std::out_of_range);
    CHECK_THROWS_AS(bank.getChord(numChordBankSlots), std::out_of_range);
    CHECK_THROWS_AS(bank.setChord(numChordBankSlots, ChordDefinition{}), std::out_of_range);
    CHECK_THROWS_AS(bank.setActiveSlot(-1), std::out_of_range);
}
