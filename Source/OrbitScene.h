#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Orbit picture: disk, photon ring, lensed stars, zone-coloured path, planet, tail, satellites.
class OrbitScene : public juce::Component
{
public:
    explicit OrbitScene (OrbitAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    std::function<void (juce::String)> onToast;

private:
    void rebuildHole();
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> toField (juce::Point<float>) const;
    juce::Point<float> lens (juce::Point<float> p, juce::Point<float> bh, float radius) const;
    float plain (const char* id) const;
    void setPlain (const char* id, float value);
    float shadowRadius() const;

    OrbitAudioProcessor& proc;
    juce::Image hole;
    float holeKey = -1.0f;
    struct Star { float x, y, bright, size; };
    std::vector<Star> stars;
    enum class Drag { none, planet, hole } drag = Drag::none;
    bool holdWas = false, overPlanet = false, overHole = false;
    float dragDistance = 0, dragCents = 0;
    juce::Point<float> dragField;
};
