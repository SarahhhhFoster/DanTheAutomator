#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>
#include <cmath>

//==============================================================================
enum class CcResolution
{
    SevenBit,       ///< Standard CC 0-127
    FourteenBit,    ///< CC MSB (cc) + CC LSB (cc+32), 0-16383
    MPE             ///< Per-note pitch-bend on allocated member channel, 14-bit
};

inline juce::String resolutionName (CcResolution r)
{
    switch (r)
    {
        case CcResolution::SevenBit:    return "7-bit";
        case CcResolution::FourteenBit: return "14-bit";
        case CcResolution::MPE:         return "MPE";
    }
    return {};
}

//==============================================================================
/// One anchor on the envelope curve.
/// Control handles are stored in absolute (time, value) space.
/// cpOut belongs to the segment leaving this anchor;
/// cpIn belongs to the segment arriving at this anchor.
struct AnchorPoint
{
    float time        = 0.0f; ///< X axis – position in beats
    float value       = 0.5f; ///< Y axis – normalised 0..1
    float cpOutTime   = 0.0f; ///< outgoing control-handle X (beats)
    float cpOutValue  = 0.5f; ///< outgoing control-handle Y
    float cpInTime    = 0.0f; ///< incoming control-handle X (beats)
    float cpInValue   = 0.5f; ///< incoming control-handle Y

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree vt ("AnchorPoint");
        vt.setProperty ("time",       time,       nullptr);
        vt.setProperty ("value",      value,      nullptr);
        vt.setProperty ("cpOutTime",  cpOutTime,  nullptr);
        vt.setProperty ("cpOutValue", cpOutValue, nullptr);
        vt.setProperty ("cpInTime",   cpInTime,   nullptr);
        vt.setProperty ("cpInValue",  cpInValue,  nullptr);
        return vt;
    }

    static AnchorPoint fromValueTree (const juce::ValueTree& vt)
    {
        AnchorPoint p;
        p.time       = vt.getProperty ("time",       0.0f);
        p.value      = vt.getProperty ("value",      0.5f);
        p.cpOutTime  = vt.getProperty ("cpOutTime",  0.0f);
        p.cpOutValue = vt.getProperty ("cpOutValue", 0.5f);
        p.cpInTime   = vt.getProperty ("cpInTime",   0.0f);
        p.cpInValue  = vt.getProperty ("cpInValue",  0.5f);
        return p;
    }
};

//==============================================================================
/// A named envelope made up of piecewise cubic Bezier segments.
class Envelope
{
public:
    juce::String name         = "Envelope";
    float        lengthInBeats = 4.0f;
    bool         looping       = false;

    std::vector<AnchorPoint> anchors;

    //--- constructors -----------------------------------------------------------
    Envelope()
    {
        AnchorPoint p0;
        p0.time = 0.0f;  p0.value = 1.0f;
        p0.cpOutTime = 1.0f; p0.cpOutValue = 1.0f;
        p0.cpInTime  = 0.0f; p0.cpInValue  = 1.0f;

        AnchorPoint p1;
        p1.time = 4.0f;  p1.value = 0.0f;
        p1.cpInTime  = 3.0f; p1.cpInValue  = 0.0f;
        p1.cpOutTime = 4.0f; p1.cpOutValue = 0.0f;

        anchors = { p0, p1 };
    }

    //--- evaluation (double-precision internally) --------------------------------
    /// Returns a value in [0,1] for the given beat position.
    float evaluate (float beatPos) const
    {
        if (anchors.size() < 2)
            return anchors.empty() ? 0.0f : anchors[0].value;

        double t = (double) beatPos;
        if (looping && lengthInBeats > 0.0f)
            t = std::fmod (t, (double) lengthInBeats);

        if (t <= (double) anchors.front().time) return anchors.front().value;
        if (t >= (double) anchors.back().time)  return anchors.back().value;

        for (size_t i = 0; i + 1 < anchors.size(); ++i)
        {
            const auto& a0 = anchors[i];
            const auto& a1 = anchors[i + 1];
            if (t >= (double) a0.time && t <= (double) a1.time)
            {
                double u = findT (t, a0.time, a0.cpOutTime, a1.cpInTime, a1.time);
                return (float) evalCubic (u, a0.value, a0.cpOutValue,
                                              a1.cpInValue, a1.value);
            }
        }
        return anchors.back().value;
    }

    //--- monotone constraint ---------------------------------------------------
    /// Clamp every control-handle's time so the Bezier curve cannot loop back
    /// in the time axis (guarantees monotone X, valid bisection search).
    void enforceMonotoneCps()
    {
        for (int i = 0; i < (int) anchors.size(); ++i)
        {
            auto& a = anchors[i];
            if (i + 1 < (int) anchors.size())
                a.cpOutTime = juce::jlimit (a.time, anchors[i + 1].time, a.cpOutTime);
            if (i > 0)
                a.cpInTime  = juce::jlimit (anchors[i - 1].time, a.time, a.cpInTime);
        }
    }

    //--- serialisation ----------------------------------------------------------
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree vt ("Envelope");
        vt.setProperty ("name",           name,           nullptr);
        vt.setProperty ("lengthInBeats",  lengthInBeats,  nullptr);
        vt.setProperty ("looping",        looping,        nullptr);
        for (const auto& a : anchors)
            vt.addChild (a.toValueTree(), -1, nullptr);
        return vt;
    }

    static Envelope fromValueTree (const juce::ValueTree& vt)
    {
        Envelope env;
        env.name          = vt.getProperty ("name",          "Envelope").toString();
        env.lengthInBeats = vt.getProperty ("lengthInBeats", 4.0f);
        env.looping       = vt.getProperty ("looping",       false);
        env.anchors.clear();
        for (int i = 0; i < vt.getNumChildren(); ++i)
            env.anchors.push_back (AnchorPoint::fromValueTree (vt.getChild (i)));
        if (env.anchors.empty())
            env = Envelope{};
        return env;
    }

private:
    /// Evaluate cubic Bezier Y at parameter u — double precision.
    static double evalCubic (double u, double y0, double c0, double c1, double y1) noexcept
    {
        double m = 1.0 - u;
        return m*m*m*y0 + 3.0*m*m*u*c0 + 3.0*m*u*u*c1 + u*u*u*y1;
    }

    /// Binary-search for u s.t. cubic-X(u) ≈ targetX — double precision.
    static double findT (double targetX,
                         double x0, double cx0, double cx1, double x1,
                         int iters = 28) noexcept
    {
        double lo = 0.0, hi = 1.0;
        for (int i = 0; i < iters; ++i)
        {
            double mid = 0.5 * (lo + hi);
            double m   = 1.0 - mid;
            double x   = m*m*m*x0 + 3.0*m*m*mid*cx0 + 3.0*m*mid*mid*cx1 + mid*mid*mid*x1;
            (x < targetX ? lo : hi) = mid;
        }
        return 0.5 * (lo + hi);
    }
};

//==============================================================================
/// Maps one MIDI input note to an envelope with playback parameters.
struct KeyMapping
{
    int          midiNote      = 60;
    int          envelopeIdx   = 0;
    float        timeStretch   = 1.0f;   ///< multiplier; default snaps to powers of 2
    int          ccNumber      = 74;     ///< target CC (14-bit uses ccNumber + ccNumber+32)
    int          outputChannel = 1;      ///< 1..16
    CcResolution resolution    = CcResolution::SevenBit;
    bool         retrigger     = true;   ///< restart envelope on repeated note-on
    bool         noteOffStops  = false;  ///< stop envelope on note-off (else runs to end)

    //--- power-of-two stretch snapping -----------------------------------------
    /// Returns the nearest power-of-2 stretch (from 1/16 up to 16×).
    static float snapToPow2 (float raw)
    {
        if (raw <= 0.0f) return 0.0625f;
        // candidate powers: 1/16 1/8 1/4 1/2 1 2 4 8 16
        static const float kStops[] = { 0.0625f, 0.125f, 0.25f, 0.5f,
                                        1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
        float best = kStops[0];
        float bestDist = std::abs (std::log2 (raw) - std::log2 (best));
        for (float s : kStops)
        {
            float d = std::abs (std::log2 (raw) - std::log2 (s));
            if (d < bestDist) { bestDist = d; best = s; }
        }
        return best;
    }

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree vt ("KeyMapping");
        vt.setProperty ("midiNote",      midiNote,           nullptr);
        vt.setProperty ("envelopeIdx",   envelopeIdx,        nullptr);
        vt.setProperty ("timeStretch",   timeStretch,        nullptr);
        vt.setProperty ("ccNumber",      ccNumber,           nullptr);
        vt.setProperty ("outputChannel", outputChannel,      nullptr);
        vt.setProperty ("resolution",    (int) resolution,   nullptr);
        vt.setProperty ("retrigger",     retrigger,          nullptr);
        vt.setProperty ("noteOffStops",  noteOffStops,       nullptr);
        return vt;
    }

    static KeyMapping fromValueTree (const juce::ValueTree& vt)
    {
        KeyMapping m;
        m.midiNote      = vt.getProperty ("midiNote",      60);
        m.envelopeIdx   = vt.getProperty ("envelopeIdx",   0);
        m.timeStretch   = vt.getProperty ("timeStretch",   1.0f);
        m.ccNumber      = vt.getProperty ("ccNumber",      74);
        m.outputChannel = vt.getProperty ("outputChannel", 1);
        m.resolution    = (CcResolution)(int) vt.getProperty ("resolution", 0);
        m.retrigger     = vt.getProperty ("retrigger",     true);
        m.noteOffStops  = vt.getProperty ("noteOffStops",  false);
        return m;
    }
};

//==============================================================================
/// Shared state: a collection of envelopes and their key-map assignments.
class EnvelopeBank : public juce::ChangeBroadcaster
{
public:
    std::vector<Envelope>   envelopes;
    std::vector<KeyMapping> mappings;

    EnvelopeBank()
    {
        Envelope e;
        e.name = "Envelope 1";
        envelopes.push_back (e);
    }

    void notifyChanged()
    {
        sendChangeMessage(); // posted to message thread; safe to call from UI
    }

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree root ("EnvelopeBank");

        juce::ValueTree envs ("Envelopes");
        for (const auto& e : envelopes)
            envs.addChild (e.toValueTree(), -1, nullptr);
        root.addChild (envs, -1, nullptr);

        juce::ValueTree maps ("Mappings");
        for (const auto& m : mappings)
            maps.addChild (m.toValueTree(), -1, nullptr);
        root.addChild (maps, -1, nullptr);

        return root;
    }

    void fromValueTree (const juce::ValueTree& root)
    {
        envelopes.clear();
        mappings.clear();

        auto envs = root.getChildWithName ("Envelopes");
        for (int i = 0; i < envs.getNumChildren(); ++i)
            envelopes.push_back (Envelope::fromValueTree (envs.getChild (i)));

        auto maps = root.getChildWithName ("Mappings");
        for (int i = 0; i < maps.getNumChildren(); ++i)
            mappings.push_back (KeyMapping::fromValueTree (maps.getChild (i)));

        if (envelopes.empty())
            envelopes.emplace_back();
    }
};
