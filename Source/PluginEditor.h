#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeEditorComponent.h"
#include "KeyMapperComponent.h"
#include "ScopeComponent.h"
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
    ScopeComponent          scopePane;

    // Resizable vertical split: envelope / keymap / scope
    juce::StretchableLayoutManager                     layout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> divider1;
    std::unique_ptr<juce::StretchableLayoutResizerBar> divider2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEnvelopeEditor)
};
