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
    setupLabel (dualLabel, "DUAL VOCALS");
    setupCombo (dualBox, "dual", dualAttachment);

    // CRONK: harshness dial, 0% = softest texture, 100% = maximum extreme.
    // Rendered as a clock face (see HardTuneLookAndFeel).
    setupLabel (cronkLabel, "CRONK");
    setupDial (cronkDial, "cronk", " %", cronkAttachment);
    cronkDial.getProperties().set ("clockFace", true);
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

    setSize (560, 500);
    startTimerHz (30);
}

HardTuneAudioProcessorEditor::~HardTuneAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HardTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);

    // Logo-style header: hot pink "IDOL" peeking out from behind metallic
    // teal "AUTISM", both with the sticker outline, like the artwork.
    const juce::Font big (juce::FontOptions (34.0f, juce::Font::bold));
    juce::GlyphArrangement measure;
    measure.addLineOfText (big, "AUTISM", 0.0f, 0.0f);
    const float autismWidth = measure.getBoundingBox (0, -1, true).getWidth();

    const float hx = (float) headerArea.getX();
    const float baseline = (float) headerArea.getBottom() - 8.0f;

    g.setFont (big);
    g.setColour (theme::pink);
    g.drawSingleLineText ("IDOL", (int) (hx + autismWidth * 0.62f), (int) (baseline + 16.0f));
    g.setColour (theme::outline);
    g.drawSingleLineText ("AUTISM", (int) hx + 2, (int) baseline + 2); // outline shadow
    g.setColour (theme::teal);
    g.drawSingleLineText ("AUTISM", (int) hx, (int) baseline);

    g.setColour (theme::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("AUTOTUNE  |  instant hard pitch snap  |  v0.7",
                getLocalBounds().removeFromBottom (24),
                juce::Justification::centred);
}

void HardTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto header = area.removeFromTop (40);
    powerButton.setBounds (header.removeFromRight (96).withSizeKeepingCentre (96, 34));
    headerArea = header;

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
    dualBox.setBounds (dualArea.removeFromTop (30));

    area.removeFromTop (14);

    // Dial row: Cronk / Dual Level / Formant / Echo / Reverb.
    auto dials = area.removeFromTop (150);
    const int dialW = dials.getWidth() / 5;
    const auto cronkCell = dials.withWidth (dialW); // for the end labels below

    auto layoutDial = [&dials, dialW] (juce::Label& label, juce::Slider& dial)
    {
        auto cell = dials.removeFromLeft (dialW).reduced (4, 0);
        label.setBounds (cell.removeFromTop (16));
        dial.setBounds (cell);
    };

    layoutDial (cronkLabel, cronkDial);
    layoutDial (dualLevelLabel, dualLevelDial);
    layoutDial (formantLabel, formantDial);
    layoutDial (echoLabel, echoDial);
    layoutDial (reverbLabel, reverbDial);

    // LESS / EXTREME markers under the CRONK dial.
    auto cronkEnds = area.removeFromTop (12)
                         .withX (cronkCell.getX())
                         .withWidth (dialW)
                         .reduced (8, 0);
    mildLabel.setBounds (cronkEnds.removeFromLeft (cronkEnds.getWidth() / 2));
    extremeLabel.setBounds (cronkEnds);
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
