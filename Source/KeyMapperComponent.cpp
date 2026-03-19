#include "KeyMapperComponent.h"
#include "PluginProcessor.h"
#include "MonokaiLookAndFeel.h"

//==============================================================================
// Cell editor components
//==============================================================================

/// ComboBox that writes back to a KeyMapping field on change.
template <typename Setter>
class MappingCombo : public juce::ComboBox
{
public:
    MappingCombo (MidiEnvelopeProcessor& proc, int row, Setter setter)
        : proc (proc), row (row), setter (std::move (setter))
    {
        onChange = [this] { commit(); };
    }

    void commit()
    {
        juce::ScopedWriteLock lock (proc.bankLock);
        if (row < (int) proc.bank.mappings.size())
            setter (proc.bank.mappings[row], getSelectedId());
        proc.bank.notifyChanged();
    }

private:
    MidiEnvelopeProcessor& proc;
    int    row;
    Setter setter;
};

/// Slider cell that writes back to a KeyMapping field.
template <typename Setter>
class MappingSlider : public juce::Slider
{
public:
    MappingSlider (MidiEnvelopeProcessor& proc, int row,
                   double lo, double hi, double step,
                   Setter setter)
        : proc (proc), row (row), setter (std::move (setter))
    {
        setRange (lo, hi, step);
        setSliderStyle (juce::Slider::LinearHorizontal);
        setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        onValueChange = [this] { commit(); };
    }

    void commit()
    {
        juce::ScopedWriteLock lock (proc.bankLock);
        if (row < (int) proc.bank.mappings.size())
            setter (proc.bank.mappings[row], getValue());
        proc.bank.notifyChanged();
    }

private:
    MidiEnvelopeProcessor& proc;
    int    row;
    Setter setter;
};

/// ToggleButton cell.
template <typename Setter>
class MappingToggle : public juce::ToggleButton
{
public:
    MappingToggle (MidiEnvelopeProcessor& proc, int row, Setter setter)
        : proc (proc), row (row), setter (std::move (setter))
    {
        onStateChange = [this] { commit(); };
    }

    void commit()
    {
        juce::ScopedWriteLock lock (proc.bankLock);
        if (row < (int) proc.bank.mappings.size())
            setter (proc.bank.mappings[row], getToggleState());
        proc.bank.notifyChanged();
    }

private:
    MidiEnvelopeProcessor& proc;
    int    row;
    Setter setter;
};

//==============================================================================
// Discrete 7-stop stretch selector — 1/8× 1/4× 1/2× [1×] 2× 4× 8×
// The centre slot (index 3) is always 1×; slots are equal-width and clickable.
//==============================================================================
class StretchEditor : public juce::Component
{
public:
    static constexpr int   kN         = 7;
    static constexpr float kStops[kN] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };
    static constexpr const char* kLabels[kN] = {
        "1/8x", "1/4x", "1/2x",
        "1x",
        "2x", "4x", "8x"
    };

    StretchEditor (MidiEnvelopeProcessor& proc, int row)
        : proc (proc), row (row)
    {
        juce::ScopedReadLock lock (proc.bankLock);
        if (row < (int) proc.bank.mappings.size())
            sel = snapToIdx (proc.bank.mappings[row].timeStretch);
    }

    void paint (juce::Graphics& g) override
    {
        float cw = getWidth() / (float) kN;
        float h  = (float) getHeight();

        for (int i = 0; i < kN; ++i)
        {
            bool isSel    = (i == sel);
            bool isCentre = (i == 3);

            g.setColour (isSel    ? juce::Colour (MonokaiLookAndFeel::Orange)
                       : isCentre ? juce::Colour (MonokaiLookAndFeel::Surface)
                                  : juce::Colour (MonokaiLookAndFeel::Bg));
            g.fillRect (i * cw, 0.0f, cw, h);

            g.setColour (juce::Colour (MonokaiLookAndFeel::Border));
            g.drawRect (i * cw, 0.0f, cw, h, 1.0f);

            g.setColour (isSel ? juce::Colour (MonokaiLookAndFeel::Bg)
                               : juce::Colour (MonokaiLookAndFeel::Fg));
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f)));
            g.drawText (kLabels[i],
                        juce::roundToInt (i * cw), 0,
                        juce::roundToInt (cw), getHeight(),
                        juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        sel = juce::jlimit (0, kN - 1, (int) (e.x / (getWidth() / (float) kN)));
        repaint();

        juce::ScopedWriteLock lock (proc.bankLock);
        if (row < (int) proc.bank.mappings.size())
            proc.bank.mappings[row].timeStretch = kStops[sel];
        proc.bank.notifyChanged();
    }

private:
    static int snapToIdx (float stretch)
    {
        int   best = 3;
        float bestD = std::abs (std::log2 (stretch));
        for (int i = 0; i < kN; ++i)
        {
            float d = std::abs (std::log2 (stretch) - std::log2 (kStops[i]));
            if (d < bestD) { bestD = d; best = i; }
        }
        return best;
    }

    MidiEnvelopeProcessor& proc;
    int row, sel = 3;
};

//==============================================================================
juce::String KeyMapperComponent::noteName (int note)
{
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    int octave = (note / 12) - 2;
    return juce::String (names[note % 12]) + juce::String (octave);
}

//==============================================================================
KeyMapperComponent::KeyMapperComponent (MidiEnvelopeProcessor& proc)
    : processor (proc), table ("MappingTable", this)
{
    addAndMakeVisible (table);

    auto& hdr = table.getHeader();
    hdr.addColumn ("Note",        ColNote,      80);
    hdr.addColumn ("Envelope",    ColEnv,       110);
    hdr.addColumn ("Stretch",     ColStretch,   110);
    hdr.addColumn ("CC",          ColCC,        60);
    hdr.addColumn ("Channel",     ColChannel,   70);
    hdr.addColumn ("Resolution",  ColRes,       90);
    hdr.addColumn ("Retrigger",   ColRetrigger, 75);
    hdr.addColumn ("Note-off stop", ColNoteOff, 90);
    hdr.addColumn ("Scale",         ColScale,   80);
    hdr.addColumn ("Offset",        ColOffset,  80);
    hdr.setStretchToFitActive (true);

    table.setColour (juce::TableListBox::backgroundColourId, juce::Colour (MonokaiLookAndFeel::Bg));
    table.setRowHeight (28);

    addAndMakeVisible (addBtn);
    addBtn.onClick = [this] { addMapping(); };

    processor.bank.addChangeListener (this);
}

KeyMapperComponent::~KeyMapperComponent()
{
    processor.bank.removeChangeListener (this);
}

void KeyMapperComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));
}

void KeyMapperComponent::resized()
{
    auto bounds = getLocalBounds();
    addBtn.setBounds (bounds.removeFromBottom (30).reduced (4, 3));
    table.setBounds (bounds);
}

//==============================================================================
int KeyMapperComponent::getNumRows()
{
    juce::ScopedReadLock lock (processor.bankLock);
    return (int) processor.bank.mappings.size();
}

void KeyMapperComponent::paintRowBackground (juce::Graphics& g, int row,
                                               int w, int h, bool sel)
{
    if (isShadowed (row))
        g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg).interpolatedWith (
                       juce::Colour (MonokaiLookAndFeel::Border), 0.15f));
    else
        g.fillAll (sel ? juce::Colour (MonokaiLookAndFeel::Surface)
                       : juce::Colour (MonokaiLookAndFeel::Bg));
}

void KeyMapperComponent::paintCell (juce::Graphics& g, int row, int col,
                                      int w, int h, bool /*sel*/)
{
    juce::ScopedReadLock lock (processor.bankLock);
    if (row >= (int) processor.bank.mappings.size()) return;
    const auto& m = processor.bank.mappings[row];

    g.setColour (isShadowed (row) ? juce::Colour (MonokaiLookAndFeel::Border)
                                  : juce::Colour (MonokaiLookAndFeel::Fg));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));

    juce::String text;
    switch (col)
    {
        case ColNote:    text = noteName (m.midiNote); break;
        case ColCC:      text = juce::String (m.ccNumber); break;
        case ColChannel: text = juce::String (m.outputChannel); break;
        default: break;
    }
    if (text.isNotEmpty())
        g.drawText (text, 4, 0, w - 4, h, juce::Justification::centredLeft);
}

//==============================================================================
juce::Component* KeyMapperComponent::refreshComponentForCell (
    int row, int col, bool /*selected*/, juce::Component* existing)
{
    // We only create components for the editable columns
    switch (col)
    {
        //── Note picker ────────────────────────────────────────────────────
        case ColNote:
        {
            auto* cb = dynamic_cast<juce::ComboBox*> (existing);
            if (cb == nullptr)
            {
                cb = new juce::ComboBox();
                for (int n = 0; n < 128; ++n)
                    cb->addItem (noteName (n), n + 1);
            }
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    cb->setSelectedId (processor.bank.mappings[row].midiNote + 1,
                                      juce::dontSendNotification);
            }
            cb->onChange = [this, cb, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].midiNote = cb->getSelectedId() - 1;
                processor.bank.notifyChanged();
            };
            return cb;
        }

        //── Envelope picker ────────────────────────────────────────────────
        case ColEnv:
        {
            auto* cb = dynamic_cast<juce::ComboBox*> (existing);
            if (cb == nullptr) cb = new juce::ComboBox();
            cb->clear (juce::dontSendNotification);
            {
                juce::ScopedReadLock lock (processor.bankLock);
                for (int i = 0; i < (int) processor.bank.envelopes.size(); ++i)
                    cb->addItem (processor.bank.envelopes[i].name, i + 1);
                if (row < (int) processor.bank.mappings.size())
                    cb->setSelectedId (processor.bank.mappings[row].envelopeIdx + 1,
                                      juce::dontSendNotification);
            }
            cb->onChange = [this, cb, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].envelopeIdx = cb->getSelectedId() - 1;
                processor.bank.notifyChanged();
            };
            return cb;
        }

        //── Stretch ────────────────────────────────────────────────────────
        case ColStretch:
        {
            // Re-create for correct row binding
            delete existing;
            return new StretchEditor (processor, row);
        }

        //── CC number ──────────────────────────────────────────────────────
        case ColCC:
        {
            auto* cb = dynamic_cast<juce::ComboBox*> (existing);
            if (cb == nullptr)
            {
                cb = new juce::ComboBox();
                for (int n = 0; n < 128; ++n)
                    cb->addItem ("CC " + juce::String (n), n + 1);
            }
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    cb->setSelectedId (processor.bank.mappings[row].ccNumber + 1,
                                      juce::dontSendNotification);
            }
            cb->onChange = [this, cb, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].ccNumber = cb->getSelectedId() - 1;
                processor.bank.notifyChanged();
            };
            return cb;
        }

        //── Channel ────────────────────────────────────────────────────────
        case ColChannel:
        {
            auto* cb = dynamic_cast<juce::ComboBox*> (existing);
            if (cb == nullptr)
            {
                cb = new juce::ComboBox();
                for (int ch = 1; ch <= 16; ++ch)
                    cb->addItem ("Ch " + juce::String (ch), ch);
            }
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    cb->setSelectedId (processor.bank.mappings[row].outputChannel,
                                      juce::dontSendNotification);
            }
            cb->onChange = [this, cb, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].outputChannel = cb->getSelectedId();
                processor.bank.notifyChanged();
            };
            return cb;
        }

        //── Resolution ────────────────────────────────────────────────────
        case ColRes:
        {
            auto* cb = dynamic_cast<juce::ComboBox*> (existing);
            if (cb == nullptr)
            {
                cb = new juce::ComboBox();
                cb->addItem ("7-bit",  1);
                cb->addItem ("14-bit", 2);
                cb->addItem ("MPE",    3);
            }
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    cb->setSelectedId ((int) processor.bank.mappings[row].resolution + 1,
                                      juce::dontSendNotification);
            }
            cb->onChange = [this, cb, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].resolution =
                        (CcResolution)(cb->getSelectedId() - 1);
                processor.bank.notifyChanged();
            };
            return cb;
        }

        //── Retrigger ─────────────────────────────────────────────────────
        case ColRetrigger:
        {
            auto* tb = dynamic_cast<juce::ToggleButton*> (existing);
            if (tb == nullptr) tb = new juce::ToggleButton();
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    tb->setToggleState (processor.bank.mappings[row].retrigger,
                                       juce::dontSendNotification);
            }
            tb->onStateChange = [this, tb, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].retrigger = tb->getToggleState();
                processor.bank.notifyChanged();
            };
            return tb;
        }

        //── Note-off stops ────────────────────────────────────────────────
        case ColNoteOff:
        {
            auto* tb = dynamic_cast<juce::ToggleButton*> (existing);
            if (tb == nullptr) tb = new juce::ToggleButton();
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    tb->setToggleState (processor.bank.mappings[row].noteOffStops,
                                       juce::dontSendNotification);
            }
            tb->onStateChange = [this, tb, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].noteOffStops = tb->getToggleState();
                processor.bank.notifyChanged();
            };
            return tb;
        }

        //── Output scale [-1, 1] ──────────────────────────────────────────
        case ColScale:
        {
            auto* sl = dynamic_cast<juce::Slider*> (existing);
            if (sl == nullptr)
            {
                sl = new juce::Slider();
                sl->setRange (-1.0, 1.0, 0.01);
                sl->setSliderStyle (juce::Slider::LinearHorizontal);
                sl->setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 20);
            }
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    sl->setValue (processor.bank.mappings[row].outputScale,
                                  juce::dontSendNotification);
            }
            sl->onValueChange = [this, sl, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].outputScale = (float) sl->getValue();
                processor.bank.notifyChanged();
            };
            return sl;
        }

        //── Output offset [-1, 1] ────────────────────────────────────────
        case ColOffset:
        {
            auto* sl = dynamic_cast<juce::Slider*> (existing);
            if (sl == nullptr)
            {
                sl = new juce::Slider();
                sl->setRange (-1.0, 1.0, 0.01);
                sl->setSliderStyle (juce::Slider::LinearHorizontal);
                sl->setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 20);
            }
            {
                juce::ScopedReadLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    sl->setValue (processor.bank.mappings[row].outputOffset,
                                  juce::dontSendNotification);
            }
            sl->onValueChange = [this, sl, row] {
                juce::ScopedWriteLock lock (processor.bankLock);
                if (row < (int) processor.bank.mappings.size())
                    processor.bank.mappings[row].outputOffset = (float) sl->getValue();
                processor.bank.notifyChanged();
            };
            return sl;
        }

        default:
            delete existing;
            return nullptr;
    }
}

//==============================================================================
bool KeyMapperComponent::isShadowed (int row) const
{
    juce::ScopedReadLock lock (processor.bankLock);
    if (row <= 0 || row >= (int) processor.bank.mappings.size()) return false;
    int note = processor.bank.mappings[row].midiNote;
    for (int i = 0; i < row; ++i)
        if (processor.bank.mappings[i].midiNote == note) return true;
    return false;
}

void KeyMapperComponent::addMapping()
{
    juce::ScopedWriteLock lock (processor.bankLock);
    if (!processor.bank.mappings.empty())
    {
        KeyMapping m = processor.bank.mappings.back();
        m.midiNote = juce::jlimit (0, 127, m.midiNote + 1);
        processor.bank.mappings.push_back (m);
    }
    else
    {
        processor.bank.mappings.emplace_back();
    }
    processor.bank.notifyChanged();
    table.updateContent();
}

void KeyMapperComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    table.updateContent();
    table.repaint();
}
