#pragma once
#include <JuceHeader.h>
#include "EnvelopeData.h"
#include "EnvelopePlayer.h"

//==============================================================================
class MidiEnvelopeProcessor : public juce::AudioProcessor
{
public:
    MidiEnvelopeProcessor();
    ~MidiEnvelopeProcessor() override;

    //--- AudioProcessor overrides -----------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // TRUE on iOS (aumi AUv3 — no audio buses, recognised as MIDI FX by AUM).
    // FALSE on macOS (stereo passthrough buses — required by Ableton VST3 bus negotiation).
    bool isMidiEffect() const override { return DAN_IS_MIDI_EFFECT != 0; }
    bool supportsDoublePrecisionProcessing() const override { return true; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return true; }

    const juce::String getName() const override { return "MIDI Envelope"; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()             override { return 1; }
    int  getCurrentProgram()          override { return 0; }
    void setCurrentProgram (int)      override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //--- Shared state -----------------------------------------------------------
    EnvelopeBank        bank;
    juce::ReadWriteLock bankLock;

    /// Current playback positions — audio thread writes via tryEnter,
    /// UI thread reads with a blocking ScopedLock.
    juce::CriticalSection      playbackLock;
    juce::Array<PlaybackEntry> playbackSnapshot;

    /// Current CC output values for scope display — same tryEnter pattern.
    juce::CriticalSection   scopeLock;
    juce::Array<ScopeEntry> scopeSnapshot;

private:
    EnvelopePlayer player;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEnvelopeProcessor)
};
