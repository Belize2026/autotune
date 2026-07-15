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
    powerButton.getProperties().set ("powerSwitch", true); // 1/0 rocker look
    addAndMakeVisible (powerButton);
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "power", powerButton);

    addAndMakeVisible (display);
    addAndMakeVisible (meter);

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

    // KEY: a strip of 12 note buttons instead of a plain dropdown.
    setupLabel (keyLabel, "KEY");
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F",
                                       "F#", "G", "G#", "A", "A#", "B" };
    for (int k = 0; k < 12; ++k)
    {
        auto& button = keyButtons[k];
        button.setButtonText (noteNames[k]);
        button.setClickingTogglesState (true);
        button.setRadioGroupId (42);
        button.setColour (juce::TextButton::buttonOnColourId, theme::teal);
        button.onClick = [this, k]
        {
            if (auto* param = processor.getValueTreeState().getParameter ("key"))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost ((float) k / 11.0f);
                param->endChangeGesture();
            }
        };
        addAndMakeVisible (button);
    }

    setupLabel (scaleLabel, "SCALE");
    setupCombo (scaleBox, "scale", scaleAttachment);

    // Four harmony slots: interval dropdowns here, their MIX dials live as
    // mini cronk-style knobs next to the big CRONK.
    setupLabel (harmonySectionLabel, "HARMONIES  (UP TO 4 VOICES UNDER THE LEAD)");
    for (int v = 0; v < 4; ++v)
    {
        const auto num = juce::String (v + 1);
        setupLabel (harmonyLabels[v], "HARMONY " + num);
        setupCombo (harmonyBoxes[v], "harm" + num, harmonyAttachments[v]);

        setupLabel (mixLabels[v], "MIX " + num);
        auto& mixDial = mixSliders[v];
        mixDial.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        mixDial.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        mixDial.getProperties().set ("inverted", true); // mini cronk look
        addAndMakeVisible (mixDial);
        mixAttachments[v] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, "mix" + num, mixDial);
    }

    // CRONK: the big centre dial, 0% = softest texture, 100% = maximum
    // extreme. Inverted colours relative to the other knobs.
    setupLabel (cronkLabel, "CRONK");
    setupDial (cronkDial, "cronk", " %", cronkAttachment);
    cronkDial.getProperties().set ("inverted", true);
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

    setSize (560, 640);
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
    g.drawText ("AUTOTUNE  |  instant hard pitch snap  |  v0.11",
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

    // KEY: 12-note button strip.
    auto keyRow = area.removeFromTop (44);
    keyLabel.setBounds (keyRow.removeFromTop (14));
    const int noteW = keyRow.getWidth() / 12;
    for (int k = 0; k < 12; ++k)
        keyButtons[k].setBounds (keyRow.removeFromLeft (k == 11 ? keyRow.getWidth() : noteW)
                                     .reduced (2, 2));

    area.removeFromTop (10);

    // Scale (left) + 2x2 harmony interval dropdowns (right).
    auto row = area.removeFromTop (72);
    auto scaleArea = row.removeFromLeft (row.getWidth() / 3).reduced (6, 0);
    scaleLabel.setBounds (scaleArea.removeFromTop (14));
    scaleBox.setBounds (scaleArea.removeFromTop (28));

    auto harmonyArea = row.reduced (6, 0);
    harmonySectionLabel.setBounds (harmonyArea.removeFromTop (14));
    for (int gridRow = 0; gridRow < 2; ++gridRow)
    {
        auto slotRow = harmonyArea.removeFromTop (29);
        for (int gridCol = 0; gridCol < 2; ++gridCol)
        {
            const int v = gridRow * 2 + gridCol;
            auto cell = slotRow.removeFromLeft (slotRow.getWidth() / (2 - gridCol)).reduced (2, 1);
            harmonyLabels[v].setBounds (cell.removeFromLeft (74));
            harmonyBoxes[v].setBounds (cell);
        }
    }

    area.removeFromTop (12);

    // Dial section: BIG CRONK (main vocal, left) | mini MIX 1-4 dials for
    // the harmony voices | Formant / Echo / Reverb.
    auto dials = area.removeFromTop (170);

    auto cronkCell = dials.removeFromLeft (150).reduced (2, 0);
    cronkLabel.setBounds (cronkCell.removeFromTop (16));
    auto cronkEnds = cronkCell.removeFromBottom (12).reduced (10, 0);
    cronkDial.setBounds (cronkCell);
    mildLabel.setBounds (cronkEnds.removeFromLeft (cronkEnds.getWidth() / 2));
    extremeLabel.setBounds (cronkEnds);

    auto mixBlock = dials.removeFromLeft (136).reduced (2, 0);
    for (int gridRow = 0; gridRow < 2; ++gridRow)
    {
        auto mixRow = mixBlock.removeFromTop (mixBlock.getHeight() / (2 - gridRow));
        for (int gridCol = 0; gridCol < 2; ++gridCol)
        {
            const int v = gridRow * 2 + gridCol;
            auto cell = mixRow.removeFromLeft (mixRow.getWidth() / (2 - gridCol)).reduced (2, 1);
            mixLabels[v].setBounds (cell.removeFromTop (12));
            mixSliders[v].setBounds (cell);
        }
    }

    const int smallW = dials.getWidth() / 3;
    auto layoutSmall = [&dials, smallW] (juce::Label& label, juce::Slider& dial)
    {
        auto cell = dials.removeFromLeft (smallW).reduced (2, 0);
        label.setBounds (cell.removeFromTop (16));
        dial.setBounds (cell.withTrimmedBottom (36));
    };

    layoutSmall (formantLabel, formantDial);
    layoutSmall (echoLabel, echoDial);
    layoutSmall (reverbLabel, reverbDial);

    area.removeFromTop (8);

    // Output level meter.
    meter.setBounds (area.removeFromTop (30));
}

void HardTuneAudioProcessorEditor::timerCallback()
{
    // Keep the key strip in sync with the parameter (host automation etc.).
    const int key = juce::jlimit (0, 11,
        (int) processor.getValueTreeState().getRawParameterValue ("key")->load());
    if (! keyButtons[key].getToggleState())
        keyButtons[key].setToggleState (true, juce::dontSendNotification);

    meter.update (processor.outputPeak.exchange (0.0f));

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
