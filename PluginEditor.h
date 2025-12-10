#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SharcTremoloAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    SharcTremoloAudioProcessorEditor(SharcTremoloAudioProcessor&);
    ~SharcTremoloAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    SharcTremoloAudioProcessor& audioProcessor;

    juce::Slider rateSlider, depthSlider, shapeSlider;
    juce::ToggleButton bypassButton{ "Bypass" };
    juce::TextButton tapButton{ "Tap Tempo" };

    juce::Label rateLabel, depthLabel, shapeLabel;

    // Visual LFO meter
    float currentMeterValue = 0.0f;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharcTremoloAudioProcessorEditor)
};