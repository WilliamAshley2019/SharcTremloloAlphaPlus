#pragma once
#include <JuceHeader.h>

class SharcTremoloAudioProcessor : public juce::AudioProcessor
{
public:
    SharcTremoloAudioProcessor();
    ~SharcTremoloAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SHARC Tremolo"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // FL Studio optimization: force 32-bit only
    bool supportsDoublePrecisionProcessing() const override { return false; }

    // FL Studio optimization: mono/stereo only
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        auto main = layouts.getMainOutputChannelSet();
        return main == juce::AudioChannelSet::mono()
            || main == juce::AudioChannelSet::stereo();
    }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    void tapTempo();

    // Atomic for GUI metering (no locks in audio thread)
    std::atomic<float> currentModDepth{ 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Pre-allocated in prepareToPlay (no allocation in processBlock)
    double currentSampleRate = 48000.0;
    float t = 0.0f;
    float tremoloRate = 0.5f;
    juce::uint64 lastTapTimeMs = 0;

    // Parameter smoothing (prevents CPU spikes on automation)
    juce::SmoothedValue<float> rateSmoothed, depthSmoothed, shapeSmoothed;

    // Silence detection
    bool isSilentOutput = false;

    // Fast waveform generator (replaces std::sin with optimized LFO)
    inline float generateWaveform(float phase, float shape);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharcTremoloAudioProcessor)
};