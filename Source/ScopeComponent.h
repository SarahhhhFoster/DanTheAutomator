#pragma once
#include <JuceHeader.h>
#include "MonokaiLookAndFeel.h"
#include <vector>
#include <memory>

//==============================================================================
/// Real-time scope display for MIDI CC / pitch-bend outputs.
/// Self-updates at 30 Hz by reading from MidiEnvelopeProcessor.
class ScopeComponent : public juce::Component,
                        private juce::Timer
{
public:
    explicit ScopeComponent (class MidiEnvelopeProcessor& proc);
    ~ScopeComponent() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    //── Per-CC trace with a ring-buffer history ──────────────────────────────
    struct Trace
    {
        int channel = 1;
        int cc      = -1;  ///< -1 means MPE pitch-bend

        static constexpr int kN = 256;
        float history[kN] = {};
        int   writePos    = 0;
        float lastValue   = 0.0f;

        juce::String label() const
        {
            return "Ch" + juce::String (channel)
                 + (cc >= 0 ? " CC" + juce::String (cc) : " PB");
        }

        void push (float v)
        {
            history[writePos] = v;
            writePos = (writePos + 1) % kN;
            lastValue = v;
        }

        float at (int age) const  // age=0 is most recent
        {
            return history[(writePos - 1 - age + kN) % kN];
        }
    };

    //── Viewport + inner content for horizontal scrolling ────────────────────
    class Inner : public juce::Component
    {
    public:
        explicit Inner (ScopeComponent& owner) : owner (owner) {}
        void paint (juce::Graphics& g) override;
    private:
        ScopeComponent& owner;
    };

    MidiEnvelopeProcessor& processor;

    std::vector<std::unique_ptr<Trace>> traces;
    juce::Viewport                      viewport;
    Inner                               inner { *this };

    static constexpr int kCellW = 210; ///< fixed cell width in pixels

    //── Helpers ──────────────────────────────────────────────────────────────
    void timerCallback() override;
    void syncTraces();
    Trace* getOrCreate (int channel, int cc);
    void paintCell (juce::Graphics& g, const Trace& t,
                    juce::Rectangle<float> bounds) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeComponent)
};
