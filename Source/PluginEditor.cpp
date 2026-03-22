#include "PluginEditor.h"

//==============================================================================
MidiEnvelopeEditor::MidiEnvelopeEditor (MidiEnvelopeProcessor& proc)
    : AudioProcessorEditor (proc),
      processor    (proc),
      envelopePane (proc),
      keyMapPane   (proc),
      scopePane    (proc)
{
    setLookAndFeel (&laf);

    // ── Three-panel vertical split: envelope / keymap / scope ────────────────
    // Items: 0=envelope, 1=divider1, 2=keymap, 3=divider2, 4=scope
    layout.setItemLayout (0, 150, -1.0, -0.45); // min 150px, default 45%
    layout.setItemLayout (1,   5,    5,     5); // divider1: fixed 5px
    layout.setItemLayout (2, 100, -1.0, -0.35); // min 100px, default 35%
    layout.setItemLayout (3,   5,    5,     5); // divider2: fixed 5px
    layout.setItemLayout (4,  70, -1.0, -0.20); // min 70px,  default 20%

    divider1 = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 1, false);
    divider2 = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 3, false);

    addAndMakeVisible (envelopePane);
    addAndMakeVisible (keyMapPane);
    addAndMakeVisible (scopePane);
    addAndMakeVisible (*divider1);
    addAndMakeVisible (*divider2);

#if JUCE_IOS
    setSize (800, 600);
    setResizable (true, true);
    setResizeLimits (600, 420, 1200, 900);
#else
    setSize (1000, 700);
    setResizable (true, true);
    setResizeLimits (700, 480, 1800, 1400);
#endif
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
    juce::Component* items[] = {
        &envelopePane, divider1.get(), &keyMapPane, divider2.get(), &scopePane
    };
    layout.layOutComponents (items, 5,
                             bounds.getX(), bounds.getY(),
                             bounds.getWidth(), bounds.getHeight(),
                             true,   // vertical stack
                             true);  // resize
}
