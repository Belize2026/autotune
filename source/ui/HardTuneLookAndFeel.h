#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Palette lifted from the AUTISMIDOL logo: white sticker background, bold
// black outlines, metallic teal lettering with hot pink behind it.
namespace theme
{
inline const juce::Colour background { 0xfff7f6f2 };
inline const juce::Colour panel      { 0xffffffff };
inline const juce::Colour control    { 0xffe7e5df };
inline const juce::Colour teal       { 0xff2fb8a6 };
inline const juce::Colour tealDark   { 0xff1e8577 };
inline const juce::Colour pink       { 0xffff1fa3 };
inline const juce::Colour text       { 0xff17171a };
inline const juce::Colour textDim    { 0x9917171a };
inline const juce::Colour outline    { 0xff17171a };
} // namespace theme

// Cartoon-sticker skin to match the logo: flat white surfaces, thick black
// outlines, teal fills with pink highlights.
class HardTuneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HardTuneLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, theme::background);
        setColour (juce::Slider::rotarySliderFillColourId, theme::teal);
        setColour (juce::Slider::rotarySliderOutlineColourId, theme::control);
        setColour (juce::Slider::textBoxTextColourId, theme::text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, theme::teal.withAlpha (0.35f));
        setColour (juce::ComboBox::backgroundColourId, theme::panel);
        setColour (juce::ComboBox::outlineColourId, theme::outline);
        setColour (juce::ComboBox::textColourId, theme::text);
        setColour (juce::ComboBox::arrowColourId, theme::text);
        setColour (juce::PopupMenu::backgroundColourId, theme::panel);
        setColour (juce::PopupMenu::textColourId, theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::teal.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId, theme::text);
        setColour (juce::TextButton::buttonColourId, theme::panel);
        setColour (juce::TextButton::buttonOnColourId, theme::pink);
        setColour (juce::TextButton::textColourOffId, theme::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::Label::textColourId, theme::text);
        setColour (juce::TextEditor::textColourId, theme::text);
        setColour (juce::TextEditor::highlightedTextColourId, theme::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        // Sliders flagged "inverted" (the big CRONK dial) swap the palette:
        // dark track, hot pink fill, teal thumb.
        const bool inverted = (bool) slider.getProperties()["inverted"];

        const float alpha = slider.isEnabled() ? 1.0f : 0.3f;
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float lineW = juce::jmax (4.0f, radius * 0.2f);
        const float arcRadius = radius - lineW * 0.5f;

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour ((inverted ? theme::outline.withAlpha (0.85f) : theme::control)
                         .withMultipliedAlpha (alpha));
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
            g.setColour ((inverted ? theme::pink : theme::teal).withMultipliedAlpha (alpha));
            g.strokePath (value, { lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        }

        const auto thumb = centre.getPointOnCircumference (arcRadius - lineW * 1.3f, toAngle);
        g.setColour ((inverted ? theme::teal : theme::outline).withMultipliedAlpha (alpha));
        g.fillEllipse (juce::Rectangle<float> (lineW, lineW).withCentre (thumb));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override
    {
        // The power control renders as a hardware-style 1/0 rocker switch:
        // recessed track with a shaded gradient, engraved digits, and a
        // dimensional sliding knob. Orange when on.
        if ((bool) button.getProperties()["powerSwitch"])
        {
            const bool on = button.getToggleState();
            const juce::Colour orange (0xffff8c1a);

            auto track = button.getLocalBounds().toFloat().reduced (1.0f);
            const float corner = track.getHeight() * 0.5f;

            const juce::Colour base = on ? orange : theme::control;
            juce::ColourGradient trackShade (base.darker (0.35f), 0.0f, track.getY(),
                                             base.brighter (0.08f), 0.0f, track.getBottom(), false);
            g.setGradientFill (trackShade);
            g.fillRoundedRectangle (track, corner);

            // Inner shadow lip for the recessed look.
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.drawRoundedRectangle (track.reduced (1.5f), corner - 1.5f, 2.0f);
            g.setColour (theme::outline);
            g.drawRoundedRectangle (track, corner, 2.0f);

            // Engraved digits: dark drop under a light face.
            auto digits = track;
            auto zeroArea = digits.removeFromLeft (digits.getWidth() * 0.5f);
            g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
            for (auto pair : { std::pair<juce::Rectangle<float>, const char*> { zeroArea, "0" },
                               { digits, "1" } })
            {
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.drawText (pair.second, pair.first.translated (0.0f, 1.0f),
                            juce::Justification::centred);
                g.setColour (on ? juce::Colours::white : theme::textDim.withAlpha (0.9f));
                g.drawText (pair.second, pair.first, juce::Justification::centred);
            }

            // Dimensional knob with a specular highlight.
            auto full = button.getLocalBounds().toFloat().reduced (4.0f);
            const float knobSize = full.getHeight();
            auto knob = juce::Rectangle<float> (knobSize, knobSize)
                            .withCentre ({ on ? full.getRight() - knobSize * 0.5f
                                              : full.getX() + knobSize * 0.5f,
                                           full.getCentreY() });

            g.setColour (juce::Colours::black.withAlpha (0.3f)); // drop shadow
            g.fillEllipse (knob.translated (0.0f, 1.5f));

            juce::ColourGradient knobShade (juce::Colours::white, knob.getCentreX(), knob.getY(),
                                            juce::Colour (0xffc9c7c1), knob.getCentreX(), knob.getBottom(), false);
            g.setGradientFill (knobShade);
            g.fillEllipse (knob);
            g.setColour (theme::outline);
            g.drawEllipse (knob, 2.0f);

            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (knob.reduced (knob.getWidth() * 0.28f)
                               .translated (0.0f, -knob.getHeight() * 0.16f));
            return;
        }

        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const float corner = bounds.getHeight() * 0.5f; // pill

        auto colour = backgroundColour;
        if (isDown)
            colour = colour.darker (0.1f);
        else if (isHighlighted)
            colour = colour.brighter (0.05f);

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, corner);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (bounds, corner, 2.0f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, 9.0f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, 9.0f, 2.0f);

        juce::Path chevron;
        const float cx = (float) width - 17.0f, cy = (float) height * 0.5f;
        chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
        chevron.lineTo (cx, cy + 2.5f);
        chevron.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (chevron, { 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (14.0f, juce::Font::bold));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        // Small pills (harmony toggles) get a proportionally smaller font.
        return juce::Font (juce::FontOptions (
            juce::jmin (14.0f, (float) buttonHeight * 0.55f), juce::Font::bold));
    }
};
