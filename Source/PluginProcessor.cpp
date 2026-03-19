#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiEnvelopeProcessor::MidiEnvelopeProcessor()
    : AudioProcessor (BusesProperties()) // MIDI effect — no audio buses
{
}

MidiEnvelopeProcessor::~MidiEnvelopeProcessor() {}

//==============================================================================
bool MidiEnvelopeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Accept only the empty layout (pure MIDI effect)
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
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
