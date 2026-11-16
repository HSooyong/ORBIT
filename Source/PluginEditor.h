#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OrbitEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OrbitEditor(OrbitAudioProcessor&);
    ~OrbitEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Label title, sub, hint;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;
    void makeKnob(Knob&, const char* id, const juce::String& korean, const juce::String& english, const juce::String& hint);
    void placeKnob(Knob&, juce::Rectangle<int>) const;
    int sx(int design) const;

    void paintBackdrop(juce::Graphics&) const;
    void paintScene(juce::Graphics&, juce::Rectangle<float>) const;
    void paintReadout(juce::Graphics&, juce::Rectangle<float>) const;
    void paintChrome(juce::Graphics&) const;

    OrbitAudioProcessor& proc;
    std::unique_ptr<juce::LookAndFeel> lnf;
    Knob gravity, rate, mix;
    juce::TextButton prev { juce::String::fromUTF8("\xE2\x80\xB9") };
    juce::TextButton next { juce::String::fromUTF8("\xE2\x80\xBA") };
    struct Star { float x, y, a; };
    std::vector<Star> stars;
    float lastTheta = -1.0f;
    bool suppressResize = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitEditor)
};
