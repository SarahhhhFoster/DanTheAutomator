#include "EnvelopeEditorComponent.h"
#include "PluginProcessor.h"
#include "MonokaiLookAndFeel.h"

//==============================================================================
// Inner component that owns the curve canvas and forwards mouse events up.
class EnvelopeEditorComponent::CurveCanvas : public juce::Component
{
public:
    explicit CurveCanvas (EnvelopeEditorComponent& owner) : owner (owner) {}

    void paint (juce::Graphics& g) override
    {
        // Filled background is handled by the parent; just ask it to paint
        // the curve-specific content in our bounds.
        auto area = getLocalBounds().toFloat().reduced (4);
        g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));
        owner.paintGrid        (g, area);
        owner.paintCurve       (g, area);
        owner.paintHandles     (g, area);
        owner.paintPlaybackBars(g, area);
    }

    void mouseDown   (const juce::MouseEvent& e) override { owner.canvasMouseDown   (e); }
    void mouseDrag   (const juce::MouseEvent& e) override { owner.canvasMouseDrag   (e); }
    void mouseUp     (const juce::MouseEvent& e) override { owner.canvasMouseUp     (e); }
    void mouseDoubleClick (const juce::MouseEvent& e) override { owner.canvasDoubleClick (e); }

private:
    EnvelopeEditorComponent& owner;
};

//==============================================================================
EnvelopeEditorComponent::EnvelopeEditorComponent (MidiEnvelopeProcessor& proc)
    : processor (proc)
{
    addAndMakeVisible (envList);
    envList.setModel (this);
    envList.setColour (juce::ListBox::backgroundColourId, juce::Colour (MonokaiLookAndFeel::Bg));
    envList.setColour (juce::ListBox::outlineColourId,    juce::Colour (MonokaiLookAndFeel::Border));
    envList.setRowHeight (24);

    addAndMakeVisible (addEnvBtn);
    addEnvBtn.onClick = [this] {
        juce::ScopedWriteLock lock (processor.bankLock);
        Envelope e;
        e.name = "Envelope " + juce::String (processor.bank.envelopes.size() + 1);
        processor.bank.envelopes.push_back (e);
        processor.bank.notifyChanged();
        envList.updateContent();
        envList.selectRow ((int) processor.bank.envelopes.size() - 1);
    };

    canvas = std::make_unique<CurveCanvas> (*this);
    addAndMakeVisible (*canvas);

    addAndMakeVisible (lengthLabel);

    lengthSlider.setRange (1.0, 64.0, 0.5);
    lengthSlider.setValue (4.0, juce::dontSendNotification);
    lengthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    lengthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    lengthSlider.onValueChange = [this] {
        float newLen = (float) lengthSlider.getValue();
        float oldLen = prevLength;

        // Proportionally scale all anchor & handle times to fit the new length
        if (oldLen > 0.0f && std::abs (newLen - oldLen) > 0.001f)
        {
            float scale = newLen / oldLen;
            for (auto& a : workingEnv.anchors)
            {
                a.time       *= scale;
                a.cpOutTime  *= scale;
                a.cpInTime   *= scale;
            }
            workingEnv.enforceMonotoneCps();
        }

        workingEnv.lengthInBeats = newLen;
        prevLength = newLen;

        {
            juce::ScopedWriteLock lock (processor.bankLock);
            if (selectedEnvIdx < (int) processor.bank.envelopes.size())
                processor.bank.envelopes[selectedEnvIdx] = workingEnv;
            processor.bank.notifyChanged();
        }
        canvas->repaint();
    };
    addAndMakeVisible (lengthSlider);

    loopToggle.onStateChange = [this] {
        juce::ScopedWriteLock lock (processor.bankLock);
        if (selectedEnvIdx < (int) processor.bank.envelopes.size())
        {
            processor.bank.envelopes[selectedEnvIdx].looping =
                loopToggle.getToggleState();
            processor.bank.notifyChanged();
        }
    };
    addAndMakeVisible (loopToggle);

    processor.bank.addChangeListener (this);
    loadWorkingEnv();
    startTimerHz (30);
}

EnvelopeEditorComponent::~EnvelopeEditorComponent()
{
    processor.bank.removeChangeListener (this);
}

//==============================================================================
void EnvelopeEditorComponent::resized()
{
    auto bounds = getLocalBounds();

    // Left: list panel
    auto listPanel = bounds.removeFromLeft (150);
    addEnvBtn.setBounds (listPanel.removeFromBottom (28).reduced (2));
    envList.setBounds (listPanel);

    // Bottom toolbar
    auto toolbar = bounds.removeFromBottom (30);
    lengthLabel  .setBounds (toolbar.removeFromLeft (110));
    lengthSlider .setBounds (toolbar.removeFromLeft (160));
    loopToggle   .setBounds (toolbar.removeFromLeft (60));

    canvas->setBounds (bounds.reduced (2));
}

void EnvelopeEditorComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MonokaiLookAndFeel::Bg));
}

//==============================================================================
// ListBoxModel
int EnvelopeEditorComponent::getNumRows()
{
    juce::ScopedReadLock lock (processor.bankLock);
    return (int) processor.bank.envelopes.size();
}

void EnvelopeEditorComponent::paintListBoxItem (int row, juce::Graphics& g,
                                                  int w, int h, bool selected)
{
    if (selected)
        g.fillAll (juce::Colour (MonokaiLookAndFeel::Surface));

    juce::String name;
    {
        juce::ScopedReadLock lock (processor.bankLock);
        if (row < (int) processor.bank.envelopes.size())
            name = processor.bank.envelopes[row].name;
    }
    g.setColour (selected ? juce::Colour (MonokaiLookAndFeel::Yellow)
                           : juce::Colour (MonokaiLookAndFeel::Fg));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));
    g.drawText (name, 6, 0, w - 6, h, juce::Justification::centredLeft);
}

void EnvelopeEditorComponent::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    selectedEnvIdx = row;
    loadWorkingEnv();
    canvas->repaint();
}

//==============================================================================
void EnvelopeEditorComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    envList.updateContent();
    loadWorkingEnv();
    canvas->repaint();
}

//==============================================================================
void EnvelopeEditorComponent::loadWorkingEnv()
{
    juce::ScopedReadLock lock (processor.bankLock);
    if (selectedEnvIdx < (int) processor.bank.envelopes.size())
    {
        workingEnv = processor.bank.envelopes[selectedEnvIdx];
        prevLength = workingEnv.lengthInBeats;
        lengthSlider.setValue (workingEnv.lengthInBeats, juce::dontSendNotification);
        loopToggle.setToggleState (workingEnv.looping, juce::dontSendNotification);
    }
}

void EnvelopeEditorComponent::commitEnvelopeChange()
{
    juce::ScopedWriteLock lock (processor.bankLock);
    if (selectedEnvIdx < (int) processor.bank.envelopes.size())
        processor.bank.envelopes[selectedEnvIdx] = workingEnv;
    processor.bank.notifyChanged();
}

//==============================================================================
// Coordinate helpers
float EnvelopeEditorComponent::timeToX (float time, juce::Rectangle<float> area) const
{
    float len = workingEnv.lengthInBeats;
    if (len <= 0.0f) len = 1.0f;
    return area.getX() + (time / len) * area.getWidth();
}

float EnvelopeEditorComponent::valueToY (float value, juce::Rectangle<float> area) const
{
    // Bipolar [-1, 1]: value 1 = top, 0 = centre, -1 = bottom
    return area.getCentreY() - value * (area.getHeight() * 0.5f);
}

float EnvelopeEditorComponent::xToTime (float x, juce::Rectangle<float> area) const
{
    float len = workingEnv.lengthInBeats;
    if (len <= 0.0f) len = 1.0f;
    return juce::jlimit (0.0f, len, ((x - area.getX()) / area.getWidth()) * len);
}

float EnvelopeEditorComponent::yToValue (float y, juce::Rectangle<float> area) const
{
    return juce::jlimit (-1.0f, 1.0f,
                         (area.getCentreY() - y) / (area.getHeight() * 0.5f));
}

//==============================================================================
// Painting
void EnvelopeEditorComponent::paintGrid (juce::Graphics& g, juce::Rectangle<float> area) const
{
    // Beat lines
    float len   = workingEnv.lengthInBeats;
    int   beats = juce::roundToInt (len);

    g.setColour (juce::Colour (MonokaiLookAndFeel::Surface));
    for (int b = 0; b <= beats; ++b)
    {
        float x = timeToX ((float) b, area);
        g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
    }
    // Horizontal grid at -1, -0.5, 0, 0.5, 1
    for (int v = -2; v <= 2; ++v)
    {
        float y = valueToY (v * 0.5f, area);
        // Emphasise the zero line
        g.setColour (v == 0 ? juce::Colour (MonokaiLookAndFeel::Dim)
                            : juce::Colour (MonokaiLookAndFeel::Surface));
        g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());
    }

    // Axis labels
    g.setColour (juce::Colour (MonokaiLookAndFeel::Border));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f)));
    for (int b = 0; b <= beats; ++b)
    {
        float x = timeToX ((float) b, area);
        g.drawText (juce::String (b),
                    juce::roundToInt (x) - 10,
                    juce::roundToInt (area.getBottom()) + 1, 20, 12,
                    juce::Justification::centred);
    }
}

void EnvelopeEditorComponent::paintCurve (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto& anchors = workingEnv.anchors;
    if (anchors.size() < 2) return;

    juce::Path path;
    path.startNewSubPath (timeToX (anchors[0].time,  area),
                          valueToY (anchors[0].value, area));

    for (size_t i = 0; i + 1 < anchors.size(); ++i)
    {
        const auto& a0 = anchors[i];
        const auto& a1 = anchors[i + 1];

        path.cubicTo (timeToX (a0.cpOutTime,  area), valueToY (a0.cpOutValue, area),
                      timeToX (a1.cpInTime,   area), valueToY (a1.cpInValue,  area),
                      timeToX (a1.time,        area), valueToY (a1.value,       area));
    }

    g.setColour (juce::Colour (MonokaiLookAndFeel::Green));
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

void EnvelopeEditorComponent::paintHandles (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto& anchors = workingEnv.anchors;

    for (int i = 0; i < (int) anchors.size(); ++i)
    {
        const auto& a = anchors[i];
        float ax = timeToX (a.time,  area);
        float ay = valueToY (a.value, area);

        // Draw control handles for cpOut (next segment) and cpIn (prev segment)
        auto drawHandle = [&](float hx, float hy, bool isOut)
        {
            g.setColour (juce::Colour (MonokaiLookAndFeel::Border));
            g.drawLine (ax, ay, hx, hy, 1.0f);
            juce::Colour col = isOut ? juce::Colour (MonokaiLookAndFeel::Orange)
                                     : juce::Colour (MonokaiLookAndFeel::Purple);
            g.setColour (col);
            g.drawRect (juce::Rectangle<float> (hx - 4, hy - 4, 8, 8), 1.5f);
        };

        if (i + 1 < (int) anchors.size())
            drawHandle (timeToX (a.cpOutTime, area), valueToY (a.cpOutValue, area), true);
        if (i > 0)
            drawHandle (timeToX (a.cpInTime, area), valueToY (a.cpInValue, area), false);

        // Anchor point
        juce::Colour anchorCol = (i == 0 || i == (int) anchors.size() - 1)
                                  ? juce::Colour (MonokaiLookAndFeel::Red)
                                  : juce::Colour (MonokaiLookAndFeel::Cyan);
        g.setColour (anchorCol);
        g.fillEllipse (ax - 6, ay - 6, 12, 12);
        g.setColour (juce::Colour (MonokaiLookAndFeel::Bg));
        g.drawEllipse (ax - 6, ay - 6, 12, 12, 1.5f);
    }
}

//==============================================================================
// Hit testing
EnvelopeEditorComponent::Hit
EnvelopeEditorComponent::hitTest (juce::Point<float> pos, juce::Rectangle<float> area) const
{
    const auto& anchors = workingEnv.anchors;
    const float kRadius = 8.0f;

    // Check handles first (smaller targets, higher priority)
    for (int i = 0; i < (int) anchors.size(); ++i)
    {
        const auto& a = anchors[i];

        if (i + 1 < (int) anchors.size())
        {
            juce::Point<float> hp { timeToX (a.cpOutTime, area), valueToY (a.cpOutValue, area) };
            if (pos.getDistanceFrom (hp) <= kRadius)
                return { HitType::CpOut, i };
        }
        if (i > 0)
        {
            juce::Point<float> hp { timeToX (a.cpInTime, area), valueToY (a.cpInValue, area) };
            if (pos.getDistanceFrom (hp) <= kRadius)
                return { HitType::CpIn, i };
        }
    }
    // Then anchors
    for (int i = 0; i < (int) anchors.size(); ++i)
    {
        const auto& a = anchors[i];
        juce::Point<float> ap { timeToX (a.time, area), valueToY (a.value, area) };
        if (pos.getDistanceFrom (ap) <= kRadius)
            return { HitType::Anchor, i };
    }
    return {};
}

//==============================================================================
// Mouse interactions (forwarded from CurveCanvas)
void EnvelopeEditorComponent::canvasMouseDown (const juce::MouseEvent& e)
{
    auto area = canvas->getLocalBounds().toFloat().reduced (4);
    dragHit   = hitTest (e.position, area);
    dragging  = (dragHit.type != HitType::None);
    dragStart = e.position;
}

void EnvelopeEditorComponent::canvasMouseDrag (const juce::MouseEvent& e)
{
    if (!dragging || dragHit.type == HitType::None) return;

    auto  area = canvas->getLocalBounds().toFloat().reduced (4);
    float t    = xToTime  (e.position.x, area);
    float v    = yToValue (e.position.y, area);
    int   idx  = dragHit.anchorIdx;
    auto& anchors = workingEnv.anchors;

    switch (dragHit.type)
    {
        case HitType::Anchor:
        {
            // Clamp time between neighbours
            float minT = (idx > 0)                        ? anchors[idx-1].time : 0.0f;
            float maxT = (idx + 1 < (int) anchors.size()) ? anchors[idx+1].time
                                                           : workingEnv.lengthInBeats;
            float oldT = anchors[idx].time;
            float oldV = anchors[idx].value;
            anchors[idx].time  = juce::jlimit (minT, maxT, t);
            anchors[idx].value = v;
            // Move control handles by the same delta (computed after clamping)
            float dt = anchors[idx].time  - oldT;
            float dv = anchors[idx].value - oldV;
            anchors[idx].cpOutTime  += dt; anchors[idx].cpOutValue += dv;
            anchors[idx].cpInTime   += dt; anchors[idx].cpInValue  += dv;
            break;
        }
        case HitType::CpOut:
            anchors[idx].cpOutTime  = t;
            anchors[idx].cpOutValue = v;
            break;
        case HitType::CpIn:
            anchors[idx].cpInTime  = t;
            anchors[idx].cpInValue = v;
            break;
        default: break;
    }

    workingEnv.enforceMonotoneCps();
    canvas->repaint();
}

void EnvelopeEditorComponent::canvasMouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        dragging = false;
        workingEnv.enforceMonotoneCps();
        commitEnvelopeChange();
    }
}

void EnvelopeEditorComponent::canvasDoubleClick (const juce::MouseEvent& e)
{
    auto area = canvas->getLocalBounds().toFloat().reduced (4);
    auto hit  = hitTest (e.position, area);

    if (hit.type == HitType::Anchor)
    {
        // Delete anchor (not first or last)
        int idx = hit.anchorIdx;
        if (idx > 0 && idx < (int) workingEnv.anchors.size() - 1)
        {
            workingEnv.anchors.erase (workingEnv.anchors.begin() + idx);
            commitEnvelopeChange();
            canvas->repaint();
        }
    }
    else if (hit.type == HitType::None)
    {
        float t = xToTime  (e.position.x, area);
        float v = yToValue (e.position.y, area);
        addAnchorAt (t, v);
    }
}

void EnvelopeEditorComponent::addAnchorAt (float time, float value)
{
    auto& anchors = workingEnv.anchors;

    // Find insertion index
    int insertIdx = (int) anchors.size();
    for (int i = 0; i < (int) anchors.size(); ++i)
        if (anchors[i].time > time) { insertIdx = i; break; }

    AnchorPoint p;
    p.time = time; p.value = value;
    // Default handles: offset by 0.5 beats horizontally
    p.cpOutTime  = time + 0.5f; p.cpOutValue = value;
    p.cpInTime   = time - 0.5f; p.cpInValue  = value;

    anchors.insert (anchors.begin() + insertIdx, p);
    workingEnv.enforceMonotoneCps();
    commitEnvelopeChange();
    canvas->repaint();
}

//==============================================================================
void EnvelopeEditorComponent::timerCallback()
{
    {
        juce::ScopedLock sl (processor.playbackLock);
        displayPositions = processor.playbackSnapshot;
    }
    canvas->repaint();
}

void EnvelopeEditorComponent::paintPlaybackBars (juce::Graphics& g,
                                                   juce::Rectangle<float> area) const
{
    g.setColour (juce::Colour (MonokaiLookAndFeel::Yellow).withAlpha (0.85f));

    for (const auto& entry : displayPositions)
    {
        if (entry.envIdx != selectedEnvIdx) continue;

        float pos = entry.posBeats;
        if (pos < 0.0f || pos > workingEnv.lengthInBeats) continue;

        float x = timeToX (pos, area);
        g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());

        // Small diamond marker at the curve's value
        float v    = workingEnv.evaluate (pos);
        float cy   = valueToY (v, area);
        juce::Path diamond;
        diamond.addPolygon ({ x, cy }, 4, 5.0f, juce::MathConstants<float>::pi * 0.25f);
        g.fillPath (diamond);
    }
}
