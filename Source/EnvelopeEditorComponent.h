#pragma once
#include <JuceHeader.h>
#include "EnvelopeData.h"
#include "EnvelopePlayer.h"
#include "Icons.h"

//==============================================================================
class EnvelopeEditorComponent : public juce::Component,
                                  public juce::ListBoxModel,
                                  public juce::ChangeListener,
                                  private juce::Timer
{
public:
    explicit EnvelopeEditorComponent (class MidiEnvelopeProcessor& proc);
    ~EnvelopeEditorComponent() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

    // ListBoxModel
    int  getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g,
                           int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    // ChangeListener
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    int  getSelectedEnvIdx() const { return selectedEnvIdx; }
    void setSelectedEnvIdx (int idx);

    // Forwarded from CurveCanvas
    void canvasMouseDown   (const juce::MouseEvent& e);
    void canvasMouseDrag   (const juce::MouseEvent& e);
    void canvasMouseUp     (const juce::MouseEvent& e);
    void canvasDoubleClick (const juce::MouseEvent& e);

private:
    class CurveCanvas;

    // ── Coordinate helpers ───────────────────────────────────────────────────
    float timeToX  (float time,  juce::Rectangle<float> area) const;
    float valueToY (float value, juce::Rectangle<float> area) const;
    float xToTime  (float x,     juce::Rectangle<float> area) const;
    float yToValue (float y,     juce::Rectangle<float> area) const;

    // ── Painting ─────────────────────────────────────────────────────────────
    void paintGrid        (juce::Graphics& g, juce::Rectangle<float> area) const;
    void paintCurve       (juce::Graphics& g, juce::Rectangle<float> area) const;
    void paintHandles     (juce::Graphics& g, juce::Rectangle<float> area) const;
    void paintPlaybackBars(juce::Graphics& g, juce::Rectangle<float> area) const;

    // ── Hit testing ──────────────────────────────────────────────────────────
    enum class HitType { None, Anchor, CpOut, CpIn };
    struct Hit { HitType type = HitType::None; int anchorIdx = -1; };
    Hit hitTest (juce::Point<float> pos, juce::Rectangle<float> area) const;

    // ── Actions ──────────────────────────────────────────────────────────────
    void addAnchorAt (float time, float value);
    void commitEnvelopeChange();
    void loadWorkingEnv();
    void sweepUnusedEnvelopes();

    // ── Timer (30 Hz) for playback bar ───────────────────────────────────────
    void timerCallback() override;

    // ── Members ──────────────────────────────────────────────────────────────
    MidiEnvelopeProcessor& processor;

    static constexpr int kHeaderH = 22;

    juce::ListBox        envList      { "Envelopes", this };
    juce::DrawableButton addEnvBtn   { "addEnv",   juce::DrawableButton::ImageFitted };
    juce::DrawableButton sweepEnvBtn { "sweepEnv", juce::DrawableButton::ImageFitted };
    std::unique_ptr<CurveCanvas> canvas;

    juce::Label        lengthLabel { {}, "Length (beats):" };
    juce::Slider       lengthSlider;
    juce::ToggleButton loopToggle  { "Loop" };

    // Icons
    std::unique_ptr<juce::Drawable> headerIcon;
    std::unique_ptr<juce::Drawable> loopIcon;
    juce::Rectangle<float>          loopIconBounds;

    int  selectedEnvIdx = 0;
    Hit              dragHit;
    bool             dragging  = false;
    juce::Point<float> dragStart;

    Envelope workingEnv;
    float    prevLength = 4.0f; // used to compute scale when length changes

    // Playback bar data — copied from processor on the timer tick
    juce::Array<PlaybackEntry> displayPositions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeEditorComponent)
};
