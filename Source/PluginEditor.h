#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeEditorComponent.h"
#include "KeyMapperComponent.h"
#include "MonokaiLookAndFeel.h"

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

    // Thin draggable divider between the two panes
    juce::StretchableLayoutManager           layout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> divider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEnvelopeEditor)
};
