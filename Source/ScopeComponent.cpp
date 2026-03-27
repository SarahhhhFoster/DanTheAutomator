#include "ScopeComponent.h"
#include "PluginProcessor.h"
#include "MonokaiLookAndFeel.h"

//==============================================================================
ScopeComponent::ScopeComponent (MidiEnvelopeProcessor& proc)
    : processor (proc)
{
    headerIcon = Icons::make (Icons::Scope);
    emptyIcon  = Icons::make (Icons::Scope);

    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&inner, false);
    viewport.setScrollBarsShown (false, true); // vertical hidden, horizontal shown
    startTimerHz (30);
}

ScopeComponent::~ScopeComponent()
{
    stopTimer();
}

void ScopeComponent::resized()
{
    viewport.setBounds (getLocalBounds().withTrimmedTop (kHeaderH));
    int innerW = juce::jmax (viewport.getWidth(),
                             (int) traces.size() * kCellW);
    inner.setSize (innerW, viewport.getMaximumVisibleHeight());
}

void ScopeComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));
    Icons::paintHeader (g, getWidth(), kHeaderH, "Scope", headerIcon.get());
}

//==============================================================================
void ScopeComponent::timerCallback()
{
    syncTraces();
    inner.repaint();
}

void ScopeComponent::syncTraces()
{
    juce::Array<ScopeEntry> snap;
    if (processor.scopeLock.tryEnter())
    {
        snap = processor.scopeSnapshot;
        processor.scopeLock.exit();
    }

    for (const auto& entry : snap)
    {
        auto* t = getOrCreate (entry.outputChannel, entry.ccNumber);
        t->push (entry.normValue);
    }
}

ScopeComponent::Trace* ScopeComponent::getOrCreate (int channel, int cc)
{
    for (auto& tp : traces)
        if (tp->channel == channel && tp->cc == cc)
            return tp.get();

    auto t    = std::make_unique<Trace>();
    t->channel = channel;
    t->cc      = cc;
    auto* raw  = t.get();
    traces.push_back (std::move (t));

    // Grow inner to fit the new cell
    int innerW = juce::jmax (viewport.getWidth(),
                             (int) traces.size() * kCellW);
    int innerH = inner.getHeight() > 0 ? inner.getHeight()
                                       : viewport.getMaximumVisibleHeight();
    inner.setSize (innerW, innerH);
    return raw;
}

//==============================================================================
void ScopeComponent::Inner::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));

    int n = (int) owner.traces.size();
    for (int i = 0; i < n; ++i)
    {
        auto cell = juce::Rectangle<float> (
            (float) i * ScopeComponent::kCellW, 0.0f,
            (float) ScopeComponent::kCellW,
            (float) getHeight());
        owner.paintCell (g, *owner.traces[(size_t) i], cell);
    }

    if (n == 0)
    {
        auto area = getLocalBounds().toFloat();
        // Icon centred, 32×32
        if (owner.emptyIcon)
        {
            auto iconR = juce::Rectangle<float> (area.getCentreX() - 16.0f,
                                                 area.getCentreY() - 24.0f,
                                                 32.0f, 32.0f);
            owner.emptyIcon->drawWithin (g, iconR,
                                         juce::RectanglePlacement::centred, 0.4f);
        }
        g.setColour (juce::Colour (MonokaiLookAndFeel::Dim));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
        g.drawText ("No active CC outputs",
                    area.withTop (area.getCentreY() + 12.0f),
                    juce::Justification::centredTop);
    }
}

//==============================================================================
void ScopeComponent::paintCell (juce::Graphics& g, const Trace& t,
                                 juce::Rectangle<float> bounds) const
{
    // Background and border
    g.setColour (juce::Colour (MonokaiLookAndFeel::Surface));
    g.fillRect (bounds.reduced (1.0f));
    g.setColour (juce::Colour (MonokaiLookAndFeel::Border));
    g.drawRect (bounds, 1.0f);

    // Label row
    auto labelArea = bounds.removeFromTop (16.0f);
    g.setColour (juce::Colour (MonokaiLookAndFeel::Cyan));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f)));
    g.drawText (t.label(), labelArea.reduced (4, 0), juce::Justification::centredLeft);

    // Centre line (0.5 = mid CC)
    float cy = bounds.getCentreY();
    g.setColour (juce::Colour (MonokaiLookAndFeel::Dim));
    g.drawHorizontalLine (juce::roundToInt (cy), bounds.getX(), bounds.getRight());

    // Waveform: oldest sample on the left, newest on the right
    const int   nPts  = Trace::kN;
    const float xStep = bounds.getWidth() / (float) (nPts - 1);

    juce::Path path;
    for (int i = 0; i < nPts; ++i)
    {
        int   age = nPts - 1 - i; // age 0 = newest (rightmost)
        float v   = t.at (age);   // [0, 1]
        float x   = bounds.getX() + i * xStep;
        float y   = bounds.getBottom() - v * bounds.getHeight();

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    g.setColour (juce::Colour (MonokaiLookAndFeel::Green));
    g.strokePath (path, juce::PathStrokeType (1.5f));
}
