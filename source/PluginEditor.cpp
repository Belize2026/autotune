#include "PluginEditor.h"

namespace
{
constexpr auto backgroundColour = 0xff16161c;
constexpr auto panelColour      = 0xff20202a;
constexpr auto accentColour     = 0xffff2d78;
constexpr auto offColour        = 0xff3a3a46;

juce::String noteNameForHz (float hz)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B" };
    const int midi = (int) std::lround (hardtune::Quantizer::frequencyToMidi (hz));
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}
} // namespace

HardTuneAudioProcessorEditor::HardTuneAudioProcessorEditor (HardTuneAudioProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    auto& apvts = processor.getValueTreeState();

    titleLabel.setText ("HARD TUNE", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    powerButton.setClickingTogglesState (true);
    powerButton.setColour (juce::TextButton::buttonColourId, juce::Colour (offColour));
    powerButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (accentColour));
    powerButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    powerButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible (powerButton);
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "power", powerButton);

    auto setupCombo = [this, &apvts] (juce::ComboBox& box, juce::Label& label,
                                      const juce::String& text, const juce::String& paramID,
                                      auto& attachment)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
        addAndMakeVisible (label);

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID)))
            box.addItemList (choice->choices, 1);

        box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (panelColour));
        box.setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.15f));
        addAndMakeVisible (box);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, paramID, box);
    };

    setupCombo (keyBox, keyLabel, "KEY", "key", keyAttachment);
    setupCombo (scaleBox, scaleLabel, "SCALE", "scale", scaleAttachment);

    readoutLabel.setFont (juce::FontOptions (15.0f));
    readoutLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
    readoutLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (readoutLabel);

    setSize (420, 300);
    startTimerHz (30);
}

void HardTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (backgroundColour));

    g.setColour (juce::Colour (panelColour));
    g.fillRoundedRectangle (getLocalBounds().reduced (12).toFloat(), 10.0f);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("zero-glide hard pitch snap  |  v0.1",
                getLocalBounds().removeFromBottom (26),
                juce::Justification::centred);
}

void HardTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (24);

    titleLabel.setBounds (area.removeFromTop (44));
    area.removeFromTop (8);

    powerButton.setBounds (area.removeFromTop (72).withSizeKeepingCentre (180, 64));
    area.removeFromTop (16);

    auto row = area.removeFromTop (58);
    auto keyArea = row.removeFromLeft (row.getWidth() / 2).reduced (8, 0);
    auto scaleArea = row.reduced (8, 0);

    keyLabel.setBounds (keyArea.removeFromTop (18));
    keyBox.setBounds (keyArea.removeFromTop (30));
    scaleLabel.setBounds (scaleArea.removeFromTop (18));
    scaleBox.setBounds (scaleArea.removeFromTop (30));

    area.removeFromTop (8);
    readoutLabel.setBounds (area.removeFromTop (24));
}

void HardTuneAudioProcessorEditor::timerCallback()
{
    powerButton.setButtonText (powerButton.getToggleState() ? "ON" : "OFF");

    const float detected = processor.detectedHz.load();
    const float target   = processor.targetHz.load();

    if (detected > 0.0f && target > 0.0f)
        readoutLabel.setText (noteNameForHz (detected) + "  ->  " + noteNameForHz (target),
                              juce::dontSendNotification);
    else
        readoutLabel.setText ("-", juce::dontSendNotification);
}
