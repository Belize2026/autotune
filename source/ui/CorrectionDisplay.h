#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "HardTuneLookAndFeel.h"

// Live note-track view of what the tuner is doing: the target note drawn as
// bold stepped teal blocks (the steps ARE the effect), the raw voice as a
// thin pink line being pulled onto them, over a faint semitone grid, with
// the current note named big on the right.
class CorrectionDisplay : public juce::Component
{
public:
    CorrectionDisplay()
    {
        detected.assign (capacity, inactive);
        target.assign (capacity, inactive);
    }

    // Called from the editor timer with fractional MIDI note numbers.
    void push (float detectedMidi, float targetMidi, bool active)
    {
        detected[(size_t) head] = active ? detectedMidi : inactive;
        target[(size_t) head]   = active ? targetMidi : inactive;
        head = (head + 1) % capacity;
        repaint();
    }

    void setStatusText (const juce::String& text) { statusText = text; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (theme::panel);
        g.fillRoundedRectangle (bounds, 12.0f);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (bounds, 12.0f, 2.0f);

        auto plot = bounds.reduced (14.0f, 12.0f);
        auto caption = plot.removeFromTop (18.0f);

        // Centre the view on the most recent target note.
        float centreMidi = 0.0f;
        bool haveNote = false;
        for (int i = capacity - 1; i >= 0 && ! haveNote; --i)
        {
            const float v = target[(size_t) ((head + i) % capacity)];
            if (v < inactive * 0.5f)
            {
                centreMidi = std::round (v);
                haveNote = true;
            }
        }

        // Faint semitone grid.
        auto yFor = [&plot, centreMidi] (float midi)
        {
            const float clamped = juce::jlimit (centreMidi - halfRange, centreMidi + halfRange, midi);
            return plot.getCentreY() - (clamped - centreMidi) * (plot.getHeight() * 0.5f / halfRange);
        };

        if (haveNote)
        {
            for (int s = -(int) halfRange; s <= (int) halfRange; ++s)
            {
                g.setColour (theme::textDim.withAlpha (s == 0 ? 0.25f : 0.08f));
                const float gy = yFor (centreMidi + (float) s);
                g.drawHorizontalLine ((int) gy, plot.getX(), plot.getRight());
            }
        }

        // Build both traces oldest -> newest, gaps where inactive. The
        // target is stepped: hold the previous level, then jump.
        juce::Path rawPath, notePath;
        bool rawPen = false, notePen = false;
        float prevNoteY = 0.0f;

        for (int i = 0; i < capacity; ++i)
        {
            const int idx = (head + i) % capacity;
            const float px = plot.getX() + plot.getWidth() * (float) i / (float) (capacity - 1);

            const float rawV = detected[(size_t) idx];
            if (rawV < inactive * 0.5f)
            {
                const float ry = yFor (rawV);
                if (rawPen) rawPath.lineTo (px, ry);
                else        rawPath.startNewSubPath (px, ry);
                rawPen = true;
            }
            else
                rawPen = false;

            const float noteV = target[(size_t) idx];
            if (noteV < inactive * 0.5f)
            {
                const float ny = yFor (noteV);
                if (notePen)
                {
                    notePath.lineTo (px, prevNoteY); // hold the step...
                    notePath.lineTo (px, ny);        // ...then jump
                }
                else
                    notePath.startNewSubPath (px, ny);
                notePen = true;
                prevNoteY = ny;
            }
            else
                notePen = false;
        }

        g.setColour (theme::pink.withAlpha (0.85f));
        g.strokePath (rawPath, { 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

        g.setColour (theme::teal.withAlpha (0.35f));
        g.strokePath (notePath, { 7.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt });
        g.setColour (theme::teal);
        g.strokePath (notePath, { 3.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt });

        // Caption, legend and the big current-note readout.
        g.setColour (theme::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawText ("LIVE TUNING", caption, juce::Justification::centredLeft);

        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
        g.setColour (theme::teal);
        g.drawText ("NOTE", caption.withTrimmedRight (60.0f), juce::Justification::centredRight);
        g.setColour (theme::pink);
        g.drawText ("VOICE", caption.withTrimmedRight (100.0f), juce::Justification::centredRight);

        g.setColour (haveNote ? theme::teal : theme::textDim);
        g.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
        g.drawText (statusText, caption, juce::Justification::centredRight);
    }

private:
    static constexpr int capacity = 240;     // ~8 s at the 30 Hz UI timer
    static constexpr float halfRange = 7.0f; // semitones shown above/below centre
    static constexpr float inactive = 1.0e6f;

    std::vector<float> detected, target;
    int head = 0;
    juce::String statusText;
};
