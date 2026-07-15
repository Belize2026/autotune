#pragma once

#include "PluginProcessor.h"
#include "ui/CorrectionDisplay.h"
#include "ui/HardTuneLookAndFeel.h"
#include "ui/LevelMeter.h"

class HardTuneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit HardTuneAudioProcessorEditor (HardTuneAudioProcessor&);
    ~HardTuneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    HardTuneAudioProcessor& processor;
    HardTuneLookAndFeel lookAndFeel; // declared before the components that use it

    juce::Rectangle<int> headerArea; // logo drawn in paint()
    juce::Image logo;
    juce::TextButton powerButton;
    juce::TextButton keyButtons[12]; // C..B strip, radio-style
    juce::ComboBox scaleBox;
    // Four harmony slots: interval dropdowns (2x2) + mini MIX dials in the
    // dial section.
    juce::ComboBox harmonyBoxes[4];
    juce::Slider mixSliders[4];
    juce::Label harmonyLabels[4], mixLabels[4];
    juce::Slider cronkDial, formantDial, echoDial, reverbDial;
    juce::Label keyLabel, scaleLabel, harmonySectionLabel, cronkLabel, extremeLabel, mildLabel,
                formantLabel, echoLabel, reverbLabel;
    CorrectionDisplay display;
    LevelMeter meter;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        scaleAttachment, harmonyAttachments[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        mixAttachments[4], cronkAttachment, formantAttachment, echoAttachment, reverbAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardTuneAudioProcessorEditor)
};
