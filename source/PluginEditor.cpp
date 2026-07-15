#include "PluginEditor.h"

#include "BinaryData.h"

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

    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    powerButton.setClickingTogglesState (true);
    addAndMakeVisible (powerButton);
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "power", powerButton);

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
        dial.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
        dial.setTextValueSuffix (suffix);
        // Explicit per-component colours: the slider caches its text-box
        // label colours before the editor's LookAndFeel is attached.
        dial.setColour (juce::Slider::textBoxTextColourId, theme::text);
        dial.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        dial.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (dial);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, paramID, dial);
    };

    setupLabel (keyLabel, "KEY");
    setupCombo (keyBox, "key", keyAttachment);
    setupLabel (scaleLabel, "SCALE");
    setupCombo (scaleBox, "scale", scaleAttachment);

    // Stackable harmony voices: tick any combination.
    setupLabel (dualLabel, "HARMONY (STACK ANY)");
    static const char* harmonyIDs[]     = { "h3rd", "h5th", "hoctup", "hoctdown" };
    static const char* harmonyText[]    = { "3RD", "5TH", "OCT +", "OCT -" };
    for (int v = 0; v < 4; ++v)
    {
        auto& button = harmonyButtons[v];
        button.setButtonText (harmonyText[v]);
        button.setClickingTogglesState (true);
        button.setColour (juce::TextButton::buttonOnColourId, theme::teal);
        addAndMakeVisible (button);
        harmonyAttachments[v] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvts, harmonyIDs[v], button);
    }

    // CRONK: the big centre dial, 0% = softest texture, 100% = maximum
    // extreme. Inverted colours relative to the other knobs.
    setupLabel (cronkLabel, "CRONK");
    setupDial (cronkDial, "cronk", " %", cronkAttachment);
    cronkDial.getProperties().set ("inverted", true);
    setupLabel (dualLevelLabel, "DUAL LEVEL");
    setupDial (dualLevelDial, "duallevel", " %", dualLevelAttachment);
    setupLabel (formantLabel, "FORMANT");
    setupDial (formantDial, "formant", " st", formantAttachment);
    setupLabel (echoLabel, "ECHO");
    setupDial (echoDial, "echo", " %", echoAttachment);
    setupLabel (reverbLabel, "REVERB");
    setupDial (reverbDial, "reverb", " %", reverbAttachment);

    auto setupEndLabel = [this] (juce::Label& label, const juce::String& text,
                                 juce::Justification just, juce::Colour colour)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType (just);
        addAndMakeVisible (label);
    };
    setupEndLabel (mildLabel, "LESS", juce::Justification::centredLeft, theme::textDim);
    setupEndLabel (extremeLabel, "EXTREME", juce::Justification::centredRight, theme::pink);

    setSize (560, 520);
    startTimerHz (30);
}

HardTuneAudioProcessorEditor::~HardTuneAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HardTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);

    // The AUTISMIDOL logo artwork, scaled into the header.
    if (logo.isValid())
    {
        const float scale = juce::jmin ((float) headerArea.getHeight() / (float) logo.getHeight(),
                                        (float) headerArea.getWidth() / (float) logo.getWidth());
        const int w = (int) ((float) logo.getWidth() * scale);
        const int h = (int) ((float) logo.getHeight() * scale);
        g.drawImage (logo,
                     headerArea.getX(), headerArea.getCentreY() - h / 2, w, h,
                     0, 0, logo.getWidth(), logo.getHeight());
    }

    g.setColour (theme::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("AUTOTUNE  |  instant hard pitch snap  |  v0.8",
                getLocalBounds().removeFromBottom (24),
                juce::Justification::centred);
}

void HardTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto header = area.removeFromTop (52);
    powerButton.setBounds (header.removeFromRight (96).withSizeKeepingCentre (96, 34));
    headerArea = header.withTrimmedRight (8);

    area.removeFromTop (12);
    display.setBounds (area.removeFromTop (122));
    area.removeFromTop (14);

    // Key / Scale / Harmony row.
    auto row = area.removeFromTop (64);
    const int colW = row.getWidth() / 3;
    auto keyArea = row.removeFromLeft (colW).reduced (6, 0);
    auto scaleArea = row.removeFromLeft (colW).reduced (6, 0);
    auto harmonyArea = row.reduced (6, 0);

    keyLabel.setBounds (keyArea.removeFromTop (16));
    keyBox.setBounds (keyArea.removeFromTop (30));
    scaleLabel.setBounds (scaleArea.removeFromTop (16));
    scaleBox.setBounds (scaleArea.removeFromTop (30));

    dualLabel.setBounds (harmonyArea.removeFromTop (16));
    auto harmonyTop = harmonyArea.removeFromTop (22);
    auto harmonyBottom = harmonyArea.removeFromTop (22).translated (0, 2);
    harmonyButtons[0].setBounds (harmonyTop.removeFromLeft (harmonyTop.getWidth() / 2).reduced (2, 1));
    harmonyButtons[1].setBounds (harmonyTop.reduced (2, 1));
    harmonyButtons[2].setBounds (harmonyBottom.removeFromLeft (harmonyBottom.getWidth() / 2).reduced (2, 1));
    harmonyButtons[3].setBounds (harmonyBottom.reduced (2, 1));

    area.removeFromTop (14);

    // Dial row: Dual Level / Formant / BIG CRONK (centre) / Echo / Reverb.
    auto dials = area.removeFromTop (168);
    const int bigW = 168;
    const int smallW = (dials.getWidth() - bigW) / 4;

    auto layoutSmall = [&dials, smallW] (juce::Label& label, juce::Slider& dial)
    {
        auto cell = dials.removeFromLeft (smallW).reduced (2, 0);
        label.setBounds (cell.removeFromTop (16));
        dial.setBounds (cell.withTrimmedBottom (34));
    };

    layoutSmall (dualLevelLabel, dualLevelDial);
    layoutSmall (formantLabel, formantDial);

    auto cronkCell = dials.removeFromLeft (bigW).reduced (2, 0);
    cronkLabel.setBounds (cronkCell.removeFromTop (16));
    auto cronkEnds = cronkCell.removeFromBottom (12).reduced (14, 0);
    cronkDial.setBounds (cronkCell);
    mildLabel.setBounds (cronkEnds.removeFromLeft (cronkEnds.getWidth() / 2));
    extremeLabel.setBounds (cronkEnds);

    layoutSmall (echoLabel, echoDial);
    layoutSmall (reverbLabel, reverbDial);
}

void HardTuneAudioProcessorEditor::timerCallback()
{
    powerButton.setButtonText (powerButton.getToggleState() ? "ON" : "OFF");

    const float detected = processor.detectedHz.load();
    const float target   = processor.targetHz.load();

    if (detected > 0.0f && target > 0.0f)
    {
        const float cents = 1200.0f * std::log2 (target / detected);
        display.update (cents, true);
        display.setStatusText (noteNameForHz (target));
    }
    else
    {
        display.update (0.0f, false);
        display.setStatusText (powerButton.getToggleState() ? juce::String() : "BYPASSED");
    }
}
