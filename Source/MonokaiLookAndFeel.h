#pragma once
#include <JuceHeader.h>

//==============================================================================
/// Flat Monokai-themed LookAndFeel.
/// All colors taken from the canonical Monokai palette.
class MonokaiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // ── Palette ──────────────────────────────────────────────────────────────
    static constexpr uint32_t Bg      = 0xff272822;
    static constexpr uint32_t Surface = 0xff3e3d32;
    static constexpr uint32_t Dim     = 0xff49483e;
    static constexpr uint32_t Border  = 0xff75715e;
    static constexpr uint32_t Fg      = 0xfff8f8f2;
    static constexpr uint32_t Red     = 0xfff92672;
    static constexpr uint32_t Orange  = 0xfffd971f;
    static constexpr uint32_t Yellow  = 0xffe6db74;
    static constexpr uint32_t Green   = 0xffa6e22e;
    static constexpr uint32_t Cyan    = 0xff66d9e8;
    static constexpr uint32_t Purple  = 0xffae81ff;

    MonokaiLookAndFeel()
    {
        using namespace juce;

        // ── Global background ─────────────────────────────────────────────
        setColour (ResizableWindow::backgroundColourId, Colour (Bg));

        // ── Labels ─────────────────────────────────────────────────────────
        setColour (Label::backgroundColourId, Colour (0x00000000));
        setColour (Label::textColourId,       Colour (Fg));

        // ── Buttons ────────────────────────────────────────────────────────
        setColour (TextButton::buttonColourId,   Colour (Surface));
        setColour (TextButton::buttonOnColourId,  Colour (Dim));
        setColour (TextButton::textColourOffId,  Colour (Green));
        setColour (TextButton::textColourOnId,   Colour (Yellow));

        // ── Toggles ────────────────────────────────────────────────────────
        setColour (ToggleButton::textColourId,         Colour (Fg));
        setColour (ToggleButton::tickColourId,         Colour (Green));
        setColour (ToggleButton::tickDisabledColourId, Colour (Border));

        // ── Sliders ────────────────────────────────────────────────────────
        setColour (Slider::backgroundColourId,          Colour (Surface));
        setColour (Slider::trackColourId,               Colour (Cyan));
        setColour (Slider::thumbColourId,               Colour (Orange));
        setColour (Slider::textBoxTextColourId,         Colour (Fg));
        setColour (Slider::textBoxBackgroundColourId,   Colour (Surface));
        setColour (Slider::textBoxHighlightColourId,    Colour (Dim));
        setColour (Slider::textBoxOutlineColourId,      Colour (Border));

        // ── ComboBox ───────────────────────────────────────────────────────
        setColour (ComboBox::backgroundColourId, Colour (Surface));
        setColour (ComboBox::textColourId,       Colour (Fg));
        setColour (ComboBox::buttonColourId,     Colour (Dim));
        setColour (ComboBox::outlineColourId,    Colour (Border));
        setColour (ComboBox::arrowColourId,      Colour (Fg));
        setColour (ComboBox::focusedOutlineColourId, Colour (Cyan));

        // ── Popup menus ────────────────────────────────────────────────────
        setColour (PopupMenu::backgroundColourId,            Colour (Surface));
        setColour (PopupMenu::textColourId,                  Colour (Fg));
        setColour (PopupMenu::headerTextColourId,            Colour (Yellow));
        setColour (PopupMenu::highlightedTextColourId,       Colour (Bg));
        setColour (PopupMenu::highlightedBackgroundColourId, Colour (Green));

        // ── ListBox ────────────────────────────────────────────────────────
        setColour (ListBox::backgroundColourId, Colour (Bg));
        setColour (ListBox::outlineColourId,    Colour (Border));
        setColour (ListBox::textColourId,       Colour (Fg));

        // ── Table ──────────────────────────────────────────────────────────
        setColour (TableListBox::backgroundColourId, Colour (Bg));
        setColour (TableListBox::outlineColourId,    Colour (Border));
        setColour (TableHeaderComponent::backgroundColourId, Colour (Dim));
        setColour (TableHeaderComponent::textColourId,       Colour (Yellow));
        setColour (TableHeaderComponent::outlineColourId,    Colour (Border));
        setColour (TableHeaderComponent::highlightColourId,  Colour (Surface));

        // ── ScrollBar ──────────────────────────────────────────────────────
        setColour (ScrollBar::backgroundColourId, Colour (Bg));
        setColour (ScrollBar::thumbColourId,      Colour (Border));
        setColour (ScrollBar::trackColourId,      Colour (Bg));

        // ── Text editor ────────────────────────────────────────────────────
        setColour (TextEditor::backgroundColourId,         Colour (Surface));
        setColour (TextEditor::textColourId,               Colour (Fg));
        setColour (TextEditor::highlightColourId,          Colour (Dim));
        setColour (TextEditor::highlightedTextColourId,    Colour (Fg));
        setColour (TextEditor::outlineColourId,            Colour (Border));
        setColour (TextEditor::focusedOutlineColourId,     Colour (Cyan));
        setColour (TextEditor::shadowColourId,             Colour (0x00000000));
    }

    // ── Flat button ──────────────────────────────────────────────────────────
    void drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                const juce::Colour&, bool hi, bool down) override
    {
        auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
        auto fill = down ? juce::Colour (Dim).brighter (0.05f)
                         : hi ? juce::Colour (Dim) : juce::Colour (Surface);
        g.setColour (fill);
        g.fillRect (bounds);
        g.setColour (juce::Colour (Border));
        g.drawRect (bounds, 1.0f);
    }

    // ── Flat combo box ───────────────────────────────────────────────────────
    void drawComboBox (juce::Graphics& g, int w, int h, bool /*isDown*/,
                       int bx, int /*by*/, int bw, int /*bh*/,
                       juce::ComboBox& /*box*/) override
    {
        g.setColour (juce::Colour (Surface));
        g.fillRect (0, 0, w, h);
        g.setColour (juce::Colour (Border));
        g.drawRect (0, 0, w, h, 1);

        // Arrow
        float cx = bx + bw * 0.5f;
        float cy = h  * 0.5f;
        g.setColour (juce::Colour (Fg));
        juce::Path arrow;
        arrow.addTriangle (cx - 4.0f, cy - 2.0f,
                           cx + 4.0f, cy - 2.0f,
                           cx,        cy + 3.0f);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions{}.withHeight (13.0f));
    }

    // ── Flat slider ──────────────────────────────────────────────────────────
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float /*min*/, float /*max*/,
                           juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearHorizontal
            || style == juce::Slider::LinearVertical)
        {
            bool horiz = (style == juce::Slider::LinearHorizontal);
            float trackH = 4.0f;
            float tx = horiz ? x : x + (w - trackH) * 0.5f;
            float ty = horiz ? y + (h - trackH) * 0.5f : y;
            float tw = horiz ? w : trackH;
            float th = horiz ? trackH : h;

            g.setColour (juce::Colour (Surface));
            g.fillRect (tx, ty, tw, th);

            if (horiz)
            {
                g.setColour (juce::Colour (Cyan));
                g.fillRect (tx, ty, sliderPos - tx, trackH);
            }
            else
            {
                g.setColour (juce::Colour (Cyan));
                g.fillRect (tx, sliderPos, trackH, ty + th - sliderPos);
            }

            // Thumb
            float thumbW = 8.0f, thumbH = 16.0f;
            float thumbX = horiz ? sliderPos - thumbW * 0.5f : tx - 2.0f;
            float thumbY = horiz ? ty + (trackH - thumbH) * 0.5f : sliderPos - thumbW * 0.5f;
            if (!horiz) std::swap (thumbW, thumbH);
            g.setColour (juce::Colour (Orange));
            g.fillRect (thumbX, thumbY, thumbW, thumbH);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos,
                                              0.0f, 1.0f, style, slider);
        }
    }

    // ── Flat toggle ──────────────────────────────────────────────────────────
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                           bool /*hi*/, bool /*down*/) override
    {
        bool on = b.getToggleState();
        float bw = 28.0f, bh = 14.0f;
        float bx = 4.0f,  by = (b.getHeight() - bh) * 0.5f;

        g.setColour (on ? juce::Colour (Green) : juce::Colour (Surface));
        g.fillRect (bx, by, bw, bh);
        g.setColour (juce::Colour (Border));
        g.drawRect (bx, by, bw, bh, 1.0f);

        float knobX = on ? bx + bw - bh + 2.0f : bx + 2.0f;
        g.setColour (juce::Colour (Fg));
        g.fillRect (knobX, by + 2.0f, bh - 4.0f, bh - 4.0f);

        g.setColour (juce::Colour (Fg));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));
        g.drawText (b.getButtonText(),
                    juce::roundToInt (bx + bw + 6.0f), 0,
                    b.getWidth(), b.getHeight(),
                    juce::Justification::centredLeft);
    }

    // ── Scrollbar: thin flat bar ──────────────────────────────────────────────
    void drawScrollbar (juce::Graphics& g, juce::ScrollBar& /*bar*/,
                        int x, int y, int w, int h,
                        bool isVertical, int thumbStart, int thumbSize,
                        bool /*hi*/, bool /*down*/) override
    {
        g.setColour (juce::Colour (Bg));
        g.fillRect (x, y, w, h);
        g.setColour (juce::Colour (Border));
        if (thumbSize > 0)
        {
            if (isVertical)
                g.fillRect (x + 2, y + thumbStart, w - 4, thumbSize);
            else
                g.fillRect (x + thumbStart, y + 2, thumbSize, h - 4);
        }
    }

    // ── Table header ─────────────────────────────────────────────────────────
    void drawTableHeaderBackground (juce::Graphics& g,
                                    juce::TableHeaderComponent& header) override
    {
        g.setColour (juce::Colour (Dim));
        g.fillAll();
        g.setColour (juce::Colour (Border));
        g.drawHorizontalLine (header.getHeight() - 1, 0.0f,
                              (float) header.getWidth());
    }

    void drawTableHeaderColumn (juce::Graphics& g,
                                juce::TableHeaderComponent& /*header*/,
                                const juce::String& colName,
                                int /*colId*/, int w, int h,
                                bool isMouseOver, bool isMouseDown,
                                int /*sortDirectionIndicator*/) override
    {
        if (isMouseDown || isMouseOver)
        {
            g.setColour (juce::Colour (Surface));
            g.fillAll();
        }
        g.setColour (juce::Colour (Yellow));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
        g.drawText (colName, 4, 0, w - 4, h, juce::Justification::centredLeft);
        g.setColour (juce::Colour (Border));
        g.drawVerticalLine (w - 1, 2.0f, (float) h - 2.0f);
    }
};
