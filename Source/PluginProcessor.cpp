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

#if !JUCE_IOS
    // Mirror the full MIDI output to the direct device (if one is open).
    // tryEnter: if the UI is swapping devices we simply skip this block.
    if (directMidiLock.tryEnter())
    {
        if (directMidiOut != nullptr)
            for (const auto meta : midi)
                directMidiOut->sendMessageNow (meta.getMessage());
        directMidiLock.exit();
    }
#endif
}

#if !JUCE_IOS
void MidiEnvelopeProcessor::setDirectMidiOutput (std::unique_ptr<juce::MidiOutput> device)
{
    juce::ScopedLock sl (directMidiLock);
    directMidiOut = std::move (device);
}
#endif

//==============================================================================
juce::AudioProcessorEditor* MidiEnvelopeProcessor::createEditor()
{
    return new MidiEnvelopeEditor (*this);
}

//==============================================================================
void MidiEnvelopeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ScopedReadLock lock (bankLock);
    auto root = bank.toValueTree();

    juce::ValueTree ui ("UIState");
    ui.setProperty ("editorWidth",  uiEditorWidth,  nullptr);
    ui.setProperty ("editorHeight", uiEditorHeight, nullptr);
    ui.setProperty ("selectedEnv",  uiSelectedEnv,  nullptr);
#if !JUCE_IOS
    ui.setProperty ("midiDeviceId", uiMidiDeviceId, nullptr);
#endif
    juce::ValueTree ps ("PanelSizes");
    for (int i = 0; i < uiPanelSizes.size(); ++i)
        ps.setProperty ("s" + juce::String (i), uiPanelSizes[i], nullptr);
    ui.addChild (ps, -1, nullptr);
    root.addChild (ui, -1, nullptr);

    juce::MemoryOutputStream stream (dest, false);
    root.writeToStream (stream);
}

void MidiEnvelopeProcessor::setStateInformation (const void* data, int size)
{
    auto root = juce::ValueTree::readFromData (data, (size_t) size);
    if (!root.isValid())
        return;

    {
        juce::ScopedWriteLock lock (bankLock);
        bank.fromValueTree (root);
    }

    auto ui = root.getChildWithName ("UIState");
    if (ui.isValid())
    {
        uiEditorWidth  = ui.getProperty ("editorWidth",  0);
        uiEditorHeight = ui.getProperty ("editorHeight", 0);
        uiSelectedEnv  = ui.getProperty ("selectedEnv",  0);
#if !JUCE_IOS
        uiMidiDeviceId = ui.getProperty ("midiDeviceId", juce::String{}).toString();
#endif
        uiPanelSizes.clear();
        auto ps = ui.getChildWithName ("PanelSizes");
        if (ps.isValid())
        {
            for (int i = 0; ; ++i)
            {
                auto key = juce::Identifier ("s" + juce::String (i));
                if (!ps.hasProperty (key)) break;
                uiPanelSizes.add ((double) ps.getProperty (key));
            }
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiEnvelopeProcessor();
}
