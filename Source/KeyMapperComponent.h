#pragma once
#include <JuceHeader.h>
#include "EnvelopeData.h"

//==============================================================================
/// Table-based editor for the list of MIDI → Envelope mappings.
///
/// Columns:
///   Note | Envelope | Stretch | CC | Channel | Resolution | Retrigger | Note-off stops
class KeyMapperComponent : public juce::Component,
                            public juce::TableListBoxModel,
                            public juce::ChangeListener
{
public:
    explicit KeyMapperComponent (class MidiEnvelopeProcessor& proc);
    ~KeyMapperComponent() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

    // TableListBoxModel
    int  getNumRows() override;
    void paintRowBackground (juce::Graphics&, int row, int w, int h, bool sel) override;
    void paintCell          (juce::Graphics&, int row, int col,
                             int w, int h, bool sel) override;

    juce::Component* refreshComponentForCell (int row, int col, bool selected,
                                               juce::Component* existing) override;

    // ChangeListener
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

private:
    enum Column { ColNote=1, ColEnv, ColStretch, ColCC, ColChannel, ColRes,
                  ColRetrigger, ColNoteOff, ColScale, ColOffset };

    MidiEnvelopeProcessor& processor;
    juce::TableListBox     table;
    juce::TextButton       addBtn { "+ Add Mapping" };

    void addMapping();

    /// Returns true if a row with a lower index already owns the same MIDI note.
    bool isShadowed (int row) const;

    static juce::String noteName (int note);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyMapperComponent)
};
