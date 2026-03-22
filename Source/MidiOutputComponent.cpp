#include "MidiOutputComponent.h"
#if !JUCE_IOS

#include "PluginProcessor.h"
#include "MonokaiLookAndFeel.h"

//==============================================================================
MidiOutputComponent::MidiOutputComponent (MidiEnvelopeProcessor& proc)
    : processor (proc)
{
    headerIcon = Icons::make (Icons::MidiOut);

    deviceCombo.setTextWhenNothingSelected ("-- select device --");
    deviceCombo.onChange = [this] { openSelected(); };
    addAndMakeVisible (deviceCombo);

    refreshBtn.onClick = [this] { refreshDeviceList(); };
    addAndMakeVisible (refreshBtn);

#if JUCE_MAC
    virtualBtn.onClick = [this] { createVirtual(); };
    addAndMakeVisible (virtualBtn);
#endif

    statusLabel.setColour (juce::Label::textColourId,
                           juce::Colour (MonokaiLookAndFeel::Dim));
    statusLabel.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f)));
    statusLabel.setText ("No device connected", juce::dontSendNotification);
    addAndMakeVisible (statusLabel);

    refreshDeviceList();
}

MidiOutputComponent::~MidiOutputComponent()
{
    processor.setDirectMidiOutput (nullptr);
}

//==============================================================================
void MidiOutputComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));
    Icons::paintHeader (g, getWidth(), kHeaderH, "MIDI Out", headerIcon.get());
}

void MidiOutputComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (kHeaderH);

    auto row1 = bounds.removeFromTop (28).reduced (4, 3);
    refreshBtn.setBounds (row1.removeFromRight (60));
    row1.removeFromRight (4);
#if JUCE_MAC
    virtualBtn.setBounds (row1.removeFromRight (90));
    row1.removeFromRight (4);
#endif
    deviceCombo.setBounds (row1);

    statusLabel.setBounds (bounds.reduced (6, 2));
}

//==============================================================================
void MidiOutputComponent::refreshDeviceList()
{
    devices = juce::MidiOutput::getAvailableDevices();

    deviceCombo.clear (juce::dontSendNotification);
    deviceCombo.addItem ("None", 1);
    for (int i = 0; i < devices.size(); ++i)
        deviceCombo.addItem (devices[i].name, i + 2);

    deviceCombo.setSelectedId (1, juce::dontSendNotification);
    processor.setDirectMidiOutput (nullptr);
    statusLabel.setText ("No device connected", juce::dontSendNotification);
}

void MidiOutputComponent::openSelected()
{
    int id = deviceCombo.getSelectedId();

    if (id <= 1)
    {
        processor.setDirectMidiOutput (nullptr);
        statusLabel.setText ("No device connected", juce::dontSendNotification);
        return;
    }

    int idx = id - 2;
    if (idx < 0 || idx >= devices.size()) return;

    auto dev = juce::MidiOutput::openDevice (devices[idx].identifier);
    if (dev != nullptr)
    {
        statusLabel.setText ("Connected: " + devices[idx].name,
                             juce::dontSendNotification);
        processor.setDirectMidiOutput (std::move (dev));
    }
    else
    {
        statusLabel.setText ("Failed to open: " + devices[idx].name,
                             juce::dontSendNotification);
        processor.setDirectMidiOutput (nullptr);
    }
}

void MidiOutputComponent::createVirtual()
{
    auto dev = juce::MidiOutput::createNewDevice ("Dan");
    if (dev == nullptr)
    {
        statusLabel.setText ("Failed to create virtual device",
                             juce::dontSendNotification);
        return;
    }

    juce::String name = dev->getName();
    statusLabel.setText ("Virtual device: " + name, juce::dontSendNotification);
    processor.setDirectMidiOutput (std::move (dev));

    // Refresh the list so the new virtual port appears; leave combo on "None"
    // since we're holding the device directly (not via openDevice).
    refreshDeviceList();
    deviceCombo.setSelectedId (1, juce::dontSendNotification);
    statusLabel.setText ("Virtual device active: " + name,
                         juce::dontSendNotification);
}

#endif // !JUCE_IOS
