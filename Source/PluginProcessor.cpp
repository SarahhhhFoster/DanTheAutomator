#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiEnvelopeProcessor::MidiEnvelopeProcessor()
#if DAN_IS_MIDI_EFFECT
    // iOS / AUv3: pure MIDI effect — no audio buses
    : AudioProcessor (BusesProperties())
#else
    // macOS: declare stereo buses so Ableton's VST3 bus negotiation succeeds.
    // Audio passes through the buffer untouched; we only write MIDI.
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
}

MidiEnvelopeProcessor::~MidiEnvelopeProcessor() {}

//==============================================================================
bool MidiEnvelopeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if DAN_IS_MIDI_EFFECT
    // iOS / pure MIDI: only the disabled (no-audio) layout is valid
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
#else
    // macOS: accept any layout — audio passes through untouched
    ignoreUnused (layouts);
    return true;
#endif
}

//==============================================================================
void MidiEnvelopeProcessor::prepareToPlay (double, int)
{
    player.reset();
}

void MidiEnvelopeProcessor::releaseResources()
{
    player.reset();
}

//==============================================================================
void MidiEnvelopeProcessor::processBlock (juce::AudioBuffer<float>& audio,
                                           juce::MidiBuffer&          midi)
{
    juce::ScopedReadLock lock (bankLock);
    player.process (midi,
                    audio.getNumSamples(),
                    bank,
                    getPlayHead(),
                    getSampleRate(),
                    playbackLock,
                    playbackSnapshot,
                    scopeLock,
                    scopeSnapshot);
}

//==============================================================================
juce::AudioProcessorEditor* MidiEnvelopeProcessor::createEditor()
{
    return new MidiEnvelopeEditor (*this);
}

//==============================================================================
void MidiEnvelopeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ScopedReadLock lock (bankLock);
    auto tree = bank.toValueTree();
    juce::MemoryOutputStream stream (dest, false);
    tree.writeToStream (stream);
}

void MidiEnvelopeProcessor::setStateInformation (const void* data, int size)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) size);
    if (tree.isValid())
    {
        juce::ScopedWriteLock lock (bankLock);
        bank.fromValueTree (tree);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiEnvelopeProcessor();
}
