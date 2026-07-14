#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/Echo.h"
#include "dsp/FormantShifter.h"
#include "dsp/PitchDetector.h"
#include "dsp/PitchShifter.h"
#include "dsp/Quantizer.h"

class HardTuneAudioProcessor : public juce::AudioProcessor
{
public:
    HardTuneAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override
    {
        const bool hasTail = (reverbParam != nullptr && reverbParam->load() > 0.0f)
                          || (echoParam != nullptr && echoParam->load() > 0.0f);
        return hasTail ? 3.0 : 0.0;
    }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }

    // Live readout for the editor (0 = unvoiced / bypassed).
    std::atomic<float> detectedHz { 0.0f };
    std::atomic<float> targetHz   { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void applyFormantStage (float* channel, int numSamples);
    void applyPostEffects (juce::AudioBuffer<float>& buffer, bool tuneWasApplied);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* powerParam   = nullptr;
    std::atomic<float>* keyParam     = nullptr;
    std::atomic<float>* scaleParam   = nullptr;
    std::atomic<float>* snapParam    = nullptr;
    std::atomic<float>* formantParam = nullptr;
    std::atomic<float>* dualParam    = nullptr;
    std::atomic<float>* dualMixParam = nullptr;
    std::atomic<float>* echoParam    = nullptr;
    std::atomic<float>* reverbParam  = nullptr;

    hardtune::PitchDetector  detector;
    hardtune::Quantizer      quantizer;
    hardtune::PitchShifter   shifter;
    hardtune::FormantShifter formantShifter;
    hardtune::Echo           echo;

    juce::Reverb reverb;
    bool reverbWasActive = false;
    bool echoWasActive   = false;
    double lastTargetHz  = 0.0;

    // Re-detect at most every `detectionHopSamples` samples so tiny host
    // block sizes don't multiply the analysis cost.
    int detectionHopSamples   = 256;
    int samplesSinceDetection = 0;
    double currentRatio       = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardTuneAudioProcessor)
};
