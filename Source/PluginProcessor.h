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

    bool isMidiEffect() const override { return true; }
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

private:
    EnvelopePlayer player;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEnvelopeProcessor)
};
