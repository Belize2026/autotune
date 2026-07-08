#pragma once

#include "PluginProcessor.h"

class HardTuneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit HardTuneAudioProcessorEditor (HardTuneAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    HardTuneAudioProcessor& processor;

    juce::TextButton powerButton;
    juce::ComboBox keyBox, scaleBox;
    juce::Label titleLabel, keyLabel, scaleLabel, readoutLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment, scaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardTuneAudioProcessorEditor)
};
