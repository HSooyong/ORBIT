#pragma once
#include <JuceHeader.h>

/** Brushed-metal L&F inspired by v0.3 / TILT density; M2: steel accent, no neon. */
class OrbitLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrbitLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;

    static juce::Colour panel()   { return juce::Colour (0xff161a20); }
    static juce::Colour recess()  { return juce::Colour (0xff0b0d10); }
    static juce::Colour metal()   { return juce::Colour (0xff2a3038); }
    static juce::Colour accent()  { return juce::Colour (0xff86aecb); }
    static juce::Colour text()    { return juce::Colour (0xffd4dbe3); }
    static juce::Colour muted()   { return juce::Colour (0xff8a95a3); }
};
