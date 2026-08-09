#include "smartchord/ChordRecognizer.h"

#include <catch2/catch_test_macros.hpp>

using namespace smartchord;

TEST_CASE("recognizeChord riconosce le triadi in posizione fondamentale", "[ChordRecognizer]")
{
    SECTION("Do maggiore")
    {
        const auto chord = recognizeChord ({ 60, 64, 67 });
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 0);
        CHECK (chord->quality == ChordQuality::Maj);
        CHECK (chord->inversion == 0);
    }

    SECTION("La minore")
    {
        const auto chord = recognizeChord ({ 57, 60, 64 });
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 9);
        CHECK (chord->quality == ChordQuality::Min);
    }

    SECTION("Sol settima di dominante")
    {
        const auto chord = recognizeChord ({ 55, 59, 62, 65 });
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 7);
        CHECK (chord->quality == ChordQuality::Dom7);
    }

    SECTION("Re minore settima")
    {
        const auto chord = recognizeChord ({ 50, 53, 57, 60 });
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 2);
        CHECK (chord->quality == ChordQuality::Min7);
    }
}

TEST_CASE("recognizeChord non dipende dall'ordine ne' dalle ottave in cui si suona", "[ChordRecognizer]")
{
    const auto compact = recognizeChord ({ 60, 64, 67 });
    const auto spread = recognizeChord ({ 67, 88, 48, 64 }); // stesse classi, sparse e disordinate

    REQUIRE (compact.has_value());
    REQUIRE (spread.has_value());
    CHECK (compact->rootSemitone == spread->rootSemitone);
    CHECK (compact->quality == spread->quality);
}

TEST_CASE("recognizeChord deduce il rivolto dalla nota piu' grave", "[ChordRecognizer]")
{
    SECTION("primo rivolto: la terza al basso")
    {
        const auto chord = recognizeChord ({ 64, 67, 72 }); // Mi Sol Do
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 0);
        CHECK (chord->quality == ChordQuality::Maj);
        CHECK (chord->inversion == 1);
    }

    SECTION("secondo rivolto: la quinta al basso")
    {
        const auto chord = recognizeChord ({ 67, 72, 76 }); // Sol Do Mi
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 0);
        CHECK (chord->inversion == 2);
    }
}

TEST_CASE("recognizeChord non azzarda sotto il numero minimo di note", "[ChordRecognizer]")
{
    CHECK_FALSE (recognizeChord ({}).has_value());
    CHECK_FALSE (recognizeChord ({ 60 }).has_value());
    CHECK_FALSE (recognizeChord ({ 60, 64 }).has_value());

    // Tre note ma due sole classi di altezza: resta ambiguo.
    CHECK_FALSE (recognizeChord ({ 60, 64, 72 }).has_value());
}

TEST_CASE("recognizeChord ignora i duplicati d'ottava nel conteggio", "[ChordRecognizer]")
{
    const auto chord = recognizeChord ({ 48, 60, 64, 67, 72, 76 });
    REQUIRE (chord.has_value());
    CHECK (chord->rootSemitone == 0);
    CHECK (chord->quality == ChordQuality::Maj);
}

TEST_CASE("recognizeChord riconosce sus e accordi estesi", "[ChordRecognizer]")
{
    SECTION("Do sus4")
    {
        const auto chord = recognizeChord ({ 60, 65, 67 });
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 0);
        CHECK (chord->quality == ChordQuality::Sus4);
    }

    SECTION("Do diminuita")
    {
        const auto chord = recognizeChord ({ 60, 63, 66 });
        REQUIRE (chord.has_value());
        CHECK (chord->rootSemitone == 0);
        CHECK (chord->quality == ChordQuality::Dim);
    }

    SECTION("Do aumentata")
    {
        const auto chord = recognizeChord ({ 60, 64, 68 });
        REQUIRE (chord.has_value());
        CHECK (chord->quality == ChordQuality::Aug);
    }
}

TEST_CASE("recognizeChord restituisce sempre una fondamentale valida", "[ChordRecognizer]")
{
    // Cluster cromatico: qualunque cosa venga scelta deve comunque essere un accordo
    // ben formato, non un valore fuori range.
    const auto chord = recognizeChord ({ 60, 61, 62, 63 });
    if (chord.has_value())
    {
        CHECK (chord->rootSemitone >= 0);
        CHECK (chord->rootSemitone < 12);
        CHECK (chord->inversion >= 0);
    }
}
