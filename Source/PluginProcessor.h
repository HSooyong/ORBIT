#pragma once
#include <JuceHeader.h>
#include "EditorLayout.h"
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

    struct UiFlags
    {
        int width = orbit::ui::kDefaultWidth;
        bool learn = false, meter = false, advanced = false, zones = false;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;

    // GUI telemetry. Depths and level-match are copied out; the sample loop does not read them back.
    std::atomic<float> visPx { 0 }, visPy { 0 }, visBhX { 0 }, visBhY { 0 }, visD { 1 }, visG { 0 }, visCents { 0 }, visBloom { 0 }, visTheta { 0 }, visWowHz { 0 }, visInstantCents { 0 };
    std::atomic<float> visCWow { 0 }, visCFlutter { 0 }, visCChorus { 0 }, visFFlutter { 0 }, visLevelMatch { 1 }, visInPower { 0 }, visOutPower { 0 };

    UiFlags getUiFlags() const;
    void setUiFlags (UiFlags);
    void toggleAB();
    bool isSlotB() const { return slotBActive; }
    void noteGesture (const juce::String& line);
    juce::StringArray history() const { return recent; }

private:
    juce::ValueTree slotA, slotB;
    bool slotBActive = false;
    juce::StringArray recent;
    UiFlags uiFlags;
    orbit::Params readParams() const;
    orbit::WarpEngine engine;
    int currentProgram = 0;
    std::vector<std::atomic<float>*> raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitAudioProcessor)
};
