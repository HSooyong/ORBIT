#pragma once
#include <JuceHeader.h>
#include "engine/WarpEngine.h"
#include "engine/Presets.h"

class OrbitAudioProcessor : public juce::AudioProcessor
{
public:
    OrbitAudioProcessor();
    ~OrbitAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 7; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // GUI telemetry (audio -> GUI), for the black-hole visual (M3)
    std::atomic<float> visPx { 0 }, visPy { 0 }, visBhX { 0 }, visBhY { 0 }, visD { 1 }, visG { 0 }, visCents { 0 }, visBloom { 0 };

private:
    orbit::Params readParams() const;
    orbit::WarpEngine engine;
    int currentProgram = 0;
    std::vector<std::atomic<float>*> raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitAudioProcessor)
};
