#pragma once
#if !JUCE_IOS

#include <JuceHeader.h>
#include "Icons.h"

//==============================================================================
/// Panel for routing the plugin's MIDI output to a physical or virtual device.
///
/// Selecting a device from the combo opens it immediately.
/// "New Virtual" creates a CoreMIDI virtual source named "Dan" that any app
/// on the machine can subscribe to.
class MidiOutputComponent : public juce::Component
{
public:
    explicit MidiOutputComponent (class MidiEnvelopeProcessor& proc);
    ~MidiOutputComponent() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    static constexpr int kHeaderH = 22;

    void refreshDeviceList();
    void openSelected();
    void createVirtual();

    MidiEnvelopeProcessor& processor;

    std::unique_ptr<juce::Drawable> headerIcon;

    juce::ComboBox   deviceCombo;
    juce::TextButton refreshBtn  { "Refresh" };
#if JUCE_MAC
    juce::TextButton virtualBtn  { "New Virtual" };
#endif
    juce::Label      statusLabel;

    juce::Array<juce::MidiDeviceInfo> devices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiOutputComponent)
};

#endif // !JUCE_IOS
