#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrbitLookAndFeel.h"
#include "GravityField.h"

class OrbitEditor : public juce::AudioProcessorEditor
{
public:
    explicit OrbitEditor (OrbitAudioProcessor&);
    ~OrbitEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setAdvancedOpen (bool);
    GravityField& getField() { return field; }

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    struct Knob { juce::Slider s; juce::Label l; std::unique_ptr<SA> a; };
    struct Combo { juce::ComboBox c; juce::Label l; std::unique_ptr<CA> a; };
    Knob* knob (const char* id, const char* label);
    Combo* combo (const char* id, const char* label);
    void layoutKnobs (std::vector<Knob*>&, juce::Rectangle<int> area, int cols);

    OrbitAudioProcessor& proc;
    OrbitLookAndFeel lnf;
    GravityField field;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Combo>> combos;
    std::vector<Knob*> left, right, adv;
    Combo *sync = nullptr, *stages = nullptr, *voices = nullptr;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, holdBtn { "HOLD" }, keplerBtn { "KEPLER" }, advBtn { "ADVANCED" };
    std::unique_ptr<BA> holdAtt, keplerAtt;
    bool advOpen = false;
    juce::Rectangle<int> leftPanel, rightPanel, advPanel, header;
};
