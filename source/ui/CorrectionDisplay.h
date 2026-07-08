#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "HardTuneLookAndFeel.h"

// Live view of what the tuner is doing: a scrolling trace of the correction
// being applied (in cents; the centre line means "already on pitch") with
// the current detected -> target note readout.
class CorrectionDisplay : public juce::Component
{
public:
    CorrectionDisplay()
    {
        history.assign (capacity, inactive);
    }

    // Called from the editor timer. `cents` is the correction the tuner is
    // applying right now; active=false leaves a gap (silence / bypass).
    void push (float cents, bool active)
    {
        history[(size_t) head] = active ? juce::jlimit (-maxCents, maxCents, cents) : inactive;
        head = (head + 1) % capacity;
        repaint();
    }

    void setReadout (const juce::String& text)
    {
        readout = text;
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (theme::panel);
        g.fillRoundedRectangle (bounds, 10.0f);

        auto plot = bounds.reduced (14.0f, 12.0f);
        plot.removeFromTop (16.0f); // caption strip
        const float midY = plot.getCentreY();

        // Reference lines: centre = in tune, faint lines at +/- 50 cents.
        g.setColour (theme::textDim.withAlpha (0.35f));
        g.drawHorizontalLine ((int) midY, plot.getX(), plot.getRight());
        g.setColour (theme::textDim.withAlpha (0.12f));
        for (float refCents : { 50.0f, -50.0f })
        {
            const float ry = midY - (refCents / maxCents) * (plot.getHeight() * 0.5f);
            g.drawHorizontalLine ((int) ry, plot.getX(), plot.getRight());
        }

        // The correction trace, oldest to newest, gaps where inactive.
        juce::Path trace;
        bool penDown = false;
        float lastX = 0.0f, lastY = midY;
        bool current = false;

        for (int i = 0; i < capacity; ++i)
        {
            const float value = history[(size_t) ((head + i) % capacity)];
            const float px = plot.getX() + plot.getWidth() * (float) i / (float) (capacity - 1);

            if (value > maxCents * 2.0f) // the inactive sentinel
            {
                penDown = false;
                current = false;
                continue;
            }

            const float py = midY - (value / maxCents) * (plot.getHeight() * 0.5f);
            if (penDown)
                trace.lineTo (px, py);
            else
                trace.startNewSubPath (px, py);
            penDown = true;
            lastX = px;
            lastY = py;
            current = true;
        }

        g.setColour (theme::accent.withAlpha (0.25f));
        g.strokePath (trace, { 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        g.setColour (theme::accent);
        g.strokePath (trace, { 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

        if (current)
        {
            g.setColour (juce::Colours::white);
            g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ lastX, lastY }));
        }

        // Caption and live note readout.
        auto caption = bounds.reduced (14.0f, 10.0f).removeFromTop (16.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawText ("PITCH CORRECTION", caption, juce::Justification::centredLeft);

        g.setColour (theme::text);
        g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        g.drawText (readout, caption, juce::Justification::centredRight);
    }

private:
    static constexpr int capacity = 240;      // ~8 s at the 30 Hz UI timer
    static constexpr float maxCents = 120.0f; // vertical range of the plot
    static constexpr float inactive = 1.0e6f;

    std::vector<float> history;
    int head = 0;
    juce::String readout;
};
