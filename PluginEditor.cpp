#include "PluginProcessor.h"
#include "PluginEditor.h"

SharcTremoloAudioProcessorEditor::SharcTremoloAudioProcessorEditor(SharcTremoloAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(500, 350);

    // Rate knob (0.1 - 20 Hz)
    addAndMakeVisible(rateSlider);
    rateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    rateSlider.setTextValueSuffix(" Hz");
    rateAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(
        audioProcessor.apvts, "RATE", rateSlider));

    // Depth knob (0 - 100%)
    addAndMakeVisible(depthSlider);
    depthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    depthSlider.setTextValueSuffix(" %");
    depthAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(
        audioProcessor.apvts, "DEPTH", depthSlider));

    // Shape knob (0 = sine, 0.5 = triangle, 1 = square)
    addAndMakeVisible(shapeSlider);
    shapeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    shapeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    shapeAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(
        audioProcessor.apvts, "SHAPE", shapeSlider));

    // Bypass button
    addAndMakeVisible(bypassButton);
    bypassAttach.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(
        audioProcessor.apvts, "BYPASS", bypassButton));

    // Tap Tempo button
    addAndMakeVisible(tapButton);
    tapButton.onClick = [this] { audioProcessor.tapTempo(); };

    // Labels
    addAndMakeVisible(rateLabel);
    rateLabel.setText("RATE", juce::dontSendNotification);
    rateLabel.setJustificationType(juce::Justification::centred);
    rateLabel.attachToComponent(&rateSlider, false);

    addAndMakeVisible(depthLabel);
    depthLabel.setText("DEPTH", juce::dontSendNotification);
    depthLabel.setJustificationType(juce::Justification::centred);
    depthLabel.attachToComponent(&depthSlider, false);

    addAndMakeVisible(shapeLabel);
    shapeLabel.setText("SHAPE", juce::dontSendNotification);
    shapeLabel.setJustificationType(juce::Justification::centred);
    shapeLabel.attachToComponent(&shapeSlider, false);

    // Start GUI update timer (30 FPS for smooth metering, no audio thread calls)
    startTimerHz(30);
}

SharcTremoloAudioProcessorEditor::~SharcTremoloAudioProcessorEditor()
{
    stopTimer();
}

void SharcTremoloAudioProcessorEditor::timerCallback()
{
    // Read atomic value from audio thread (lock-free, FL Studio optimization)
    currentMeterValue = audioProcessor.currentModDepth.load();
    repaint();
}

void SharcTremoloAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("DDX3216 SHARC Tremolo", 0, 15, getWidth(), 30,
        juce::Justification::centred, 1);

    // Subtitle
    g.setFont(12.0f);
    g.setColour(juce::Colours::grey);
    g.drawFittedText("Vintage Tremolo • Shape Morphing LFO", 0, 45, getWidth(), 20,
        juce::Justification::centred, 1);

    // LFO activity meter (visual feedback)
    auto meterArea = juce::Rectangle<int>(20, 240, getWidth() - 40, 30);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(meterArea);

    g.setColour(juce::Colour(0xff00ff88).withAlpha(0.8f));
    auto meterWidth = static_cast<int> (meterArea.getWidth() * currentMeterValue);
    g.fillRect(meterArea.getX(), meterArea.getY(), meterWidth, meterArea.getHeight());

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawRect(meterArea, 1);
    g.drawText("LFO DEPTH", meterArea, juce::Justification::centred);
}

void SharcTremoloAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(20);
    r.removeFromTop(75); // Space for title and subtitle

    // Three knobs in a row
    auto knobArea = r.removeFromTop(130);
    auto knobWidth = knobArea.getWidth() / 3;

    rateSlider.setBounds(knobArea.removeFromLeft(knobWidth).reduced(15));
    depthSlider.setBounds(knobArea.removeFromLeft(knobWidth).reduced(15));
    shapeSlider.setBounds(knobArea.removeFromLeft(knobWidth).reduced(15));

    r.removeFromTop(50); // Space for meter
    r.removeFromTop(30); // Meter area
    r.removeFromTop(10); // Spacing

    // Buttons at bottom
    auto buttonArea = r.removeFromTop(50);
    bypassButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(10, 5));
    tapButton.setBounds(buttonArea.reduced(10, 5));
}