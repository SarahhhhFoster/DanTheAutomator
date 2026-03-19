#include "PluginEditor.h"

//==============================================================================
MidiEnvelopeEditor::MidiEnvelopeEditor (MidiEnvelopeProcessor& proc)
    : AudioProcessorEditor (proc),
      processor    (proc),
      envelopePane (proc),
      keyMapPane   (proc)
{
    setLookAndFeel (&laf);

    // ── Split layout: envelope pane (top) / divider / key-map pane (bottom) ──
    // Items: 0 = envelope, 1 = divider, 2 = keymap
    layout.setItemLayout (0, 180, -1.0, -0.58); // min 180px, default 58%
    layout.setItemLayout (1, 5, 5, 5);           // divider: fixed 5px
    layout.setItemLayout (2, 120, -1.0, -0.42); // min 120px, default 42%

    divider = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 1, false);
    // Paint the divider bar with the Surface colour via its LookAndFeel
    // (StretchableLayoutResizerBar draws itself via LookAndFeel, no separate colour ID)

    addAndMakeVisible (envelopePane);
    addAndMakeVisible (keyMapPane);
    addAndMakeVisible (*divider);

    setSize (900, 680);
    setResizable (true, true);
    setResizeLimits (620, 420, 1800, 1400);
}

MidiEnvelopeEditor::~MidiEnvelopeEditor()
{
    setLookAndFeel (nullptr);
}

void MidiEnvelopeEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));
}

void MidiEnvelopeEditor::resized()
{
    auto bounds = getLocalBounds();
    juce::Component* items[] = { &envelopePane, divider.get(), &keyMapPane };
    layout.layOutComponents (items, 3,
                             bounds.getX(), bounds.getY(),
                             bounds.getWidth(), bounds.getHeight(),
                             true,  // vertical stack
                             true); // resize
}
