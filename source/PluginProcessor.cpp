#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const juce::StringArray keyNames { "C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B" };
const juce::StringArray scaleNames { "Chromatic", "Major", "Minor", "Minor Pentatonic" };
} // namespace

HardTuneAudioProcessor::HardTuneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::mono(), true)
                          .withOutput ("Output", juce::AudioChannelSet::mono(), true)),
      apvts (*this, nullptr, "HardTune", createParameterLayout())
{
    powerParam   = apvts.getRawParameterValue ("power");
    keyParam     = apvts.getRawParameterValue ("key");
    scaleParam   = apvts.getRawParameterValue ("scale");
    formantParam = apvts.getRawParameterValue ("formant");
    dualParam    = apvts.getRawParameterValue ("dual");
    dualMixParam = apvts.getRawParameterValue ("dualmix");
    echoParam    = apvts.getRawParameterValue ("echo");
    reverbParam  = apvts.getRawParameterValue ("reverb");
}

juce::AudioProcessorValueTreeState::ParameterLayout HardTuneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "power", 1 }, "On", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "key", 1 }, "Key", keyNames, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "scale", 1 }, "Scale", scaleNames, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "formant", 1 }, "Formant",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "dual", 1 }, "Dual Vocals", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dualmix", 1 }, "Dual Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "echo", 1 }, "Echo",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverb", 1 }, "Reverb",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return { params.begin(), params.end() };
}

bool HardTuneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void HardTuneAudioProcessor::prepareToPlay (double sampleRate, int)
{
    detector.prepare (sampleRate);
    shifter.prepare (sampleRate);

    // Re-detect roughly every 3 ms: corrections land essentially instantly.
    detectionHopSamples   = juce::jmax (32, (int) std::lround (sampleRate * 0.003));
    samplesSinceDetection = detectionHopSamples; // detect on the first block
    currentRatio          = 1.0;

    formantShifter.prepare (sampleRate);
    echo.prepare (sampleRate);
    echoWasActive = false;
    lastTargetHz  = 0.0;

    reverb.setSampleRate (sampleRate);
    reverb.reset();
    reverbWasActive = false;

    setLatencySamples (shifter.getLatencySamples());
}

void HardTuneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0)
        return;

    float* channel = buffer.getWritePointer (0);

    // The detector always sees the dry input, on or off, so the pitch
    // readout works and re-engaging is instant.
    detector.push (channel, numSamples);

    const bool on = powerParam->load() > 0.5f;
    if (! on)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            shifter.skipSample (channel[i]);
            formantShifter.skipSample (channel[i]);
        }

        detectedHz.store (0.0f);
        targetHz.store (0.0f);

        // The tune chain passes dry, but echo and reverb stay independent.
        applyPostEffects (buffer, false);
        return;
    }

    samplesSinceDetection += numSamples;
    if (samplesSinceDetection >= detectionHopSamples)
    {
        samplesSinceDetection = 0;

        const auto result = detector.detect();
        if (result.voiced)
        {
            quantizer.set ((int) keyParam->load(),
                           (hardtune::Scale) (int) scaleParam->load());

            const float target = quantizer.snapFrequencyHz (result.frequencyHz);
            currentRatio = (double) target / (double) result.frequencyHz;
            lastTargetHz = target;

            detectedHz.store (result.frequencyHz);
            targetHz.store (target);
        }
        else
        {
            // Unvoiced / uncertain frames HOLD the last ratio instead of
            // easing back to dry — the tuner never relaxes its grip
            // mid-phrase. (During real silence the ratio is inaudible
            // anyway.) Harsh and artificial on purpose.
            detectedHz.store (0.0f);
            targetHz.store (0.0f);
        }

        // Zero glide: the new ratio lands mid-buffer, all at once.
        shifter.setRatio (currentRatio);
    }

    for (int i = 0; i < numSamples; ++i)
        channel[i] = shifter.processSample (channel[i]);

    applyFormantStage (channel, numSamples);
    applyPostEffects (buffer, true);

    for (int ch = 1; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, channel, numSamples);
}

void HardTuneAudioProcessor::applyFormantStage (float* channel, int numSamples)
{
    const float formantSemitones = formantParam->load();
    const bool  dual             = dualParam->load() > 0.5f;
    const float dualMix          = dualMixParam->load() * 0.01f;

    // Grains follow the note the tuned vocal is sitting on.
    formantShifter.setFundamental (lastTargetHz);
    formantShifter.setRatio (std::exp2 ((double) formantSemitones / 12.0));

    if (dual)
    {
        // Main vocal stays on level; the formant-shifted double is blended
        // in underneath with the mix dial.
        for (int i = 0; i < numSamples; ++i)
            channel[i] += dualMix * formantShifter.processSample (channel[i]);
    }
    else if (std::abs (formantSemitones) > 0.05f)
    {
        // Single-vocal mode: reshape the main vocal directly.
        for (int i = 0; i < numSamples; ++i)
            channel[i] = formantShifter.processSample (channel[i]);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            formantShifter.skipSample (channel[i]);
    }
}

void HardTuneAudioProcessor::applyPostEffects (juce::AudioBuffer<float>& buffer, bool tuneWasApplied)
{
    const int numSamples = buffer.getNumSamples();
    const bool stereoDry = ! tuneWasApplied && buffer.getNumChannels() >= 2;

    const float echoAmount = echoParam->load() * 0.01f;
    if (echoAmount > 0.001f)
    {
        echoWasActive = true;
        if (stereoDry)
            echo.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1),
                                numSamples, echoAmount * 0.85f);
        else
            echo.processMono (buffer.getWritePointer (0), numSamples, echoAmount * 0.85f);
    }
    else if (echoWasActive)
    {
        echo.reset();
        echoWasActive = false;
    }

    const float amount = reverbParam->load() * 0.01f;

    if (amount <= 0.001f)
    {
        // Dial at zero = reverb fully inactive; clear the tail so nothing
        // stale plays back when it is dialled up again.
        if (reverbWasActive)
        {
            reverb.reset();
            reverbWasActive = false;
        }
        return;
    }
    reverbWasActive = true;

    juce::Reverb::Parameters params;
    params.roomSize = 0.72f;
    params.damping  = 0.45f;
    params.width    = 1.0f;
    params.wetLevel = amount * 0.9f;
    params.dryLevel = 1.0f - 0.4f * amount;
    reverb.setParameters (params);

    // The tuned path is mono (channel 0 is copied out afterwards); the
    // bypassed path keeps a stereo input stereo.
    if (! tuneWasApplied && buffer.getNumChannels() >= 2)
        reverb.processStereo (buffer.getWritePointer (0),
                              buffer.getWritePointer (1),
                              buffer.getNumSamples());
    else
        reverb.processMono (buffer.getWritePointer (0), buffer.getNumSamples());
}

void HardTuneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HardTuneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* HardTuneAudioProcessor::createEditor()
{
    return new HardTuneAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HardTuneAudioProcessor();
}
