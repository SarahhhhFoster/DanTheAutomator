#include "EnvelopePlayer.h"

//==============================================================================
EnvelopePlayer::EnvelopePlayer()
{
    active.reserve (32);
    mpeAlloc.reset();
}

void EnvelopePlayer::reset()
{
    // Release any MPE channels
    for (auto& inst : active)
        if (inst.mpeChannel > 0)
            mpeAlloc.release (inst.mpeChannel);

    active.clear();
    freePpq = 0.0;
}

//==============================================================================
void EnvelopePlayer::process (juce::MidiBuffer&          midi,
                               int                         numSamples,
                               const EnvelopeBank&         bank,
                               juce::AudioPlayHead*        playHead,
                               double                      sampleRate,
                               juce::CriticalSection&      snapLock,
                               juce::Array<PlaybackEntry>& snapDest,
                               juce::CriticalSection&      scopeLock,
                               juce::Array<ScopeEntry>&    scopeDest)
{
    lastSampleRate = sampleRate;

    // ── Resolve current PPQ position ────────────────────────────────────────
    double ppqNow  = freePpq;
    bool   playing = false;

    if (playHead != nullptr)
    {
        juce::AudioPlayHead::CurrentPositionInfo pos{};
        if (playHead->getCurrentPosition (pos))
        {
            ppqNow  = pos.ppqPosition;
            playing = pos.isPlaying;
            freeBpm = pos.bpm > 0.0 ? pos.bpm : 120.0;
        }
    }

    if (!playing)
    {
        // Accumulate free-running PPQ
        double beatsPerSample = freeBpm / (60.0 * sampleRate);
        freePpq += numSamples * beatsPerSample;
        ppqNow   = freePpq;
    }
    else
    {
        freePpq = ppqNow; // keep in sync
    }

    wasPlaying = playing;

    // ── Process incoming MIDI ────────────────────────────────────────────────
    juce::MidiBuffer passthrough;

    for (auto it = midi.cbegin(); it != midi.cend(); ++it)
    {
        const auto msg        = (*it).getMessage();
        const int  samplePos  = (*it).samplePosition;

        if (msg.isNoteOn())
            handleNoteOn (msg.getNoteNumber(), msg.getChannel(), ppqNow, bank);
        else if (msg.isNoteOff())
            handleNoteOff (msg.getNoteNumber(), msg.getChannel());

        // Pass note events through so downstream synths hear them
        passthrough.addEvent (msg, samplePos);
    }

    // Replace buffer contents with passthrough + generated CC
    midi.clear();
    midi.addEvents (passthrough, 0, numSamples, 0);

    // ── Generate envelope output ─────────────────────────────────────────────
    generateOutputEvents (midi, bank, ppqNow, 0);

    // ── Remove finished instances ─────────────────────────────────────────────
    active.erase (std::remove_if (active.begin(), active.end(),
                                   [this](const ActiveEnvelope& a) {
                                       if (a.finished && a.mpeChannel > 0)
                                           mpeAlloc.release (a.mpeChannel);
                                       return a.finished;
                                   }),
                   active.end());

    // ── Update UI playback snapshot (non-blocking try) ────────────────────────
    if (snapLock.tryEnter())
    {
        snapDest.clearQuick();
        for (const auto& inst : active)
        {
            if (!inst.finished)
            {
                double elapsed = (ppqNow - inst.startPpq) / inst.timeStretch;
                snapDest.add ({ inst.envelopeIdx, (float) elapsed });
            }
        }
        snapLock.exit();
    }

    // ── Update scope snapshot (non-blocking try) ──────────────────────────────
    if (scopeLock.tryEnter())
    {
        scopeDest.clearQuick();
        for (const auto& inst : active)
        {
            if (!inst.finished)
            {
                ScopeEntry e;
                e.outputChannel = (inst.resolution == CcResolution::MPE && inst.mpeChannel > 0)
                                  ? inst.mpeChannel : inst.outputChannel;
                e.ccNumber  = (inst.resolution == CcResolution::MPE) ? -1 : inst.ccNumber;
                e.normValue = inst.lastNormValue;
                scopeDest.add (e);
            }
        }
        scopeLock.exit();
    }
}

//==============================================================================
void EnvelopePlayer::handleNoteOn (int note, int channel, double ppqNow,
                                    const EnvelopeBank& bank)
{
    // Find all mappings for this note
    for (int mi = 0; mi < (int) bank.mappings.size(); ++mi)
    {
        const auto& map = bank.mappings[mi];
        if (map.midiNote != note) continue;

        // Always stop any existing instance for this note+mapping so repeated
        // note-ons never stack multiple envelopes onto the same CC output.
        // (The retrigger switch controls UI behaviour; overlap is never correct.)
        for (auto& inst : active)
        {
            if (inst.triggerNote == note && inst.envelopeIdx == map.envelopeIdx
                && inst.ccNumber == map.ccNumber
                && inst.outputChannel == map.outputChannel)
            {
                inst.finished = true;
            }
        }

        ActiveEnvelope inst;
        inst.triggerNote   = note;
        inst.inputChannel  = channel;
        inst.envelopeIdx   = map.envelopeIdx;
        inst.ccNumber      = map.ccNumber;
        inst.outputChannel = map.outputChannel;
        inst.timeStretch   = map.timeStretch;
        inst.resolution    = map.resolution;
        inst.noteOffStops  = map.noteOffStops;
        inst.outputScale   = map.outputScale;
        inst.outputOffset  = map.outputOffset;
        inst.mappingIndex  = mi;
        inst.startPpq      = ppqNow;
        inst.noteHeld      = true;
        inst.finished      = false;
        inst.lastSentValue = -1;

        if (inst.resolution == CcResolution::MPE)
        {
            int mch = mpeAlloc.allocate();
            inst.mpeChannel = (mch > 0) ? mch : 1; // fallback to ch1 if full
        }

        active.push_back (inst);
    }
}

void EnvelopePlayer::handleNoteOff (int note, int channel)
{
    for (auto& inst : active)
    {
        if (inst.triggerNote == note && inst.inputChannel == channel)
        {
            inst.noteHeld = false;
            if (inst.noteOffStops)
                inst.finished = true;
        }
    }
}

//==============================================================================
void EnvelopePlayer::generateOutputEvents (juce::MidiBuffer&   buf,
                                            const EnvelopeBank& bank,
                                            double              ppqNow,
                                            int                 samplePos)
{
    for (auto& inst : active)
    {
        if (inst.finished) continue;

        int envIdx = inst.envelopeIdx;
        if (envIdx < 0 || envIdx >= (int) bank.envelopes.size()) continue;

        const auto& env = bank.envelopes[envIdx];

        double elapsed   = (ppqNow - inst.startPpq) / inst.timeStretch;
        float  beatPos   = (float) elapsed;
        float  envLen    = env.lengthInBeats;

        // Check if envelope has finished (non-looping)
        if (!env.looping && beatPos >= envLen)
        {
            beatPos = envLen;
            inst.finished = true;
        }

        // Evaluate bipolar signal value [-1, 1] — no clamp here
        float signalValue = env.evaluate (beatPos);

        // Primacy: if another active instance targets the same (channel, CC) with a
        // lower mapping index (higher in the list), suppress this instance's output
        // but keep evaluating it so it takes over immediately when the dominant one ends.
        auto getKey = [] (const ActiveEnvelope& a) -> std::pair<int,int>
        {
            int ch = (a.resolution == CcResolution::MPE && a.mpeChannel > 0)
                     ? a.mpeChannel : a.outputChannel;
            int cc = (a.resolution == CcResolution::MPE) ? -1 : a.ccNumber;
            return { ch, cc };
        };

        bool dominated = false;
        const auto myKey = getKey (inst);
        for (const auto& other : active)
        {
            if (&other == &inst || other.finished) continue;
            if (getKey (other) == myKey && other.mappingIndex < inst.mappingIndex)
            {
                dominated = true;
                break;
            }
        }

        if (dominated)
        {
            // Update scope value so the trace stays live, but don't emit MIDI.
            // Invalidate lastSentValue so the correct value is sent immediately
            // when dominance is lifted.
            float processed = inst.outputOffset + inst.outputScale * signalValue;
            inst.lastNormValue  = juce::jlimit (0.0f, 1.0f, (processed + 1.0f) * 0.5f);
            inst.lastSentValue  = -1;
        }
        else
        {
            sendCcValue (buf, inst, signalValue, samplePos);
        }
    }
}

//==============================================================================
void EnvelopePlayer::sendCcValue (juce::MidiBuffer& buf,
                                   ActiveEnvelope&   inst,
                                   float             signalValue,
                                   int               samplePos)
{
    // Apply per-mapping transform in signal space [-1, 1]:
    //   processed = offset + scale * signal
    float processed = inst.outputOffset + inst.outputScale * signalValue;

    // Normalize bipolar [-1, 1] to [0, 1] for CC output
    float normValue = juce::jlimit (0.0f, 1.0f, (processed + 1.0f) * 0.5f);

    // Always update scope value (even if CC is unchanged)
    inst.lastNormValue = normValue;

    switch (inst.resolution)
    {
        case CcResolution::SevenBit:
        {
            int val = juce::roundToInt (normValue * 127.0f);
            val = juce::jlimit (0, 127, val);
            if (val == inst.lastSentValue) return;
            inst.lastSentValue = val;

            buf.addEvent (juce::MidiMessage::controllerEvent (
                              inst.outputChannel, inst.ccNumber, val),
                          samplePos);
            break;
        }

        case CcResolution::FourteenBit:
        {
            int val14 = juce::roundToInt (normValue * 16383.0f);
            val14 = juce::jlimit (0, 16383, val14);
            if (val14 == inst.lastSentValue) return;
            inst.lastSentValue = val14;

            int msb   = (val14 >> 7) & 0x7F;
            int lsb   = val14 & 0x7F;
            int lsbCc = inst.ccNumber + 32;

            buf.addEvent (juce::MidiMessage::controllerEvent (
                              inst.outputChannel, inst.ccNumber, msb), samplePos);
            buf.addEvent (juce::MidiMessage::controllerEvent (
                              inst.outputChannel, lsbCc, lsb), samplePos);
            break;
        }

        case CcResolution::MPE:
        {
            // Map 0..1 → pitch-bend range -8192..8191
            int pb = juce::roundToInt ((normValue - 0.5f) * 16382.0f);
            pb = juce::jlimit (-8192, 8191, pb);
            if (pb == inst.lastSentValue) return;
            inst.lastSentValue = pb;

            int ch = inst.mpeChannel > 0 ? inst.mpeChannel : inst.outputChannel;
            buf.addEvent (juce::MidiMessage::pitchWheel (ch, pb), samplePos);
            break;
        }
    }
}
