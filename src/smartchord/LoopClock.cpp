#include "smartchord/LoopClock.h"

namespace smartchord
{

LoopClock::Frame LoopClock::advance (const TransportState& transport,
                                      bool freeRunWhenStopped,
                                      double blockLengthBeats)
{
    Frame frame;
    frame.usingHostClock = transport.hostProvidesPpq && transport.isPlaying;
    frame.shouldPlay = transport.isPlaying || freeRunWhenStopped;

    const double rawPosition = frame.usingHostClock ? transport.hostPpq : internalPosition;
    const bool domainChanged = havePreviousFrame && frame.usingHostClock != wasUsingHostClock;

    if (restartPending)
    {
        phaseAnchor = rawPosition;
    }
    else if (! havePreviousFrame)
    {
        // Primo blocco: se l'host detta il tempo si parte gia' agganciati alla sua
        // griglia, altrimenti il loop comincia qui.
        phaseAnchor = frame.usingHostClock ? 0.0 : rawPosition;
    }
    else if (domainChanged)
    {
        if (frame.usingHostClock && ! freeRunWhenStopped)
            phaseAnchor = 0.0;
        else
            phaseAnchor = rawPosition - nextExpectedLoopPosition;
    }

    restartPending = false;

    frame.loopPosition = rawPosition - phaseAnchor;

    nextExpectedLoopPosition = frame.loopPosition + blockLengthBeats;
    internalPosition += blockLengthBeats;
    wasUsingHostClock = frame.usingHostClock;
    havePreviousFrame = true;

    return frame;
}

void LoopClock::restartLoop()
{
    restartPending = true;
}

void LoopClock::reset()
{
    internalPosition = 0.0;
    phaseAnchor = 0.0;
    nextExpectedLoopPosition = 0.0;
    wasUsingHostClock = false;
    havePreviousFrame = false;
    restartPending = false;
}

} // namespace smartchord
