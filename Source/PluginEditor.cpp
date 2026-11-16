#include "PluginEditor.h"
#include "EditorLayout.h"

namespace
{
class ShellLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ShellLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff4f1ea));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, juce::Colour(0xfff4f1ea));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff4f1ea));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(4.0f);
        const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        bounds = bounds.withSizeKeepingCentre(size, size);
        const auto centre = bounds.getCentre();
        const float radius = size * 0.5f;
        const float thickness = juce::jmax(1.6f, size * 0.045f);
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float arcR = radius - thickness;

        g.setColour(juce::Colour(0xff2a3140));
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.strokePath(track, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(juce::Colour(0xffe6e1d6));
        juce::Path value;
        value.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
        g.strokePath(value, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path needle;
        const float len = radius * 0.55f;
        needle.startNewSubPath(centre);
        needle.lineTo(centre.x + len * std::sin(angle), centre.y - len * std::cos(angle));
        g.strokePath(needle, juce::PathStrokeType(juce::jmax(1.2f, thickness * 0.45f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool over, bool down) override
    {
        auto r = button.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(juce::Colour(down ? 0xff2c3344 : over ? 0xff222836 : 0xff161b24));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(juce::Colour(0xff343c4c));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
    }
};

juce::Point<float> fieldToScreen(float x, float y, juce::Rectangle<float> area)
{
    const auto centre = area.getCentre();
    const float scale = juce::jmin(area.getWidth(), area.getHeight()) * 0.34f;
    return { centre.x + x * scale, centre.y - y * scale };
}

void strokeZoneCircle(juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour colour)
{
    juce::Path path;
    constexpr int kSteps = 48;
    for (int i = 0; i < kSteps; i += 2)
    {
        const float a0 = juce::MathConstants<float>::twoPi * (float) i / (float) kSteps;
        const float a1 = juce::MathConstants<float>::twoPi * (float) (i + 1) / (float) kSteps;
        path.startNewSubPath(centre.x + radius * std::cos(a0), centre.y - radius * std::sin(a0));
        path.lineTo(centre.x + radius * std::cos(a1), centre.y - radius * std::sin(a1));
    }
    g.setColour(colour.withAlpha(0.45f));
    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void drawPill(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& korean, const juce::String& english, bool on)
{
    g.setColour(on ? juce::Colour(0xffd5e4f8) : juce::Colour(0xff171c26));
    g.fillRoundedRectangle(r, r.getHeight() * 0.45f);
    g.setColour(on ? juce::Colour(0xff10141c) : juce::Colour(0xffc8d0dc));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    const float koreanWidth = (float) korean.length() * 12.0f;
    auto text = r.reduced(10.0f, 0.0f);
    g.drawFittedText(korean, text.removeFromLeft(koreanWidth).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(on ? juce::Colour(0xff3c4658) : juce::Colour(0xff8b93a3));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawFittedText(english, text.toNearestInt(), juce::Justification::centredLeft, 1);
}
} // namespace

OrbitEditor::OrbitEditor(OrbitAudioProcessor& p) : juce::AudioProcessorEditor(p), proc(p)
{
    setOpaque(true);
    setName("ORBIT");
    lnf = std::make_unique<ShellLookAndFeel>();
    setLookAndFeel(lnf.get());

    makeKnob(gravity, "gravity", juce::String::fromUTF8("중력"), "GRAVITY", juce::String::fromUTF8("블랙홀의 질량"));
    makeKnob(rate, "rate", juce::String::fromUTF8("속도"), "RATE", juce::String::fromUTF8("한 바퀴 도는 빠르기"));
    makeKnob(mix, "mix", juce::String::fromUTF8("믹스"), "MIX", juce::String::fromUTF8("원음과 흔들린 소리의 비율"));

    prev.onClick = [this]
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram((proc.getCurrentProgram() + n - 1) % n);
        repaint();
    };
    next.onClick = [this]
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram((proc.getCurrentProgram() + 1) % n);
        repaint();
    };
    addAndMakeVisible(prev);
    addAndMakeVisible(next);

    juce::Random rng(20261116);
    stars.resize(90);
    for (auto& s : stars)
    {
        s.x = rng.nextFloat();
        s.y = rng.nextFloat();
        s.a = 0.12f + 0.45f * rng.nextFloat();
    }

    // setResizeLimits applies the constrainer to the still-empty bounds. Ignore that pass.
    suppressResize = true;
    setResizable(true, true);
    setResizeLimits(orbit::ui::kMinWidth, orbit::ui::heightForWidth(orbit::ui::kMinWidth),
                    orbit::ui::kMaxWidth, orbit::ui::heightForWidth(orbit::ui::kMaxWidth));
    if (auto* boundsLock = getConstrainer())
        boundsLock->setFixedAspectRatio((double) orbit::ui::kDefaultWidth / (double) orbit::ui::kDesignHeight);
    suppressResize = false;

    const int width = juce::jlimit(orbit::ui::kMinWidth, orbit::ui::kMaxWidth, proc.getUiWidth());
    setSize(width, orbit::ui::heightForWidth(width));
    startTimerHz(orbit::ui::kIdleTimerHz);
}

OrbitEditor::~OrbitEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void OrbitEditor::makeKnob(Knob& knob, const char* id, const juce::String& korean, const juce::String& english, const juce::String& hint)
{
    knob.title.setText(korean, juce::dontSendNotification);
    knob.sub.setText(english, juce::dontSendNotification);
    knob.hint.setText(hint, juce::dontSendNotification);
    for (auto* label : { &knob.title, &knob.sub, &knob.hint })
    {
        label->setJustificationType(juce::Justification::centred);
        label->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(label);
    }
    knob.title.setFont(juce::Font(juce::FontOptions(13.0f)));
    knob.title.setColour(juce::Label::textColourId, juce::Colour(0xfff4f1ea));
    knob.sub.setFont(juce::Font(juce::FontOptions(9.0f)));
    knob.sub.setColour(juce::Label::textColourId, juce::Colour(0xff8b93a3));
    knob.hint.setFont(juce::Font(juce::FontOptions(11.0f)));
    knob.hint.setColour(juce::Label::textColourId, juce::Colour(0xffa7b0c0));
    knob.hint.setMinimumHorizontalScale(0.6f);

    knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 16);
    knob.slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, knob.slider);
    addAndMakeVisible(knob.slider);
}

void OrbitEditor::placeKnob(Knob& knob, juce::Rectangle<int> cell) const
{
    const int titleH = juce::jmax(14, cell.getHeight() / 8);
    const int subH = juce::jmax(12, cell.getHeight() / 10);
    const int hintH = juce::jmax(16, cell.getHeight() / 7);
    knob.title.setBounds(cell.removeFromTop(titleH));
    knob.sub.setBounds(cell.removeFromTop(subH));
    knob.hint.setBounds(cell.removeFromBottom(hintH));
    knob.slider.setBounds(cell.reduced(2, 0));
}

int OrbitEditor::sx(int design) const
{
    return juce::roundToInt((float) design * (float) getWidth() / (float) orbit::ui::kDefaultWidth);
}

void OrbitEditor::resized()
{
    if (suppressResize)
        return;

    const int width = getWidth();
    if (width >= orbit::ui::kMinWidth && width <= orbit::ui::kMaxWidth)
        proc.setUiWidth(width);

    const int cluster = sx(222);
    const int clusterX = (getWidth() - cluster) / 2;
    prev.setBounds(clusterX, sx(14), sx(28), sx(28));
    next.setBounds(clusterX + cluster - sx(28), sx(14), sx(28), sx(28));

    const int colX = getWidth() - sx(orbit::ui::kRightMargin + orbit::ui::kRightWidth);
    const int colW = sx(orbit::ui::kRightWidth);
    const int top = sx(orbit::ui::kHeaderHeight + 16);
    const int gap = sx(8);
    const int h = juce::jmax(40, (getHeight() - top - sx(18) - 2 * gap) / 3);
    placeKnob(gravity, { colX, top, colW, h });
    placeKnob(rate, { colX, top + h + gap, colW, h });
    placeKnob(mix, { colX, top + 2 * (h + gap), colW, h });
}

void OrbitEditor::timerCallback()
{
    const float theta = proc.visTheta.load();
    const bool moving = std::abs(theta - lastTheta) > 1.0e-5f;
    lastTheta = theta;
    const int hz = moving ? orbit::ui::kActiveTimerHz : orbit::ui::kIdleTimerHz;
    if (std::abs(getTimerInterval() - (1000 / hz)) > 2)
        startTimerHz(hz);
    repaint();
}

void OrbitEditor::paint(juce::Graphics& g)
{
    paintBackdrop(g);

    const auto scene = juce::Rectangle<float>((float) sx(orbit::ui::kSceneX),
                                               (float) sx(orbit::ui::kHeaderHeight),
                                               (float) getWidth() - (float) sx(orbit::ui::kSceneX + orbit::ui::kRightMargin + orbit::ui::kRightWidth + 8),
                                               (float) getHeight() - (float) sx(orbit::ui::kHeaderHeight + 28));
    paintScene(g, scene);

    const auto readout = juce::Rectangle<float>((float) sx(orbit::ui::kLeftX),
                                                 (float) sx(orbit::ui::kHeaderHeight + 16),
                                                 (float) sx(orbit::ui::kLeftWidth),
                                                 (float) getHeight() - (float) sx(orbit::ui::kHeaderHeight + 40));
    paintReadout(g, readout);
    paintChrome(g);
}

void OrbitEditor::paintBackdrop(juce::Graphics& g) const
{
    g.fillAll(juce::Colour(0xff07090d));
    for (const auto& s : stars)
    {
        g.setColour(juce::Colours::white.withAlpha(s.a));
        g.fillEllipse(s.x * (float) getWidth(), s.y * (float) getHeight(), 1.4f, 1.4f);
    }
}

void OrbitEditor::paintScene(juce::Graphics& g, juce::Rectangle<float> area) const
{
    if (area.getWidth() < 20.0f || area.getHeight() < 20.0f)
        return;

    const float bhX = proc.visBhX.load();
    const float bhY = proc.visBhY.load();
    const float px = proc.visPx.load();
    const float py = proc.visPy.load();
    const float radius = proc.apvts.getRawParameterValue("radius")->load();
    const float ecc = proc.apvts.getRawParameterValue("ecc")->load();
    const auto bh = fieldToScreen(bhX, bhY, area);
    const float unit = juce::jmin(area.getWidth(), area.getHeight()) * 0.34f;

    const float zoneEdge[2] = { orbit::ui::kNearEdge, orbit::ui::kFarEdge };
    for (int zone = 0; zone < 2; ++zone)
        strokeZoneCircle(g, bh, unit * zoneEdge[zone] * 2.0f, juce::Colour(orbit::ui::zoneColour(zone)));

    constexpr int kSteps = 96;
    for (int i = 0; i < kSteps; ++i)
    {
        const float t0 = (float) i / (float) kSteps;
        const float t1 = (float) (i + 1) / (float) kSteps;
        const float x0 = radius * std::cos(juce::MathConstants<float>::twoPi * t0);
        const float y0 = radius * (1.0f - ecc) * std::sin(juce::MathConstants<float>::twoPi * t0);
        const float x1 = radius * std::cos(juce::MathConstants<float>::twoPi * t1);
        const float y1 = radius * (1.0f - ecc) * std::sin(juce::MathConstants<float>::twoPi * t1);
        const float d = std::hypot(x0 - bhX, y0 - bhY) * 0.5f;
        const auto p0 = fieldToScreen(x0, y0, area);
        const auto p1 = fieldToScreen(x1, y1, area);
        g.setColour(juce::Colour(orbit::ui::zoneColour(orbit::ui::zoneIndex(d))));
        g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.7f);
    }

    const float hole = juce::jmax(8.0f, unit * 0.08f);
    g.setColour(juce::Colour(0xffc6a15a).withAlpha(0.85f));
    g.drawEllipse(bh.x - hole * 1.85f, bh.y - hole * 0.42f, hole * 3.7f, hole * 0.84f, 1.2f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(bh.x - hole, bh.y - hole, hole * 2.0f, hole * 2.0f);
    g.setColour(juce::Colour(0xffe8d7a4).withAlpha(0.7f));
    g.drawEllipse(bh.x - hole * 1.15f, bh.y - hole * 1.15f, hole * 2.3f, hole * 2.3f, 1.0f);

    const auto planet = fieldToScreen(px, py, area);
    const float pr = juce::jmax(4.0f, unit * 0.035f);
    g.setColour(juce::Colour(0xffd5dde6));
    g.fillEllipse(planet.x - pr, planet.y - pr, pr * 2.0f, pr * 2.0f);
}

void OrbitEditor::paintReadout(juce::Graphics& g, juce::Rectangle<float> column) const
{
    g.setColour(juce::Colour(0xcc10141b));
    g.fillRoundedRectangle(column, 12.0f);

    const float distance = proc.visD.load();
    const float cents = proc.visCents.load();
    const int zone = orbit::ui::zoneIndex(distance);
    const juce::Colour zoneTint(orbit::ui::zoneColour(zone));
    const juce::String zoneKorean[3] = { juce::String::fromUTF8("근접"), juce::String::fromUTF8("중거리"), juce::String::fromUTF8("원거리") };
    const juce::String zoneEnglish[3] = { "EVENT HORIZON", "DRIFT", "CLEAR SPACE" };

    auto row = column.reduced(14.0f, 16.0f);
    g.setColour(juce::Colour(0xfff4f1ea));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawFittedText(juce::String::fromUTF8("피치 흔들림"), row.removeFromTop(16.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xff8b93a3));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawFittedText("PITCH WOBBLE", row.removeFromTop(12.0f).toNearestInt(), juce::Justification::centredLeft, 1);

    row.removeFromTop(6.0f);
    auto numberRow = row.removeFromTop(juce::jmax(36.0f, column.getHeight() * 0.16f));
    g.setColour(juce::Colour(0xfff7f4ee));
    g.setFont(juce::Font(juce::FontOptions(juce::jmin(54.0f, numberRow.getHeight() * 0.92f))));
    const auto number = juce::String::fromUTF8("\xC2\xB1") + juce::String(cents, 1);
    g.drawFittedText(number, numberRow.removeFromLeft(numberRow.getWidth() * 0.72f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xffc8d0dc));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawFittedText(juce::String::fromUTF8("센트"), numberRow.toNearestInt(), juce::Justification::bottomLeft, 1);

    row.removeFromTop(8.0f);
    auto chip = row.removeFromTop(26.0f).withWidth(juce::jmin(row.getWidth(), 168.0f));
    g.setColour(zoneTint.withAlpha(0.16f));
    g.fillRoundedRectangle(chip, 11.0f);
    g.setColour(zoneTint);
    g.drawRoundedRectangle(chip, 11.0f, 1.0f);
    auto chipText = chip.reduced(8.0f, 0.0f);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawFittedText(zoneKorean[zone], chipText.removeFromLeft(52.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawFittedText(zoneEnglish[zone], chipText.toNearestInt(), juce::Justification::centredLeft, 1);

    row.removeFromTop(18.0f);
    auto label = row.removeFromTop(16.0f);
    g.setColour(juce::Colour(0xffc8d0dc));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawFittedText(juce::String::fromUTF8("블랙홀까지 거리"), label.removeFromLeft(label.getWidth() * 0.62f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xfff4f1ea));
    g.drawFittedText(juce::String(distance, 2), label.toNearestInt(), juce::Justification::centredRight, 1);

    row.removeFromTop(8.0f);
    auto bar = row.removeFromTop(6.0f);
    auto fill = [&](float a, float b, juce::Colour colour)
    {
        g.setColour(colour);
        g.fillRect(bar.getX() + bar.getWidth() * a, bar.getY(), bar.getWidth() * (b - a), bar.getHeight());
    };
    fill(0.0f, orbit::ui::kNearEdge, juce::Colour(orbit::ui::kNearColour));
    fill(orbit::ui::kNearEdge, orbit::ui::kFarEdge, juce::Colour(orbit::ui::kMidColour));
    fill(orbit::ui::kFarEdge, 1.0f, juce::Colour(orbit::ui::kFarColour));

    const float marker = bar.getX() + bar.getWidth() * juce::jlimit(0.0f, 1.0f, distance);
    juce::Path triangle;
    triangle.addTriangle(marker, bar.getY() - 7.0f, marker - 4.5f, bar.getY() - 1.0f, marker + 4.5f, bar.getY() - 1.0f);
    g.setColour(juce::Colour(0xfff4f1ea));
    g.fillPath(triangle);

    row.removeFromTop(6.0f);
    auto ends = row.removeFromTop(14.0f);
    g.setColour(juce::Colour(0xff8b93a3));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawFittedText(juce::String::fromUTF8("가까움"), ends.removeFromLeft(ends.getWidth() * 0.5f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.drawFittedText(juce::String::fromUTF8("멀"), ends.toNearestInt(), juce::Justification::centredRight, 1);
}

void OrbitEditor::paintChrome(juce::Graphics& g) const
{
    const float headerH = (float) sx(orbit::ui::kHeaderHeight);
    g.setColour(juce::Colour(0xf010131a));
    g.fillRect(0.0f, 0.0f, (float) getWidth(), headerH);
    g.setColour(juce::Colour(0xff2a3140));
    g.drawHorizontalLine(sx(orbit::ui::kHeaderHeight) - 1, 0.0f, (float) getWidth());

    g.setColour(juce::Colour(0xfff7f4ee));
    g.setFont(juce::Font(juce::FontOptions(18.0f)));
    g.drawFittedText("ORBIT", juce::Rectangle<int>(sx(22), sx(8), sx(120), sx(22)), juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xff8b93a3));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawFittedText("GRAVITY WARP  ·  BEARFIELD", juce::Rectangle<int>(sx(22), sx(30), sx(220), sx(16)), juce::Justification::centredLeft, 1);

    const int cluster = sx(222);
    const int clusterX = (getWidth() - cluster) / 2;
    auto scene = juce::Rectangle<int>(clusterX + sx(32), sx(6), cluster - sx(64), sx(44));
    g.setColour(juce::Colour(0xff8b93a3));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawFittedText(juce::String::fromUTF8("장면"), scene.removeFromTop(sx(14)), juce::Justification::centred, 1);
    g.setColour(juce::Colour(0xfff4f1ea));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawFittedText(proc.getProgramName(proc.getCurrentProgram()), scene, juce::Justification::centred, 1);

    float right = (float) getWidth() - (float) sx(16);
    auto take = [&](int designWidth)
    {
        const float w = (float) sx(designWidth);
        right -= w;
        auto r = juce::Rectangle<float>(right, (float) sx(14), w, (float) sx(28));
        right -= (float) sx(8);
        return r;
    };
    drawPill(g, take(108), juce::String::fromUTF8("고급"), "ADVANCED", false);
    drawPill(g, take(96), juce::String::fromUTF8("미터"), "METER", false);
    drawPill(g, take(108), juce::String::fromUTF8("배우기"), "LEARN", false);

    auto b = take(28);
    auto a = take(28);
    g.setColour(juce::Colour(0xff171c26));
    g.fillRoundedRectangle(b, 6.0f);
    g.setColour(juce::Colour(0xffd5e4f8));
    g.fillRoundedRectangle(a, 6.0f);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(0xff10141c));
    g.drawFittedText("A", a.toNearestInt(), juce::Justification::centred, 1);
    g.setColour(juce::Colour(0xffc8d0dc));
    g.drawFittedText("B", b.toNearestInt(), juce::Justification::centred, 1);

    auto handle = juce::Rectangle<float>((float) getWidth() * 0.5f - (float) sx(36), (float) getHeight() - (float) sx(22), (float) sx(72), (float) sx(16));
    g.setColour(juce::Colour(0xff171c26));
    g.fillRoundedRectangle(handle, handle.getHeight() * 0.5f);
    g.setColour(juce::Colour(0xffc8d0dc));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawFittedText(juce::String::fromUTF8("\xE2\x96\xB2  미터"), handle.toNearestInt(), juce::Justification::centred, 1);
}
