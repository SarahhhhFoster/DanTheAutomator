#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>

//==============================================================================
/// Factory for icon Drawables loaded from embedded SVG binary resources.
///
/// Usage:
///   auto icon = Icons::make (Icons::Add);
///   icon->drawWithin (g, bounds, juce::RectanglePlacement::centred, 1.0f);
///
///   // Or pass to DrawableButton:
///   btn.setImages (icon.get());
struct Icons
{
    enum Type { Add, Loop, Envelope, KeyMap, Scope, Sweep, MidiOut };

    static std::unique_ptr<juce::Drawable> make (Type t)
    {
        switch (t)
        {
            case Add:      return fromSVG (BinaryData::icon_add_svg,
                                           BinaryData::icon_add_svgSize);
            case Loop:     return fromSVG (BinaryData::icon_loop_svg,
                                           BinaryData::icon_loop_svgSize);
            case Envelope: return fromSVG (BinaryData::icon_envelope_svg,
                                           BinaryData::icon_envelope_svgSize);
            case KeyMap:   return fromSVG (BinaryData::icon_keymap_svg,
                                           BinaryData::icon_keymap_svgSize);
            case Scope:    return fromSVG (BinaryData::icon_scope_svg,
                                           BinaryData::icon_scope_svgSize);
            case Sweep:    return fromSVG (BinaryData::icon_sweep_svg,
                                           BinaryData::icon_sweep_svgSize);
            case MidiOut:  return fromSVG (BinaryData::icon_midiout_svg,
                                           BinaryData::icon_midiout_svgSize);
            default:       return nullptr;
        }
    }

    /// Paint a standard panel header bar (icon on left, title text beside it).
    /// Call from paint() after painting the component background.
    static void paintHeader (juce::Graphics& g, int componentWidth, int headerH,
                             const juce::String& title, juce::Drawable* icon)
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                              (float) componentWidth,
                                              (float) headerH);
        g.setColour (juce::Colour (0xff3e3d32)); // Surface
        g.fillRect (bounds);
        g.setColour (juce::Colour (0xff75715e)); // Border
        g.drawHorizontalLine (headerH - 1, 0.0f, (float) componentWidth);

        if (icon != nullptr)
        {
            auto iconR = bounds.removeFromLeft ((float) headerH).reduced (4.0f);
            icon->drawWithin (g, iconR, juce::RectanglePlacement::centred, 1.0f);
        }

        g.setColour (juce::Colour (0xffe6db74)); // Yellow
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
        g.drawText (title, bounds.reduced (2.0f, 0.0f),
                    juce::Justification::centredLeft);
    }

private:
    static std::unique_ptr<juce::Drawable> fromSVG (const char* data, int size)
    {
        if (auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (data, size)))
            return juce::Drawable::createFromSVG (*xml);
        return nullptr;
    }
};
