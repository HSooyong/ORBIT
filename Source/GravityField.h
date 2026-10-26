#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/** Interstellar-style play field: one black hole (lensing, thin accretion disc, photon ring),
    a draggable planet (drag = Phase / Orbit Size, temporary Hold), a draggable black hole
    (BH X/Y, wheel = Gravity). Everything is driven by the processor's telemetry atomics. */
class GravityField : public juce::Component, private juce::Timer
{
public:
    explicit GravityField (OrbitAudioProcessor&);
    ~GravityField() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    /** advance the animation n frames without a timer (offscreen snapshot) */
    void stepFrames (int n) { for (int i = 0; i < n; ++i) timerCallback(); }

private:
    void timerCallback() override;
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> toField (juce::Point<float>) const;
    juce::Point<float> lens (juce::Point<float> p, juce::Point<float> bh, float rE) const;
    void setParam (const char* id, float plainValue);
    float getParam (const char* id) const;

    OrbitAudioProcessor& proc;
    struct Star { float x, y, b, sz; };
    std::vector<Star> stars;
    juce::Image nebula;
    struct Dust { float ang, rad, life, speed; };
    std::vector<Dust> dust;
    juce::Random rng { 20260612 };
    std::array<juce::Point<float>, 48> trail {};
    int trailN = 0;
    float px = 0.7f, py = 0, bhX = 0.85f, bhY = 0, d = 1, g = 0, cents = 0, bloom = 0, theta = 0, wowHz = 0;
    float phaseClock = 0;
    enum class Drag { none, planet, hole } drag = Drag::none;
    bool holdWasOn = false, hoverPlanet = false, hoverHole = false;
};
