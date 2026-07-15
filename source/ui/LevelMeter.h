#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "HardTuneLookAndFeel.h"

// Output level meter: green when quiet, through amber, red when loud, with
// a clip lamp that latches briefly when the output hits 0 dBFS.
class LevelMeter : public juce::Component
{
public:
    // Fed from the editor timer with the peak since the last frame.
    void update (float peak)
    {
        displayLevel = peak > displayLevel ? peak : displayLevel * 0.86f;
        if (peak >= 0.999f)
            clipHold = 24; // ~0.8 s at 30 Hz
        else if (clipHold > 0)
            --clipHold;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        auto label = bounds.removeFromLeft (44.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawText ("OUT", label, juce::Justification::centredLeft);

        auto lamp = bounds.removeFromRight (34.0f).reduced (6.0f, 6.0f);
        auto track = bounds.reduced (0.0f, 4.0f);
        const float corner = track.getHeight() * 0.5f;

        const juce::Colour green (0xff27c46f), amber (0xfff5b840), red (0xffe5484d);
        juce::ColourGradient gradient (green, track.getX(), 0.0f,
                                       red, track.getRight(), 0.0f, false);
        gradient.addColour (0.72, amber);

        g.setGradientFill (gradient);
        g.setOpacity (0.2f);
        g.fillRoundedRectangle (track, corner);

        // -60 dB .. 0 dB mapped across the bar.
        const float db = juce::Decibels::gainToDecibels (displayLevel, -60.0f);
        const float fraction = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        if (fraction > 0.01f)
        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path clip;
            clip.addRoundedRectangle (track, corner);
            g.reduceClipRegion (clip);
            g.setGradientFill (gradient);
            g.setOpacity (1.0f);
            g.fillRect (track.withWidth (track.getWidth() * fraction));
        }

        g.setOpacity (1.0f);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (track, corner, 1.6f);

        // Clip lamp.
        g.setColour (clipHold > 0 ? red : theme::control);
        g.fillEllipse (lamp);
        g.setColour (theme::outline);
        g.drawEllipse (lamp, 1.6f);
    }

private:
    float displayLevel = 0.0f;
    int clipHold = 0;
};
