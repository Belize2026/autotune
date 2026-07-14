#include "PluginEditor.h"

namespace
{
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
    setLookAndFeel (&lookAndFeel);
    auto& apvts = processor.getValueTreeState();

    titleLabel.setText ("AUTISMIDOL AUTOTUNE", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, theme::text);
    addAndMakeVisible (titleLabel);

    powerButton.setClickingTogglesState (true);
    addAndMakeVisible (powerButton);
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "power", powerButton);

    dualButton.setClickingTogglesState (true);
    addAndMakeVisible (dualButton);
    dualAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "dual", dualButton);

    addAndMakeVisible (display);

    auto setupLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        label.setColour (juce::Label::textColourId, theme::textDim);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    auto setupCombo = [this, &apvts] (juce::ComboBox& box, const juce::String& paramID,
                                      auto& attachment)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID)))
            box.addItemList (choice->choices, 1);
        addAndMakeVisible (box);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, paramID, box);
    };

    auto setupDial = [this, &apvts] (juce::Slider& dial, const juce::String& paramID,
                                     const juce::String& suffix, auto& attachment)
    {
        dial.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        dial.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 16);
        dial.setTextValueSuffix (suffix);
        addAndMakeVisible (dial);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, paramID, dial);
    };

    setupLabel (keyLabel, "KEY");
    setupCombo (keyBox, "key", keyAttachment);
    setupLabel (scaleLabel, "SCALE");
    setupCombo (scaleBox, "scale", scaleAttachment);
    setupLabel (dualLabel, "DUAL VOCALS");

    setupLabel (formantLabel, "FORMANT");
    setupDial (formantDial, "formant", " st", formantAttachment);
    setupLabel (dualMixLabel, "DUAL MIX");
    setupDial (dualMixDial, "dualmix", " %", dualMixAttachment);
    setupLabel (echoLabel, "ECHO");
    setupDial (echoDial, "echo", " %", echoAttachment);
    setupLabel (reverbLabel, "REVERB");
    setupDial (reverbDial, "reverb", " %", reverbAttachment);

    setSize (560, 478);
    startTimerHz (30);
}

HardTuneAudioProcessorEditor::~HardTuneAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HardTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);

    g.setColour (theme::textDim.withAlpha (0.5f));
    g.setFont (juce::Font (juce::FontOptions (10.5f)));
    g.drawText ("zero-glide hard pitch snap  |  v0.1",
                getLocalBounds().removeFromBottom (24),
                juce::Justification::centred);
}

void HardTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto header = area.removeFromTop (36);
    powerButton.setBounds (header.removeFromRight (96).withSizeKeepingCentre (96, 32));
    titleLabel.setBounds (header);

    area.removeFromTop (12);
    display.setBounds (area.removeFromTop (128));
    area.removeFromTop (14);

    // Key / Scale / Dual toggle row.
    auto row = area.removeFromTop (50);
    const int colW = row.getWidth() / 3;
    auto keyArea = row.removeFromLeft (colW).reduced (6, 0);
    auto scaleArea = row.removeFromLeft (colW).reduced (6, 0);
    auto dualArea = row.reduced (6, 0);

    keyLabel.setBounds (keyArea.removeFromTop (16));
    keyBox.setBounds (keyArea.removeFromTop (30));
    scaleLabel.setBounds (scaleArea.removeFromTop (16));
    scaleBox.setBounds (scaleArea.removeFromTop (30));
    dualLabel.setBounds (dualArea.removeFromTop (16));
    dualButton.setBounds (dualArea.removeFromTop (30).withSizeKeepingCentre (84, 28));

    area.removeFromTop (16);

    // Dial row: Formant / Dual Mix / Echo / Reverb.
    auto dials = area.removeFromTop (150);
    const int dialW = dials.getWidth() / 4;

    auto layoutDial = [&dials, dialW] (juce::Label& label, juce::Slider& dial)
    {
        auto cell = dials.removeFromLeft (dialW).reduced (4, 0);
        label.setBounds (cell.removeFromTop (16));
        dial.setBounds (cell);
    };

    layoutDial (formantLabel, formantDial);
    layoutDial (dualMixLabel, dualMixDial);
    layoutDial (echoLabel, echoDial);
    layoutDial (reverbLabel, reverbDial);
}

void HardTuneAudioProcessorEditor::timerCallback()
{
    powerButton.setButtonText (powerButton.getToggleState() ? "ON" : "OFF");
    dualButton.setButtonText (dualButton.getToggleState() ? "ON" : "OFF");
    dualMixDial.setEnabled (dualButton.getToggleState());
    dualMixLabel.setAlpha (dualButton.getToggleState() ? 1.0f : 0.35f);

    const float detected = processor.detectedHz.load();
    const float target   = processor.targetHz.load();

    if (detected > 0.0f && target > 0.0f)
    {
        const float cents = 1200.0f * std::log2 (target / detected);
        display.push (cents, true);

        juce::String sign (cents >= 0.5f ? "+" : (cents <= -0.5f ? "-" : ""));
        display.setReadout (noteNameForHz (detected) + "  >  " + noteNameForHz (target)
                            + "   " + sign + juce::String (std::abs ((int) std::lround (cents)))
                            + " ct");
    }
    else
    {
        display.push (0.0f, false);
        display.setReadout (powerButton.getToggleState() ? juce::String() : "BYPASSED");
    }
}
