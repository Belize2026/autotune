#pragma once

#include "PluginProcessor.h"
#include "ui/CorrectionDisplay.h"
#include "ui/HardTuneLookAndFeel.h"

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

    juce::Label titleLabel;
    juce::TextButton powerButton, dualButton;
    juce::ComboBox keyBox, scaleBox;
    juce::Slider formantDial, dualMixDial, echoDial, reverbDial;
    juce::Label keyLabel, scaleLabel, dualLabel, formantLabel, dualMixLabel, echoLabel, reverbLabel;
    CorrectionDisplay display;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment, dualAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment, scaleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        formantAttachment, dualMixAttachment, echoAttachment, reverbAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardTuneAudioProcessorEditor)
};
