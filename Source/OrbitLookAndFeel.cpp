#include "OrbitLookAndFeel.h"

OrbitLookAndFeel::OrbitLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, panel());
    setColour (juce::ComboBox::backgroundColourId, recess());
    setColour (juce::ComboBox::outlineColourId, metal());
    setColour (juce::ComboBox::textColourId, text());
    setColour (juce::ComboBox::arrowColourId, accent());
    setColour (juce::PopupMenu::backgroundColourId, panel());
    setColour (juce::PopupMenu::textColourId, text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, metal());
    setColour (juce::Label::textColourId, muted());
    setColour (juce::Slider::textBoxTextColourId, text());
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

juce::Font OrbitLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::FontOptions (12.0f);
}

juce::Font OrbitLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::FontOptions (10.0f).withStyle ("Bold");
}

void OrbitLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float rw = radius * 2.0f;

    // Outer bezel
    juce::ColourGradient bezel (juce::Colour (0xff3a424c), centre.x, centre.y - radius,
                                juce::Colour (0xff12151a), centre.x, centre.y + radius, false);
    g.setGradientFill (bezel);
    g.fillEllipse (centre.x - radius, centre.y - radius, rw, rw);

    // Inner metal disc
    const float inner = radius * 0.78f;
    juce::ColourGradient metalGrad (juce::Colour (0xff4a5562), centre.x - inner, centre.y - inner,
                                    juce::Colour (0xff1a1f26), centre.x + inner, centre.y + inner, false);
    g.setGradientFill (metalGrad);
    g.fillEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);

    // Ring tick track
    g.setColour (juce::Colour (0xff0a0c0f));
    g.drawEllipse (centre.x - radius + 2.0f, centre.y - radius + 2.0f, rw - 4.0f, rw - 4.0f, 2.0f);

    // Value arc
    juce::Path arc;
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    arc.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f, rotaryStartAngle, toAngle, true);
    g.setColour (accent().withAlpha (slider.isEnabled() ? 0.95f : 0.4f));
    g.strokePath (arc, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer
    juce::Path pointer;
    const float pointerLen = inner * 0.72f;
    pointer.addRoundedRectangle (-1.2f, -pointerLen, 2.4f, pointerLen * 0.72f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.fillPath (pointer, juce::AffineTransform::rotation (toAngle).translated (centre.x, centre.y));

    // Center cap
    g.setColour (juce::Colour (0xff2c343e));
    g.fillEllipse (centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);
    g.setColour (accent().withAlpha (0.35f));
    g.drawEllipse (centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f, 1.0f);

    juce::ignoreUnused (slider);
}

void OrbitLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (recess());
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (isButtonDown ? accent() : metal());
    g.drawRoundedRectangle (r, 4.0f, 1.2f);

    const float arrowX = (float) width - 14.0f;
    const float arrowY = (float) height * 0.5f;
    juce::Path arrow;
    arrow.addTriangle (arrowX - 4.0f, arrowY - 2.5f, arrowX + 4.0f, arrowY - 2.5f, arrowX, arrowY + 3.5f);
    g.setColour (accent());
    g.fillPath (arrow);
    juce::ignoreUnused (box);
}

void OrbitLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (panel());
    g.setColour (metal());
    g.drawRect (0, 0, width, height, 1);
}

void OrbitLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    juce::ColourGradient grad (juce::Colour (on ? 0xff3c4652 : 0xff2e353e), r.getX(), r.getY(),
                               juce::Colour (on ? 0xff1e252d : 0xff15191e), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (on ? accent().withAlpha (0.8f) : (over || down ? juce::Colour (0xff4a5562) : metal()));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
}

void OrbitLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (b.getToggleState() ? text() : muted());
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}
