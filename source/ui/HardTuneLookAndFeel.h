#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace theme
{
inline const juce::Colour background { 0xff121218 };
inline const juce::Colour panel      { 0xff1b1b23 };
inline const juce::Colour control    { 0xff262631 };
inline const juce::Colour accent     { 0xffff2d78 };
inline const juce::Colour text       { 0xffe9e9f0 };
inline const juce::Colour textDim    { 0x99e9e9f0 };
inline const juce::Colour outline    { 0x16ffffff };
} // namespace theme

// Flat, modern skin: arc-style dials, pill buttons, borderless combos.
class HardTuneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HardTuneLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, theme::background);
        setColour (juce::Slider::rotarySliderFillColourId, theme::accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, theme::control);
        setColour (juce::Slider::textBoxTextColourId, theme::textDim);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId, theme::control);
        setColour (juce::ComboBox::outlineColourId, theme::outline);
        setColour (juce::ComboBox::textColourId, theme::text);
        setColour (juce::ComboBox::arrowColourId, theme::textDim);
        setColour (juce::PopupMenu::backgroundColourId, theme::panel);
        setColour (juce::PopupMenu::textColourId, theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::accent.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId, theme::text);
        setColour (juce::TextButton::buttonColourId, theme::control);
        setColour (juce::TextButton::buttonOnColourId, theme::accent);
        setColour (juce::TextButton::textColourOffId, theme::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::Label::textColourId, theme::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        const float alpha = slider.isEnabled() ? 1.0f : 0.35f;
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float lineW = juce::jmax (3.0f, radius * 0.16f);
        const float arcRadius = radius - lineW * 0.5f;

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (theme::control.withMultipliedAlpha (alpha));
        g.strokePath (track, { lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

        const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        // Bipolar dials (e.g. formant) fill from the top centre outwards.
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const float fromAngle = bipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f
                                        : rotaryStartAngle;

        if (std::abs (toAngle - fromAngle) > 0.01f)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                 fromAngle, toAngle, true);
            g.setColour (theme::accent.withMultipliedAlpha (alpha));
            g.strokePath (value, { lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        }

        const auto thumb = centre.getPointOnCircumference (arcRadius - lineW * 1.4f, toAngle);
        g.setColour (theme::text.withMultipliedAlpha (alpha));
        g.fillEllipse (juce::Rectangle<float> (lineW, lineW).withCentre (thumb));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const float corner = bounds.getHeight() * 0.5f; // pill

        auto colour = backgroundColour;
        if (isDown)
            colour = colour.brighter (0.15f);
        else if (isHighlighted)
            colour = colour.brighter (0.07f);

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, corner);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (bounds, corner, 1.0f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        juce::Path chevron;
        const float cx = (float) width - 16.0f, cy = (float) height * 0.5f;
        chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
        chevron.lineTo (cx, cy + 2.5f);
        chevron.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (chevron, { 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (14.0f));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (juce::FontOptions (14.0f, juce::Font::bold));
    }
};
