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

    juce::Rectangle<int> headerArea; // logo-style title painted in paint()
    juce::TextButton powerButton;
    juce::ComboBox keyBox, scaleBox, dualBox;
    juce::Slider cronkDial, formantDial, echoDial, reverbDial;
    juce::Label keyLabel, scaleLabel, dualLabel, cronkLabel, extremeLabel, mildLabel,
                formantLabel, echoLabel, reverbLabel;
    CorrectionDisplay display;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        keyAttachment, scaleAttachment, dualAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        cronkAttachment, formantAttachment, echoAttachment, reverbAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardTuneAudioProcessorEditor)
};
