#include "PluginEditor.h"

//==============================================================================
MidiEnvelopeEditor::MidiEnvelopeEditor (MidiEnvelopeProcessor& p)
    : AudioProcessorEditor (p),
      proc         (p),
      envelopePane (p),
      keyMapPane   (p),
      scopePane    (p)
#if !JUCE_IOS
    , midiOutPane  (p)
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

    if (p.uiPanelSizes.size() == 3)
    {
        layout.setItemLayout (0, 150, -1.0, p.uiPanelSizes[0]);
        layout.setItemLayout (2, 100, -1.0, p.uiPanelSizes[1]);
        layout.setItemLayout (4,  70, -1.0, p.uiPanelSizes[2]);
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

    if (p.uiPanelSizes.size() == 4)
    {
        layout.setItemLayout (0, 150, -1.0, p.uiPanelSizes[0]);
        layout.setItemLayout (2, 100, -1.0, p.uiPanelSizes[1]);
        layout.setItemLayout (4,  70, -1.0, p.uiPanelSizes[2]);
        layout.setItemLayout (6,  70, -1.0, p.uiPanelSizes[3]);
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

    envelopePane.setSelectedEnvIdx (p.uiSelectedEnv);

#if JUCE_IOS
    setSize (p.uiEditorWidth  > 0 ? p.uiEditorWidth  : 800,
             p.uiEditorHeight > 0 ? p.uiEditorHeight : 600);
    setResizable (true, true);
    setResizeLimits (600, 420, 1200, 900);
#else
    setSize (p.uiEditorWidth  > 0 ? p.uiEditorWidth  : 1000,
             p.uiEditorHeight > 0 ? p.uiEditorHeight : 700);
    setResizable (true, true);
    setResizeLimits (700, 480, 1800, 1400);
#endif
}

MidiEnvelopeEditor::~MidiEnvelopeEditor()
{
    proc.uiEditorWidth  = getWidth();
    proc.uiEditorHeight = getHeight();
    proc.uiSelectedEnv  = envelopePane.getSelectedEnvIdx();

    proc.uiPanelSizes.clear();
    proc.uiPanelSizes.add (envelopePane.getHeight());
    proc.uiPanelSizes.add (keyMapPane.getHeight());
    proc.uiPanelSizes.add (scopePane.getHeight());
#if !JUCE_IOS
    proc.uiPanelSizes.add (midiOutPane.getHeight());
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
