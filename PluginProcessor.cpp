#include "PluginProcessor.h"
#include "PluginEditor.h"

SharcTremoloAudioProcessor::SharcTremoloAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    setLatencySamples(0); // FL Studio optimization: no latency = no extra buffering
}

SharcTremoloAudioProcessor::~SharcTremoloAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SharcTremoloAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "RATE", "Rate",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), // 0.1-20 Hz, skewed for better knob feel
        1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DEPTH", "Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), // 0-100%
        50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SHAPE", "Shape",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), // 0=sine, 0.5=triangle, 1=square
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "BYPASS", "Bypass",
        false));

    return { params.begin(), params.end() };
}

void SharcTremoloAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    t = 0.0f;

    // FL Studio optimization: 10ms smoothing prevents automation spikes
    rateSmoothed.reset(sampleRate, 0.01);
    depthSmoothed.reset(sampleRate, 0.01);
    shapeSmoothed.reset(sampleRate, 0.01);

    // Initialize smoothed values to current parameter values
    rateSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("RATE"));
    depthSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("DEPTH"));
    shapeSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("SHAPE"));
}

void SharcTremoloAudioProcessor::releaseResources()
{
    // Clean shutdown
}

void SharcTremoloAudioProcessor::tapTempo()
{
    auto now = juce::Time::getMillisecondCounterHiRes();
    if (lastTapTimeMs > 0)
    {
        auto deltaMs = static_cast<juce::uint64> (now - static_cast<double> (lastTapTimeMs));
        if (deltaMs > 50 && deltaMs < 5000) // 0.2 Hz to 20 Hz range
        {
            double periodSamples = (static_cast<double>(deltaMs) / 1000.0) * currentSampleRate;
            tremoloRate = static_cast<float>(juce::MathConstants<float>::twoPi / periodSamples);
        }
    }
    lastTapTimeMs = static_cast<juce::uint64>(now);
}

// Optimized waveform generator with shape morphing
inline float SharcTremoloAudioProcessor::generateWaveform(float phase, float shape)
{
    // Normalize phase to 0-1 range for easier morphing
    float normPhase = phase / juce::MathConstants<float>::twoPi;

    if (shape < 0.33f) // Sine to Triangle blend
    {
        float blend = shape / 0.33f;
        float sine = std::sin(phase);
        float triangle = (normPhase < 0.5f) ? (4.0f * normPhase - 1.0f) : (3.0f - 4.0f * normPhase);
        return sine * (1.0f - blend) + triangle * blend;
    }
    else if (shape < 0.67f) // Triangle to Square blend
    {
        float blend = (shape - 0.33f) / 0.34f;
        float triangle = (normPhase < 0.5f) ? (4.0f * normPhase - 1.0f) : (3.0f - 4.0f * normPhase);
        float square = (normPhase < 0.5f) ? 1.0f : -1.0f;
        return triangle * (1.0f - blend) + square * blend;
    }
    else // Square with soft edges
    {
        float blend = (shape - 0.67f) / 0.33f;
        float square = (normPhase < 0.5f) ? 1.0f : -1.0f;
        // Harder square wave as shape → 1.0
        return square * (1.0f + blend * 0.2f);
    }
}

void SharcTremoloAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear unused channels (FL Studio optimization)
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // FL Studio optimization: clear MIDI buffer after processing
    midiMessages.clear();

    bool isBypassed = *apvts.getRawParameterValue("BYPASS") > 0.5f;

    // FL Studio optimization: report silence to host
    if (isBypassed)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.clear(ch, 0, buffer.getNumSamples());

        isSilentOutput = true;
        return;
    }

    // Get parameter values (APVTS optimization: efficient automation)
    float rateHz = *apvts.getRawParameterValue("RATE");
    float depthPercent = *apvts.getRawParameterValue("DEPTH");
    float shape = *apvts.getRawParameterValue("SHAPE");

    // Set smoothing targets (prevents CPU spikes on automation)
    rateSmoothed.setTargetValue(rateHz);
    depthSmoothed.setTargetValue(depthPercent / 100.0f); // Convert to 0-1
    shapeSmoothed.setTargetValue(shape);

    // Use tap-tempo if recently tapped
    bool useTapTempo = (lastTapTimeMs > 0 &&
        juce::Time::getMillisecondCounterHiRes() - lastTapTimeMs < 2000);

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    // Process audio (no allocations here!)
    for (int i = 0; i < numSamples; ++i)
    {
        // Get smoothed parameter values (sample-accurate automation)
        float currentRate = useTapTempo ? tremoloRate
            : (rateSmoothed.getNextValue() * juce::MathConstants<float>::twoPi / static_cast<float>(currentSampleRate));
        float currentDepth = depthSmoothed.getNextValue();
        float currentShape = shapeSmoothed.getNextValue();

        // Generate LFO waveform with shape morphing
        float lfoOut = generateWaveform(t, currentShape);

        // Convert bipolar LFO (-1 to +1) to unipolar tremolo factor (1-depth to 1+depth)
        // This creates amplitude modulation like classic tremolo
        float tremFactor = 1.0f - (currentDepth * (0.5f * lfoOut + 0.5f));

        // Update phase
        t += currentRate;
        if (t > juce::MathConstants<float>::twoPi)
            t -= juce::MathConstants<float>::twoPi;

        // Apply tremolo to all channels (stereo or mono)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch);
            channelData[i] *= tremFactor;
        }

        // Update GUI meter (atomic, no locks)
        if (i == numSamples - 1)
            currentModDepth.store(currentDepth * std::abs(lfoOut));
    }

    // FL Studio optimization: detect and report silence
    bool isBufferSilent = true;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (buffer.getMagnitude(ch, 0, numSamples) > 0.0001f)
        {
            isBufferSilent = false;
            break;
        }
    }

    isSilentOutput = isBufferSilent;
}

juce::AudioProcessorEditor* SharcTremoloAudioProcessor::createEditor()
{
    return new SharcTremoloAudioProcessorEditor(*this);
}

void SharcTremoloAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SharcTremoloAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SharcTremoloAudioProcessor();
}