#include "smartchord/ChordProgressions.h"

#include <catch2/catch_test_macros.hpp>

#include <set>

using namespace smartchord;

TEST_CASE ("getChordProgressions espone preset ben formati e con id univoci", "[ChordProgressions]")
{
    const auto& all = getChordProgressions();
    REQUIRE_FALSE (all.empty());

    std::set<std::string> ids;
    for (const auto& progression : all)
    {
        INFO ("progressione: " << progression.id);
        CHECK_FALSE (progression.id.empty());
        CHECK_FALSE (progression.displayName.empty());
        CHECK_FALSE (progression.degrees.empty());
        CHECK (ids.insert (progression.id).second);

        for (const auto& degree : progression.degrees)
        {
            CHECK (degree.semitonesAboveTonic >= 0);
            CHECK (degree.semitonesAboveTonic < 12);
        }
    }
}

TEST_CASE ("findChordProgression trova per id e restituisce nullptr se non esiste", "[ChordProgressions]")
{
    const auto* pop = findChordProgression ("pop_i_v_vi_iv");
    REQUIRE (pop != nullptr);
    CHECK (pop->id == "pop_i_v_vi_iv");

    CHECK (findChordProgression ("non_esiste") == nullptr);
}

TEST_CASE ("buildChordBank costruisce la progressione nella tonalita' richiesta", "[ChordProgressions]")
{
    const auto* pop = findChordProgression ("pop_i_v_vi_iv");
    REQUIRE (pop != nullptr);

    SECTION ("in Do")
    {
        const auto bank = buildChordBank (*pop, 0);

        CHECK (bank.getChord (0) == ChordDefinition { 0, ChordQuality::Maj, 0, 0 });   // C
        CHECK (bank.getChord (1) == ChordDefinition { 7, ChordQuality::Maj, 0, 0 });   // G
        CHECK (bank.getChord (2) == ChordDefinition { 9, ChordQuality::Min, 0, 0 });   // Am
        CHECK (bank.getChord (3) == ChordDefinition { 5, ChordQuality::Maj, 0, 0 });   // F
    }

    SECTION ("trasposta in Sol")
    {
        const auto bank = buildChordBank (*pop, 7);

        CHECK (bank.getChord (0) == ChordDefinition { 7, ChordQuality::Maj, 0, 0 });   // G
        CHECK (bank.getChord (1) == ChordDefinition { 2, ChordQuality::Maj, 0, 0 });   // D
        CHECK (bank.getChord (2) == ChordDefinition { 4, ChordQuality::Min, 0, 0 });   // Em
        CHECK (bank.getChord (3) == ChordDefinition { 0, ChordQuality::Maj, 0, 0 });   // C
    }
}

TEST_CASE ("buildChordBank ripete una progressione corta fino a riempire gli 8 slot", "[ChordProgressions]")
{
    const auto* pop = findChordProgression ("pop_i_v_vi_iv");
    REQUIRE (pop != nullptr);
    REQUIRE (pop->degrees.size() == 4);

    const auto bank = buildChordBank (*pop, 0);

    for (int slot = 0; slot < 4; ++slot)
    {
        INFO ("slot " << slot);
        CHECK (bank.getChord (slot) == bank.getChord (slot + 4));
    }
}

TEST_CASE ("buildChordBank normalizza una tonica fuori range", "[ChordProgressions]")
{
    const auto* pop = findChordProgression ("pop_i_v_vi_iv");
    REQUIRE (pop != nullptr);

    const auto wrapped = buildChordBank (*pop, 14);   // 14 mod 12 == 2
    const auto direct = buildChordBank (*pop, 2);
    const auto negative = buildChordBank (*pop, -10); // -10 mod 12 == 2

    for (int slot = 0; slot < numChordBankSlots; ++slot)
    {
        INFO ("slot " << slot);
        CHECK (wrapped.getChord (slot) == direct.getChord (slot));
        CHECK (negative.getChord (slot) == direct.getChord (slot));
    }
}

TEST_CASE ("tutti i preset producono 8 accordi in una tonalita' qualunque", "[ChordProgressions]")
{
    for (const auto& progression : getChordProgressions())
    {
        for (int tonic = 0; tonic < 12; ++tonic)
        {
            const auto bank = buildChordBank (progression, tonic);

            for (int slot = 0; slot < numChordBankSlots; ++slot)
            {
                INFO (progression.id << " tonica " << tonic << " slot " << slot);
                CHECK (bank.getChord (slot).rootSemitone >= 0);
                CHECK (bank.getChord (slot).rootSemitone < 12);
            }
        }
    }
}
