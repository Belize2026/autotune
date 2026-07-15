#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "HardTuneLookAndFeel.h"

// Antares-style correction meter: a green -> red scale showing how much the
// tuner had to pull the voice. Green = the note was basically in tune,
// red = the original note was way off. The current target note is named
// large on the left.
class CorrectionDisplay : public juce::Component
{
public:
    // Called from the editor timer with the correction being applied, in
    // cents. Fast rise, smooth fall so the meter is readable.
    void update (float correctionCents, bool active)
    {
        if (active)
        {
            const float v = juce::jlimit (0.0f, maxCents, std::abs (correctionCents));
            meterValue = v > meterValue ? v : meterValue * fallRate;
        }
        else
        {
            meterValue *= fallRate;
        }
        hasSignal = active;
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

        auto area = bounds.reduced (16.0f, 12.0f);
        auto caption = area.removeFromTop (18.0f);

        g.setColour (theme::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawText ("PITCH CORRECTION", caption, juce::Justification::centredLeft);

        if (hasSignal)
        {
            g.setColour (theme::text);
            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.drawText (juce::String ((int) std::lround (meterValue)) + " ct",
                        caption, juce::Justification::centredRight);
        }

        // Big current-note name on the left.
        auto noteArea = area.removeFromLeft (96.0f);
        g.setColour (hasSignal ? theme::teal : theme::textDim);
        g.setFont (juce::Font (juce::FontOptions (hasSignal ? 40.0f : 16.0f, juce::Font::bold)));
        g.drawText (statusText, noteArea, juce::Justification::centred);

        // The meter: green (barely correcting) -> amber -> red (way off).
        area.removeFromLeft (10.0f);
        auto labels = area.removeFromBottom (16.0f);
        auto track = area.withSizeKeepingCentre (area.getWidth(), 30.0f);

        const juce::Colour green (0xff27c46f), amber (0xfff5b840), red (0xffe5484d);
        juce::ColourGradient gradient (green, track.getX(), 0.0f,
                                       red, track.getRight(), 0.0f, false);
        gradient.addColour (0.5, amber);

        // Whole scale faint, lit up to the current correction amount.
        g.setGradientFill (gradient);
        g.setOpacity (0.22f);
        g.fillRoundedRectangle (track, track.getHeight() * 0.5f);

        const float fraction = juce::jlimit (0.0f, 1.0f, meterValue / maxCents);
        if (fraction > 0.01f)
        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path clip;
            clip.addRoundedRectangle (track, track.getHeight() * 0.5f);
            g.reduceClipRegion (clip);
            g.setGradientFill (gradient);
            g.setOpacity (1.0f);
            g.fillRect (track.withWidth (track.getWidth() * fraction));
        }

        g.setOpacity (1.0f);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (track, track.getHeight() * 0.5f, 1.6f);

        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
        g.setColour (green);
        g.drawText ("IN TUNE", labels, juce::Justification::centredLeft);
        g.setColour (red);
        g.drawText ("WAY OFF", labels, juce::Justification::centredRight);
    }

private:
    static constexpr float maxCents = 100.0f; // full-scale = a semitone out
    static constexpr float fallRate = 0.92f;  // smooth release at 30 Hz

    float meterValue = 0.0f;
    bool hasSignal = false;
    juce::String statusText;
};
