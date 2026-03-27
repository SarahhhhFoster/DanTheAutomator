#pragma once
#include <JuceHeader.h>
#include "EnvelopeData.h"
#include <array>
#include <vector>

//==============================================================================
/// One actively-playing envelope instance (lives on the audio thread).
struct ActiveEnvelope
{
    int   triggerNote    = -1;
    int   inputChannel   = 1;
    int   envelopeIdx    = 0;
    int   ccNumber       = 74;
    int   outputChannel  = 1;
    float timeStretch    = 1.0f;
    CcResolution resolution = CcResolution::SevenBit;
    bool  noteOffStops   = false;
    float outputScale    = 1.0f;
    float outputOffset   = 0.0f;

    double startPpq      = 0.0;  ///< DAW PPQ position when triggered
    bool   noteHeld      = true;
    bool   finished      = false;

    /// MPE: allocated member channel (0 = not using MPE)
    int mpeChannel = 0;

    /// Last sent value (to avoid redundant CC messages).
    int lastSentValue = -1;

    /// Index into bank.mappings — lower index = higher list priority.
    int mappingIndex = 0;

    /// Last normalised [0, 1] output value — for scope display.
    float lastNormValue = 0.0f;
};

//==============================================================================
/// Manages per-note MPE channel allocation for the lower MPE zone.
/// Master channel = 1, member channels = 2..maxCh (default 2..15).
class MpeChannelAllocator
{
public:
    static constexpr int kFirstMember = 2;
    static constexpr int kLastMember  = 15;

    void reset()
    {
        for (auto& b : busy) b = false;
    }

    /// Returns a free member channel (1-based), or -1 if all busy.
    int allocate()
    {
        for (int ch = kFirstMember; ch <= kLastMember; ++ch)
            if (!busy[(size_t)(ch - 1)]) { busy[(size_t)(ch - 1)] = true; return ch; }
        return -1;
    }

    void release (int ch)
    {
        if (ch >= kFirstMember && ch <= kLastMember)
            busy[(size_t)(ch - 1)] = false;
    }

private:
    std::array<bool, 16> busy{};
};

//==============================================================================
/// Snapshot of a single playing envelope instance – written by audio thread,
/// read by the UI for the playback bar.
struct PlaybackEntry
{
    int   envIdx   = -1;
    float posBeats = 0.0f;
};

/// One scope snapshot sample — (channel, CC, normValue) written by the audio
/// thread and consumed by ScopeComponent at 30 Hz.
struct ScopeEntry
{
    int   outputChannel = 1;
    int   ccNumber      = -1;  ///< -1 = MPE pitch-bend
    float normValue     = 0.0f; ///< [0, 1] final CC value
};

//==============================================================================
/// Runs on the audio thread.  Reads note-on/off from the incoming MIDI buffer,
/// drives active envelopes against the DAW playhead, and appends CC (or
/// pitch-bend for MPE) output messages to the same buffer.
class EnvelopePlayer
{
public:
    EnvelopePlayer();

    /// Called at the start of each processBlock.
    /// bank must be held under an external read-lock for the duration of this call.
    /// snapLock / snapDest are updated via tryEnter so the UI can read positions.
    void process (juce::MidiBuffer&           midi,
                  int                          numSamples,
                  const EnvelopeBank&          bank,
                  juce::AudioPlayHead*         playHead,
                  double                       sampleRate,
                  juce::CriticalSection&       snapLock,
                  juce::Array<PlaybackEntry>&  snapDest,
                  juce::CriticalSection&       scopeLock,
                  juce::Array<ScopeEntry>&     scopeDest);

    /// Reset all playing envelopes (called on transport stop / prepare).
    void reset();

private:
    //--- internal helpers -------------------------------------------------------
    void handleNoteOn  (int note, int channel, double ppqNow,
                        const EnvelopeBank& bank);
    void handleNoteOff (int note, int channel);

    /// Append CC / pitch-bend messages to buf for all active envelopes.
    void generateOutputEvents (juce::MidiBuffer&   buf,
                               const EnvelopeBank& bank,
                               double              ppqNow,
                               int                 samplePos);

    void sendCcValue (juce::MidiBuffer& buf, ActiveEnvelope& inst,
                      float normValue, int samplePos);

    //--- state ------------------------------------------------------------------
    std::vector<ActiveEnvelope> active;
    MpeChannelAllocator         mpeAlloc;

    /// Fallback PPQ accumulator when the DAW isn't running.
    double  freePpq      = 0.0;
    double  freeBpm      = 120.0;
    double  lastSampleRate = 44100.0;
    bool    wasPlaying   = false;
};
