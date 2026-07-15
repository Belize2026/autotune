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

    juce::Rectangle<int> headerArea; // logo drawn in paint()
    juce::Image logo;
    juce::TextButton powerButton;
    juce::ComboBox keyBox, scaleBox;
    juce::TextButton harmonyButtons[4]; // 3rd / 5th / oct up / oct down
    juce::Slider cronkDial, dualLevelDial, formantDial, echoDial, reverbDial;
    juce::Label keyLabel, scaleLabel, dualLabel, cronkLabel, extremeLabel, mildLabel,
                dualLevelLabel, formantLabel, echoLabel, reverbLabel;
    CorrectionDisplay display;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        powerAttachment, harmonyAttachments[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        keyAttachment, scaleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        cronkAttachment, dualLevelAttachment, formantAttachment, echoAttachment, reverbAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardTuneAudioProcessorEditor)
};
