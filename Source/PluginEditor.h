#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeEditorComponent.h"
#include "KeyMapperComponent.h"
#include "ScopeComponent.h"
#include "MonokaiLookAndFeel.h"
#if !JUCE_IOS
  #include "MidiOutputComponent.h"
#endif

//==============================================================================
class MidiEnvelopeEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiEnvelopeEditor (MidiEnvelopeProcessor& proc);
    ~MidiEnvelopeEditor() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    MidiEnvelopeProcessor& processor;

    MonokaiLookAndFeel laf;

    EnvelopeEditorComponent envelopePane;
    KeyMapperComponent      keyMapPane;
    ScopeComponent          scopePane;
#if !JUCE_IOS
    MidiOutputComponent     midiOutPane;
#endif

    // Resizable vertical split
    juce::StretchableLayoutManager                     layout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> divider1;
    std::unique_ptr<juce::StretchableLayoutResizerBar> divider2;
#if !JUCE_IOS
    std::unique_ptr<juce::StretchableLayoutResizerBar> divider3;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEnvelopeEditor)
};
