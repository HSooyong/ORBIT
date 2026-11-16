#pragma once
#include <JuceHeader.h>
#include "OrbitScene.h"
#include "PluginProcessor.h"

class OrbitEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OrbitEditor (OrbitAudioProcessor&);
    ~OrbitEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void setModes (bool learn, bool meter, bool advanced, bool zones);
    void captureFrame() { timerCallback(); }

private:
    struct Knob
    {
        juce::Label title, sub, hint;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct Row
    {
        juce::Label korean, english;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;
    void refreshLayout();
    void showToast (juce::String);
    void toggle (bool OrbitAudioProcessor::UiFlags::* flag);
    juce::Font fontFor (float px, bool korean, bool mono) const;
    void drawBox (juce::Graphics&, juce::Rectangle<float>, juce::String, float px, bool korean, bool mono, juce::Justification) const;
    juce::Rectangle<float> findBox (const char* name, int nth = 0) const;

    OrbitAudioProcessor& proc;
    juce::Typeface::Ptr korean, sans, mono;
    std::unique_ptr<juce::LookAndFeel> lnf;
    OrbitScene scene;
    Knob knobs[3];
    juce::TextButton prev { "<" }, next { ">" }, slotA { "A" }, slotB { "B" }, undo { "U" };
    juce::TextButton zones { "ZONES" }, learn { "LEARN" }, meter { "METER" }, advanced { "ADV" }, menu { "..." };
    juce::Viewport advancedPort;
    juce::Component advancedBody;
    std::vector<std::unique_ptr<Row>> rows;
    orbit::ui::Layout layout;
    std::array<float, 480> trace {};
    std::array<int, 480> traceZone {};
    int traceCount = 0;
    juce::String toast;
    double toastUntil = 0;
    int lastZone = -1;
    bool capNoted = false;
    bool suppress = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitEditor)
};
