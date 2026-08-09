#include "smartchord/LoopClock.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

using namespace smartchord;

namespace
{
    constexpr double blockBeats = 0.25;

    TransportState playing (double ppq)
    {
        return { 120.0, ppq, true, true };
    }

    TransportState stopped (double ppq)
    {
        return { 120.0, ppq, true, false };
    }
}

TEST_CASE ("LoopClock segue la posizione dell'host mentre il trasporto gira", "[loopclock]")
{
    LoopClock clock;

    const auto first = clock.advance (playing (8.0), false, blockBeats);
    REQUIRE (first.shouldPlay);
    REQUIRE (first.usingHostClock);

    // Free run spento: il loop e' agganciato alla griglia dell'host, quindi la posizione
    // nel loop coincide con la PPQ. Partire a battuta 3 significa entrare nel pattern
    // esattamente dove sarebbe stato partendo da zero.
    REQUIRE_THAT (first.loopPosition, Catch::Matchers::WithinAbs (8.0, 1e-9));

    const auto second = clock.advance (playing (8.25), false, blockBeats);
    REQUIRE_THAT (second.loopPosition, Catch::Matchers::WithinAbs (8.25, 1e-9));
}

TEST_CASE ("LoopClock non suona a trasporto fermo senza free run", "[loopclock]")
{
    LoopClock clock;

    const auto frame = clock.advance (stopped (0.0), false, blockBeats);
    REQUIRE_FALSE (frame.shouldPlay);
    REQUIRE_FALSE (frame.usingHostClock);
}

TEST_CASE ("LoopClock usa il clock interno a trasporto fermo in free run", "[loopclock]")
{
    LoopClock clock;

    // La PPQ dell'host resta congelata: senza clock interno il pattern non avanzerebbe.
    const auto first = clock.advance (stopped (4.0), true, blockBeats);
    const auto second = clock.advance (stopped (4.0), true, blockBeats);
    const auto third = clock.advance (stopped (4.0), true, blockBeats);

    REQUIRE (first.shouldPlay);
    REQUIRE_FALSE (first.usingHostClock);
    REQUIRE_THAT (first.loopPosition, Catch::Matchers::WithinAbs (0.0, 1e-9));
    REQUIRE_THAT (second.loopPosition, Catch::Matchers::WithinAbs (blockBeats, 1e-9));
    REQUIRE_THAT (third.loopPosition, Catch::Matchers::WithinAbs (2.0 * blockBeats, 1e-9));
}

TEST_CASE ("LoopClock conserva la fase quando il trasporto parte in free run", "[loopclock]")
{
    LoopClock clock;

    double expected = 0.0;
    for (int i = 0; i < 5; ++i)
    {
        const auto frame = clock.advance (stopped (100.0), true, blockBeats);
        REQUIRE_THAT (frame.loopPosition, Catch::Matchers::WithinAbs (expected, 1e-9));
        expected += blockBeats;
    }

    // Il trasporto parte da una PPQ del tutto scorrelata dal clock interno: il pattern
    // deve proseguire dove era arrivato, non sobbalzare.
    const auto afterPlay = clock.advance (playing (100.0), true, blockBeats);
    REQUIRE (afterPlay.usingHostClock);
    REQUIRE_THAT (afterPlay.loopPosition, Catch::Matchers::WithinAbs (expected, 1e-9));

    const auto next = clock.advance (playing (100.25), true, blockBeats);
    REQUIRE_THAT (next.loopPosition, Catch::Matchers::WithinAbs (expected + blockBeats, 1e-9));
}

TEST_CASE ("LoopClock conserva la fase quando il trasporto si ferma in free run", "[loopclock]")
{
    LoopClock clock;

    clock.advance (playing (16.0), true, blockBeats);
    const auto lastPlaying = clock.advance (playing (16.25), true, blockBeats);

    const auto afterStop = clock.advance (stopped (16.5), true, blockBeats);
    REQUIRE (afterStop.shouldPlay);
    REQUIRE_FALSE (afterStop.usingHostClock);
    REQUIRE_THAT (afterStop.loopPosition,
                  Catch::Matchers::WithinAbs (lastPlaying.loopPosition + blockBeats, 1e-9));
}

TEST_CASE ("LoopClock si riaggancia alla griglia dell'host quando il free run e' spento", "[loopclock]")
{
    LoopClock clock;

    // Blocchi a trasporto fermo: il clock interno avanza comunque, ma non deve
    // contaminare la fase alla ripartenza.
    for (int i = 0; i < 7; ++i)
        clock.advance (stopped (0.0), false, blockBeats);

    const auto afterPlay = clock.advance (playing (12.0), false, blockBeats);
    REQUIRE (afterPlay.shouldPlay);
    REQUIRE_THAT (afterPlay.loopPosition, Catch::Matchers::WithinAbs (12.0, 1e-9));
}

TEST_CASE ("LoopClock e' ripetibile fra due passaggi sulla stessa battuta", "[loopclock]")
{
    // Requisito di fedelta' in registrazione: due take sullo stesso intervallo devono
    // dare la stessa posizione nel loop blocco per blocco.
    auto renderPass = []
    {
        LoopClock clock;
        std::vector<double> positions;

        for (int i = 0; i < 4; ++i)
            clock.advance (stopped (0.0), false, blockBeats);

        for (int i = 0; i < 16; ++i)
            positions.push_back (clock.advance (playing (8.0 + i * blockBeats), false, blockBeats).loopPosition);

        return positions;
    };

    REQUIRE (renderPass() == renderPass());
}

TEST_CASE ("restartLoop riporta il loop a zero al blocco successivo", "[loopclock]")
{
    LoopClock clock;

    clock.advance (playing (4.0), false, blockBeats);
    clock.advance (playing (4.25), false, blockBeats);

    clock.restartLoop();

    const auto restarted = clock.advance (playing (4.5), false, blockBeats);
    REQUIRE_THAT (restarted.loopPosition, Catch::Matchers::WithinAbs (0.0, 1e-9));

    const auto next = clock.advance (playing (4.75), false, blockBeats);
    REQUIRE_THAT (next.loopPosition, Catch::Matchers::WithinAbs (blockBeats, 1e-9));
}

TEST_CASE ("restartLoop ha la precedenza sul riaggancio alla griglia", "[loopclock]")
{
    LoopClock clock;

    clock.advance (stopped (0.0), false, blockBeats);
    clock.restartLoop();

    const auto frame = clock.advance (playing (9.0), false, blockBeats);
    REQUIRE_THAT (frame.loopPosition, Catch::Matchers::WithinAbs (0.0, 1e-9));
}

TEST_CASE ("reset riporta LoopClock allo stato iniziale", "[loopclock]")
{
    LoopClock clock;

    for (int i = 0; i < 10; ++i)
        clock.advance (stopped (0.0), true, blockBeats);

    clock.reset();

    const auto frame = clock.advance (stopped (0.0), true, blockBeats);
    REQUIRE_THAT (frame.loopPosition, Catch::Matchers::WithinAbs (0.0, 1e-9));
}

TEST_CASE ("LoopClock funziona anche se l'host non fornisce la PPQ", "[loopclock]")
{
    LoopClock clock;

    TransportState transport;
    transport.hostProvidesPpq = false;
    transport.isPlaying = true;

    const auto first = clock.advance (transport, false, blockBeats);
    const auto second = clock.advance (transport, false, blockBeats);

    REQUIRE (first.shouldPlay);
    REQUIRE_FALSE (first.usingHostClock);
    REQUIRE_THAT (first.loopPosition, Catch::Matchers::WithinAbs (0.0, 1e-9));
    REQUIRE_THAT (second.loopPosition, Catch::Matchers::WithinAbs (blockBeats, 1e-9));
}
