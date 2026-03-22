#include "PluginEditor.h"

//==============================================================================
MidiEnvelopeEditor::MidiEnvelopeEditor (MidiEnvelopeProcessor& proc)
    : AudioProcessorEditor (proc),
      processor    (proc),
      envelopePane (proc),
      keyMapPane   (proc),
      scopePane    (proc)
#if !JUCE_IOS
    , midiOutPane  (proc)
#endif
{
    setLookAndFeel (&laf);

#if JUCE_IOS
    // ── Three-panel layout for iOS ───────────────────────────────────────────
    // Items: 0=envelope, 1=div, 2=keymap, 3=div, 4=scope
    layout.setItemLayout (0, 150, -1.0, -0.45);
    layout.setItemLayout (1,   5,    5,     5);
    layout.setItemLayout (2, 100, -1.0, -0.35);
    layout.setItemLayout (3,   5,    5,     5);
    layout.setItemLayout (4,  70, -1.0, -0.20);

    if (proc.uiPanelSizes.size() == 3)
    {
        layout.setItemLayout (0, 150, -1.0, proc.uiPanelSizes[0]);
        layout.setItemLayout (2, 100, -1.0, proc.uiPanelSizes[1]);
        layout.setItemLayout (4,  70, -1.0, proc.uiPanelSizes[2]);
    }

    divider1 = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 1, false);
    divider2 = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 3, false);

    addAndMakeVisible (envelopePane);
    addAndMakeVisible (keyMapPane);
    addAndMakeVisible (scopePane);
    addAndMakeVisible (*divider1);
    addAndMakeVisible (*divider2);
#else
    // ── Four-panel layout for macOS ──────────────────────────────────────────
    // Items: 0=envelope, 1=div, 2=keymap, 3=div, 4=scope, 5=div, 6=midiout
    layout.setItemLayout (0, 150, -1.0, -0.40);
    layout.setItemLayout (1,   5,    5,     5);
    layout.setItemLayout (2, 100, -1.0, -0.30);
    layout.setItemLayout (3,   5,    5,     5);
    layout.setItemLayout (4,  70, -1.0, -0.18);
    layout.setItemLayout (5,   5,    5,     5);
    layout.setItemLayout (6,  70, -1.0, -0.12);

    if (proc.uiPanelSizes.size() == 4)
    {
        layout.setItemLayout (0, 150, -1.0, proc.uiPanelSizes[0]);
        layout.setItemLayout (2, 100, -1.0, proc.uiPanelSizes[1]);
        layout.setItemLayout (4,  70, -1.0, proc.uiPanelSizes[2]);
        layout.setItemLayout (6,  70, -1.0, proc.uiPanelSizes[3]);
    }

    divider1 = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 1, false);
    divider2 = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 3, false);
    divider3 = std::make_unique<juce::StretchableLayoutResizerBar> (&layout, 5, false);

    addAndMakeVisible (envelopePane);
    addAndMakeVisible (keyMapPane);
    addAndMakeVisible (scopePane);
    addAndMakeVisible (midiOutPane);
    addAndMakeVisible (*divider1);
    addAndMakeVisible (*divider2);
    addAndMakeVisible (*divider3);
#endif

    envelopePane.setSelectedEnvIdx (proc.uiSelectedEnv);

#if JUCE_IOS
    setSize (proc.uiEditorWidth  > 0 ? proc.uiEditorWidth  : 800,
             proc.uiEditorHeight > 0 ? proc.uiEditorHeight : 600);
    setResizable (true, true);
    setResizeLimits (600, 420, 1200, 900);
#else
    setSize (proc.uiEditorWidth  > 0 ? proc.uiEditorWidth  : 1000,
             proc.uiEditorHeight > 0 ? proc.uiEditorHeight : 700);
    setResizable (true, true);
    setResizeLimits (700, 480, 1800, 1400);
#endif
}

MidiEnvelopeEditor::~MidiEnvelopeEditor()
{
    processor.uiEditorWidth  = getWidth();
    processor.uiEditorHeight = getHeight();
    processor.uiSelectedEnv  = envelopePane.getSelectedEnvIdx();

    processor.uiPanelSizes.clear();
    processor.uiPanelSizes.add (envelopePane.getHeight());
    processor.uiPanelSizes.add (keyMapPane.getHeight());
    processor.uiPanelSizes.add (scopePane.getHeight());
#if !JUCE_IOS
    processor.uiPanelSizes.add (midiOutPane.getHeight());
#endif

    setLookAndFeel (nullptr);
}

void MidiEnvelopeEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));
}

void MidiEnvelopeEditor::resized()
{
    auto bounds = getLocalBounds();
#if JUCE_IOS
    juce::Component* items[] = {
        &envelopePane, divider1.get(), &keyMapPane, divider2.get(), &scopePane
    };
    layout.layOutComponents (items, 5,
                             bounds.getX(), bounds.getY(),
                             bounds.getWidth(), bounds.getHeight(),
                             true, true);
#else
    juce::Component* items[] = {
        &envelopePane, divider1.get(), &keyMapPane, divider2.get(),
        &scopePane, divider3.get(), &midiOutPane
    };
    layout.layOutComponents (items, 7,
                             bounds.getX(), bounds.getY(),
                             bounds.getWidth(), bounds.getHeight(),
                             true, true);
#endif
}
