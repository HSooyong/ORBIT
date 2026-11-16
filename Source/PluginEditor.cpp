#include "PluginEditor.h"
#include "EditorLayout.h"
#include <BinaryData.h>
#include <cstring>
#include <map>

namespace
{
juce::Rectangle<float> asRect (orbit::ui::R r) { return { r.x, r.y, r.w, r.h }; }

bool containsHangul (const juce::String& text)
{
    for (int i = 0; i < text.length(); ++i)
        if (text[i] >= 0xAC00 && text[i] <= 0xD7A3) return true;
    return false;
}

class ShellLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit ShellLookAndFeel (juce::Typeface::Ptr koreanIn, juce::Typeface::Ptr sansIn)
        : korean (std::move (koreanIn)), sans (std::move (sansIn))
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xfff4f1ea));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colour (0xfff4f1ea));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe7edf5));
    }

    juce::Font getTextButtonFont (juce::TextButton& button, int height) override
    {
        const bool hangul = containsHangul (button.getButtonText());
        auto face = hangul ? korean : sans;
        const float px = juce::jlimit (9.0f, 14.0f, height * 0.42f);
        if (face == nullptr) return juce::Font (juce::FontOptions (px));
        return juce::Font (juce::FontOptions (face).withHeight (px));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool over, bool down) override
    {
        auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        g.setColour (on ? juce::Colour (0xffd5e4f8) : juce::Colour (down ? 0xff2a3344 : over ? 0xff222836 : 0xff171c26));
        g.fillRoundedRectangle (r, juce::jmin (8.0f, r.getHeight() * 0.4f));
        if (on) return;
        g.setColour (juce::Colour (0xff343c4c));
        g.drawRoundedRectangle (r, juce::jmin (8.0f, r.getHeight() * 0.4f), 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.setColour (button.getToggleState() ? juce::Colour (0xff121820) : juce::Colour (0xffe7edf5));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 0), juce::Justification::centred, 1);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos, float start, float end, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (3.0f);
        const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        bounds = bounds.withSizeKeepingCentre (size, size);
        const auto c = bounds.getCentre();
        const float radius = size * 0.5f - 2.0f;
        const float angle = start + pos * (end - start);
        g.setColour (juce::Colour (0xff2c3548));
        juce::Path track;
        track.addCentredArc (c.x, c.y, radius, radius, 0.0f, start, end, true);
        g.strokePath (track, juce::PathStrokeType (juce::jmax (2.0f, size * 0.06f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff8ec8ff));
        juce::Path value;
        value.addCentredArc (c.x, c.y, radius, radius, 0.0f, start, angle, true);
        g.strokePath (value, juce::PathStrokeType (juce::jmax (2.0f, size * 0.06f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.x, c.y, c.x + std::sin (angle) * radius * 0.62f, c.y - std::cos (angle) * radius * 0.62f, 1.6f);
    }

    juce::Typeface::Ptr korean, sans;
};

const char* advancedIds[] = {
    "gravity", "rate", "mix", "sync", "shape", "stages", "ecc", "kepler", "hold",
    "horizon", "bloom", "bh_x", "bh_y", "phase", "radius", "warp", "flutter", "chaos",
    "floor", "voices", "width", "feedback", "tone", "drive", "output" };

juce::String liveText (OrbitAudioProcessor& proc, const char* name, int nth, const char* fallback)
{
    const float d = proc.visD.load();
    const int zone = orbit::ui::zoneIndex (d);
    const char* zoneKo[] = { "근접", "중거리", "원거리" };
    const char* zoneEn[] = { "EVENT HORIZON", "DRIFT", "CLEAR SPACE" };
    if (std::strcmp (name, "big number") == 0) return juce::String::fromUTF8 ("\xC2\xB1") + juce::String (proc.visCents.load(), 1);
    if (std::strcmp (name, "zone chip") == 0)
    {
        if (proc.getUiFlags().zones)
        {
            const char* tileKo[] = { "원거리", "중거리", "근접" };
            const char* tileEn[] = { "CLEAR SPACE", "DRIFT", "EVENT HORIZON" };
            const int index = juce::jlimit (0, 2, nth);
            return juce::String::fromUTF8 (tileKo[index]) + "   " + tileEn[index];
        }
        return juce::String::fromUTF8 (zoneKo[zone]) + "   " + zoneEn[zone];
    }
    if (std::strcmp (name, "distance value") == 0) return juce::String (d, 2);
    if (std::strcmp (name, "scene name") == 0) return proc.getProgramName (proc.getCurrentProgram());
    if (std::strcmp (name, "meter line") == 0)
        return "거리 " + juce::String (d, 2) + " · 끌림 " + juce::String (juce::roundToInt (proc.visG.load() * 100.0f))
               + "% · ±" + juce::String (proc.visCents.load(), 1) + " ct · 와우 " + juce::String (proc.visWowHz.load(), 2) + " Hz";
    const auto flowValue = [&]() -> juce::String
    {
        const float g = proc.visG.load();
        switch (nth)
        {
            case 0: return "궤도 각도  " + juce::String (juce::roundToInt (proc.visTheta.load() * 360.0f)) + juce::String::fromUTF8 ("\xc2\xb0");
            case 1: return "거리  " + juce::String (d, 2);
            case 2: return "중력 끌림  " + juce::String (juce::roundToInt (g * 100.0f)) + " %";
            case 3: return "지연  " + juce::String (7.0f + 8.0f * g, 1) + " ms";
            default: return "피치  ±" + juce::String (proc.visCents.load(), 1) + " ct";
        }
    };
    if (std::strcmp (name, "flow row") == 0) return flowValue();
    const float distances[] = { 0.80f, 0.30f, 0.10f };
    const auto preset = orbit::factoryPresets()[0].p;
    const auto mapped = orbit::mapModulation (distances[juce::jlimit (0, 2, nth)], preset.gravity, preset.horizon, preset.warp, preset.flutter, preset.floor, preset.chaos);
    if (std::strcmp (name, "zone number") == 0)
        return juce::String::fromUTF8 ("\xC2\xB1") + juce::String (mapped.cWow + mapped.cFlutter + mapped.cChorus, 1);
    if (std::strcmp (name, "zone wow") == 0)
        return "와우 ±" + juce::String (mapped.cWow, 1) + " ct · " + juce::String (mapped.fWow, 2) + " Hz";
    if (std::strcmp (name, "zone flutter") == 0)
        return "플러터 ±" + juce::String (mapped.cFlutter, 2) + " ct · " + juce::String (mapped.fFlutter, 1) + " Hz";
    if (std::strcmp (name, "zone chorus") == 0)
        return "코러스 ±" + juce::String (mapped.cChorus, 1) + " ct · 보이스 " + juce::String (preset.voices);
    if (std::strcmp (name, "part wow") == 0) return "와우 깊이  ±" + juce::String (proc.visCWow.load(), 1) + " ct";
    if (std::strcmp (name, "part wow hz") == 0) return "와우 속도  " + juce::String (proc.visWowHz.load(), 2) + " Hz";
    if (std::strcmp (name, "part fl") == 0) return "플러터 깊이  ±" + juce::String (proc.visCFlutter.load(), 2) + " ct";
    if (std::strcmp (name, "part fl hz") == 0) return "플러터 속도  " + juce::String (proc.visFFlutter.load(), 1) + " Hz";
    if (std::strcmp (name, "part ch") == 0) return "코러스 깊이  ±" + juce::String (proc.visCChorus.load(), 1) + " ct";
    if (std::strcmp (name, "part voices") == 0)
        return "보이스  " + juce::String ((int) proc.apvts.getRawParameterValue ("voices")->load() + 1.0f);
    auto db = [] (float power) { return power > 1.0e-8f ? juce::String (10.0f * std::log10 (power), 1) : juce::String ("-60.0"); };
    if (std::strcmp (name, "in value") == 0) return db (proc.visInPower.load()) + " dB";
    if (std::strcmp (name, "out value") == 0) return db (proc.visOutPower.load()) + " dB";
    if (std::strcmp (name, "match value") == 0)
    {
        const float gain = juce::Decibels::gainToDecibels (proc.visLevelMatch.load(), -6.0f);
        return (gain >= 0 ? "+" : "") + juce::String (gain, 1) + " dB";
    }
    if (std::strcmp (name, "wow value") == 0) return juce::String::fromUTF8 ("\xC2\xB1") + juce::String (proc.visCWow.load(), 1);
    if (std::strcmp (name, "flutter value") == 0) return juce::String::fromUTF8 ("\xC2\xB1") + juce::String (proc.visCFlutter.load(), 2);
    if (std::strcmp (name, "chorus value") == 0) return juce::String::fromUTF8 ("\xC2\xB1") + juce::String (proc.visCChorus.load(), 1);
    return fallback != nullptr ? juce::String::fromUTF8 (fallback) : juce::String();
}

bool skipPaint (const char* name)
{
    const char* skip[] = { "menu", "advanced", "meter", "learn", "zones", "undo", "A", "B", "scene prev", "scene next",
                           "knob ko", "knob en", "knob value", "knob hint", "adv ko", "adv en", "adv value" };
    for (const char* s : skip)
        if (std::strcmp (name, s) == 0) return true;
    return false;
}

void paintDepth (juce::Graphics& g, juce::Rectangle<float> area, const orbit::Params& p, float distance)
{
    if (area.getWidth() < 8.0f) return;
    g.setColour (juce::Colour (0x33101822));
    g.fillRoundedRectangle (area, 6.0f);
    juce::Path path;
    float peak = 1.0f;
    for (int i = 0; i <= 64; ++i)
    {
        const float d = (float) i / 64.0f;
        const auto m = orbit::mapModulation (d, p.gravity, p.horizon, p.warp, p.flutter, p.floor, p.chaos);
        peak = std::max (peak, m.cWow + m.cFlutter + m.cChorus);
    }
    for (int i = 0; i <= 64; ++i)
    {
        const float d = (float) i / 64.0f;
        const auto m = orbit::mapModulation (d, p.gravity, p.horizon, p.warp, p.flutter, p.floor, p.chaos);
        const float cents = m.cWow + m.cFlutter + m.cChorus;
        const float x = area.getX() + area.getWidth() * d;
        const float y = area.getBottom() - 6.0f - (area.getHeight() - 12.0f) * (cents / peak);
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    g.setColour (juce::Colour (0xffe7edf5));
    g.strokePath (path, juce::PathStrokeType (1.6f));
    const float x = area.getX() + area.getWidth() * juce::jlimit (0.0f, 1.0f, distance);
    g.setColour (juce::Colour (0xffffd27a));
    g.drawLine (x, area.getY() + 4.0f, x, area.getBottom() - 4.0f, 1.0f);
}

void paintOrbitCurve (juce::Graphics& g, juce::Rectangle<float> area, const orbit::Params& p, float theta)
{
    if (area.getWidth() < 8.0f) return;
    g.setColour (juce::Colour (0x33101822));
    g.fillRoundedRectangle (area, 6.0f);
    std::array<double, 257> cum {};
    orbit::fillKepler (p.ecc, cum.data(), 256);
    juce::Path path;
    float peak = 1.0f;
    std::array<float, 65> cents {};
    for (int i = 0; i <= 64; ++i)
    {
        const double angle = p.kepler ? orbit::keplerAngle (cum.data(), 256, (double) i / 64.0) : (double) i / 64.0;
        const float x = p.radius * std::cos ((float) (angle * orbit::kTwoPi));
        const float y = p.radius * (1.0f - p.ecc) * std::sin ((float) (angle * orbit::kTwoPi));
        const float d = juce::jlimit (0.0f, 1.0f, std::hypot (x - p.bhX, y - p.bhY) * 0.5f);
        const auto m = orbit::mapModulation (d, p.gravity, p.horizon, p.warp, p.flutter, p.floor, p.chaos);
        cents[(size_t) i] = m.cWow + m.cFlutter + m.cChorus;
        peak = std::max (peak, cents[(size_t) i]);
    }
    for (int i = 0; i <= 64; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / 64.0f;
        const float y = area.getBottom() - 4.0f - (area.getHeight() - 8.0f) * (cents[(size_t) i] / peak);
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    g.setColour (juce::Colour (0xff9fd8c8));
    g.strokePath (path, juce::PathStrokeType (1.4f));
    const float mark = area.getX() + area.getWidth() * juce::jlimit (0.0f, 1.0f, theta);
    g.setColour (juce::Colour (0xffffd27a));
    g.drawLine (mark, area.getY() + 3.0f, mark, area.getBottom() - 3.0f, 1.0f);
}
} // namespace

OrbitEditor::OrbitEditor (OrbitAudioProcessor& processorIn)
    : juce::AudioProcessorEditor (processorIn), proc (processorIn), scene (processorIn)
{
    korean = juce::Typeface::createSystemTypefaceFor (BinaryData::NotoSansKRRegular_otf, BinaryData::NotoSansKRRegular_otfSize);
    sans = juce::Typeface::createSystemTypefaceFor (BinaryData::DMSansRegular_ttf, BinaryData::DMSansRegular_ttfSize);
    mono = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexMonoRegular_ttf, BinaryData::IBMPlexMonoRegular_ttfSize);
    lnf = std::make_unique<ShellLookAndFeel> (korean, sans);
    setLookAndFeel (lnf.get());
    setOpaque (true);
    setWantsKeyboardFocus (true);

    const char* ids[] = { "gravity", "rate", "mix" };
    const juce::String titles[] = { juce::String::fromUTF8 ("중력"), juce::String::fromUTF8 ("속도"), juce::String::fromUTF8 ("믹스") };
    const char* ens[] = { "GRAVITY", "RATE", "MIX" };
    const juce::String hints[] = { juce::String::fromUTF8 ("블랙홀의 질량"), juce::String::fromUTF8 ("한 바퀴 도는 빠르기"), juce::String::fromUTF8 ("원음과 흔들린 소리의 비율") };
    for (int i = 0; i < 3; ++i)
    {
        knobs[i].title.setText (titles[i], juce::dontSendNotification);
        knobs[i].sub.setText (ens[i], juce::dontSendNotification);
        knobs[i].hint.setText (hints[i], juce::dontSendNotification);
        for (auto* label : { &knobs[i].title, &knobs[i].sub, &knobs[i].hint })
        {
            label->setJustificationType (juce::Justification::centred);
            label->setInterceptsMouseClicks (false, false);
            if (korean != nullptr) label->setFont (juce::Font (juce::FontOptions (korean).withHeight (12.0f)));
            addAndMakeVisible (label);
        }
        knobs[i].sub.setFont (juce::Font (juce::FontOptions (sans != nullptr ? sans : korean).withHeight (9.0f)));
        knobs[i].sub.setColour (juce::Label::textColourId, juce::Colour (0xff8b93a3));
        knobs[i].hint.setColour (juce::Label::textColourId, juce::Colour (0xffa7b0c0));
        knobs[i].slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knobs[i].slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 16);
        knobs[i].slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        knobs[i].slider.setWantsKeyboardFocus (true);
        knobs[i].slider.addMouseListener (this, false);
        knobs[i].attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], knobs[i].slider);
        addAndMakeVisible (knobs[i].slider);
    }
    knobs[0].slider.onDragEnd = [this]
    {
        showToast ("중력 " + juce::String (juce::roundToInt (knobs[0].slider.getValue())) + " % · 흔들림 ±" + juce::String (proc.visCents.load(), 1) + " 센트");
    };

    auto wire = [this] (juce::TextButton& button, auto flag)
    {
        button.setClickingTogglesState (true);
        button.onClick = [this, flag, &button] { if (! suppress) { auto next = proc.getUiFlags(); next.*flag = button.getToggleState(); proc.setUiFlags (next); refreshLayout(); } };
        addAndMakeVisible (button);
    };
    zones.setButtonText (juce::String::fromUTF8 ("구역 비교 ZONES"));
    learn.setButtonText (juce::String::fromUTF8 ("배우기 LEARN"));
    meter.setButtonText (juce::String::fromUTF8 ("미터 METER"));
    advanced.setButtonText (juce::String::fromUTF8 ("고급 ADVANCED"));
    wire (zones, &OrbitAudioProcessor::UiFlags::zones);
    wire (learn, &OrbitAudioProcessor::UiFlags::learn);
    wire (meter, &OrbitAudioProcessor::UiFlags::meter);
    wire (advanced, &OrbitAudioProcessor::UiFlags::advanced);

    slotA.onClick = [this] { if (proc.isSlotB()) proc.toggleAB(); refreshLayout(); };
    slotB.onClick = [this] { if (! proc.isSlotB()) proc.toggleAB(); refreshLayout(); };
    undo.onClick = [this] { proc.undo.undo(); };
    prev.onClick = [this]
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n);
        showToast ("장면 " + proc.getProgramName (proc.getCurrentProgram()) + juce::String::fromUTF8 ("으로 바꿨습니다"));
    };
    next.onClick = [this]
    {
        proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms());
        showToast ("장면 " + proc.getProgramName (proc.getCurrentProgram()) + juce::String::fromUTF8 ("으로 바꿨습니다"));
    };
    menu.onClick = [this]
    {
        juce::PopupMenu popup;
        popup.addItem (1, juce::String::fromUTF8 ("작게  1100"));
        popup.addItem (2, juce::String::fromUTF8 ("기본  1280"));
        popup.addItem (3, juce::String::fromUTF8 ("크게  1760"));
        popup.addSeparator();
        popup.addItem (4, juce::String::fromUTF8 ("키보드  L 배우기  M 미터  A 고급  Z 구역"));
        popup.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menu), [this] (int result)
        {
            auto next = proc.getUiFlags();
            if (result == 1) next.width = 1100;
            if (result == 2) next.width = 1280;
            if (result == 3) next.width = 1760;
            if (result >= 1 && result <= 3) { proc.setUiFlags (next); setSize (next.width, orbit::ui::heightForWidth (next.width)); }
        });
    };
    for (auto* button : { &prev, &next, &slotA, &slotB, &undo, &menu }) addAndMakeVisible (button);

    for (const char* id : advancedIds)
    {
        auto row = std::make_unique<Row>();
        row->slider.setSliderStyle (juce::Slider::LinearHorizontal);
        row->slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 16);
        row->slider.setWantsKeyboardFocus (true);
        if (auto* parameter = proc.apvts.getParameter (id))
            row->slider.textFromValueFunction = [parameter] (double) { return parameter->getCurrentValueAsText(); };
        row->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, row->slider);
        row->korean.setInterceptsMouseClicks (false, false);
        row->english.setInterceptsMouseClicks (false, false);
        advancedBody.addAndMakeVisible (row->korean);
        advancedBody.addAndMakeVisible (row->english);
        advancedBody.addAndMakeVisible (row->slider);
        rows.push_back (std::move (row));
    }
    advancedPort.setViewedComponent (&advancedBody, false);
    advancedPort.setScrollBarsShown (true, false);
    addAndMakeVisible (advancedPort);

    scene.onToast = [this] (juce::String text) { showToast (std::move (text)); };
    addAndMakeVisible (scene);

    const auto saved = proc.getUiFlags();
    suppress = true;
    setResizable (true, true);
    setResizeLimits (orbit::ui::kMinWidth, orbit::ui::heightForWidth (orbit::ui::kMinWidth), orbit::ui::kMaxWidth, orbit::ui::heightForWidth (orbit::ui::kMaxWidth));
    if (auto* bounds = getConstrainer())
        bounds->setFixedAspectRatio ((double) orbit::ui::kDefaultWidth / (double) orbit::ui::kDesignHeight);
    suppress = false;
    setSize (saved.width, orbit::ui::heightForWidth (saved.width));
    startTimerHz (orbit::ui::kIdleTimerHz);
}

OrbitEditor::~OrbitEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void OrbitEditor::setModes (bool learnOn, bool meterOn, bool advancedOn, bool zonesOn)
{
    auto flags = proc.getUiFlags();
    flags.learn = learnOn;
    flags.meter = meterOn;
    flags.advanced = advancedOn;
    flags.zones = zonesOn;
    proc.setUiFlags (flags);
    refreshLayout();
}

void OrbitEditor::toggle (bool OrbitAudioProcessor::UiFlags::* flag)
{
    auto flags = proc.getUiFlags();
    flags.*flag = ! (flags.*flag);
    proc.setUiFlags (flags);
    refreshLayout();
}

bool OrbitEditor::keyPressed (const juce::KeyPress& key)
{
    const auto character = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    if (character == 'l') { toggle (&OrbitAudioProcessor::UiFlags::learn); return true; }
    if (character == 'm') { toggle (&OrbitAudioProcessor::UiFlags::meter); return true; }
    if (character == 'a') { toggle (&OrbitAudioProcessor::UiFlags::advanced); return true; }
    if (character == 'z') { toggle (&OrbitAudioProcessor::UiFlags::zones); return true; }
    return false;
}

void OrbitEditor::showToast (juce::String text)
{
    toast = std::move (text);
    toastUntil = juce::Time::getMillisecondCounterHiRes() + 2500.0;
    proc.noteGesture (toast);
    refreshLayout();
}

juce::Font OrbitEditor::fontFor (float px, bool hangul, bool useMono) const
{
    auto face = hangul ? korean : (useMono ? mono : sans);
    if (face == nullptr) return juce::Font (juce::FontOptions (px));
    return juce::Font (juce::FontOptions (face).withHeight (px));
}

void OrbitEditor::drawBox (juce::Graphics& g, juce::Rectangle<float> area, juce::String text, float px, bool hangul, bool useMono, juce::Justification justification) const
{
    g.setFont (fontFor (px, hangul, useMono));
    g.drawFittedText (text, area.toNearestInt(), justification, 3);
}

juce::Rectangle<float> OrbitEditor::findBox (const char* name, int nth) const
{
    int seen = 0;
    for (const auto& item : layout.items)
        if (std::strcmp (item.name, name) == 0 && seen++ == nth)
            return asRect (item.box);
    return {};
}

void OrbitEditor::refreshLayout()
{
    if (suppress) return;
    auto flags = proc.getUiFlags();
    const bool toastOn = juce::Time::getMillisecondCounterHiRes() < toastUntil && toast.isNotEmpty();
    layout = orbit::ui::layoutFor (getWidth(), { flags.learn, flags.meter, flags.advanced, flags.zones, toastOn });
    auto place = [&] (juce::Component& component, const char* name, int nth = 0)
    {
        const auto area = findBox (name, nth);
        component.setBounds (area.toNearestInt());
        component.setVisible (! area.isEmpty());
    };
    place (prev, "scene prev");
    place (next, "scene next");
    place (slotA, "A");
    place (slotB, "B");
    place (undo, "undo");
    place (zones, "zones");
    place (learn, "learn");
    place (meter, "meter");
    place (advanced, "advanced");
    place (menu, "menu");
    suppress = true;
    zones.setToggleState (flags.zones, juce::dontSendNotification);
    learn.setToggleState (flags.learn, juce::dontSendNotification);
    meter.setToggleState (flags.meter, juce::dontSendNotification);
    advanced.setToggleState (flags.advanced, juce::dontSendNotification);
    slotA.setToggleState (! proc.isSlotB(), juce::dontSendNotification);
    slotB.setToggleState (proc.isSlotB(), juce::dontSendNotification);
    suppress = false;

    const bool knobsOn = ! flags.zones && ! flags.advanced;
    for (int i = 0; i < 3; ++i)
    {
        const auto cell = asRect (layout.knob[i]);
        knobs[i].slider.setVisible (knobsOn);
        knobs[i].title.setVisible (knobsOn);
        knobs[i].sub.setVisible (knobsOn);
        knobs[i].hint.setVisible (knobsOn && ! flags.meter);
        if (! knobsOn) continue;
        knobs[i].title.setBounds (findBox ("knob ko", i).toNearestInt());
        knobs[i].sub.setBounds (findBox ("knob en", i).toNearestInt());
        knobs[i].hint.setBounds (findBox ("knob hint", i).toNearestInt());
        auto slider = cell;
        slider.removeFromTop (30.0f * getWidth() / 1280.0f);
        if (! flags.meter) slider.removeFromBottom (28.0f * getWidth() / 1280.0f);
        knobs[i].slider.setBounds (slider.toNearestInt());
        knobs[i].title.setFont (fontFor (13.0f * getHeight() / 660.0f, true, false));
        knobs[i].sub.setFont (fontFor (9.0f * getHeight() / 660.0f, false, false));
        knobs[i].hint.setFont (fontFor (11.0f * getHeight() / 660.0f, true, false));
    }
    scene.setVisible (! flags.zones);
    scene.setBounds (asRect (layout.scene).toNearestInt());
    advancedPort.setVisible (flags.advanced && ! flags.zones);
    advancedPort.setBounds (asRect (layout.advancedView).toNearestInt());
    const int rowH = 20;
    advancedBody.setSize (juce::jmax (180, advancedPort.getWidth() - 8), 25 * rowH + 8);
    for (int i = 0; i < (int) rows.size() && i < (int) layout.advancedRows.size() / 3; ++i)
    {
        const auto& ko = layout.advancedRows[(size_t) i * 3];
        const auto& en = layout.advancedRows[(size_t) i * 3 + 1];
        const auto& value = layout.advancedRows[(size_t) i * 3 + 2];
        const float scale = advancedBody.getWidth() / (float) (orbit::ui::kAdvancedWidth - 36);
        rows[(size_t) i]->korean.setText (juce::String::fromUTF8 (ko.text), juce::dontSendNotification);
        rows[(size_t) i]->english.setText (en.text, juce::dontSendNotification);
        rows[(size_t) i]->korean.setBounds ((int) (ko.box.x * scale), (int) (ko.box.y * scale), (int) (ko.box.w * scale), rowH);
        rows[(size_t) i]->english.setBounds ((int) (en.box.x * scale), (int) (en.box.y * scale), (int) (en.box.w * scale), rowH);
        rows[(size_t) i]->slider.setBounds ((int) ((en.box.x + en.box.w + 4) * scale), (int) (value.box.y * scale),
                                            (int) ((value.box.x - en.box.x - en.box.w - 8) * scale) + (int) (value.box.w * scale), rowH);
        rows[(size_t) i]->korean.setFont (fontFor (12.0f * scale, true, false));
        rows[(size_t) i]->english.setFont (fontFor (10.0f * scale, false, false));
        rows[(size_t) i]->english.setColour (juce::Label::textColourId, juce::Colour (0xff8b93a3));
    }
    repaint();
}

void OrbitEditor::resized()
{
    if (suppress) return;
    auto flags = proc.getUiFlags();
    if (getWidth() >= orbit::ui::kMinWidth && getWidth() <= orbit::ui::kMaxWidth)
    {
        flags.width = getWidth();
        proc.setUiFlags (flags);
    }
    refreshLayout();
}

void OrbitEditor::timerCallback()
{
    const float theta = proc.visTheta.load();
    const int zone = orbit::ui::zoneIndex (proc.visD.load());
    if (lastZone >= 0 && zone != lastZone)
    {
        const char* line[] = { "근접 구역에 들어왔습니다 · 빠른 떨림이 더해집니다",
                               "중거리 구역에 들어왔습니다 · 느린 흔들림이 커집니다",
                               "원거리 구역에 들어왔습니다 · 흔들림이 옅어집니다" };
        showToast (juce::String::fromUTF8 (line[zone]));
    }
    lastZone = zone;
    if (proc.visCents.load() >= 99.5f)
    {
        if (! capNoted) showToast (juce::String::fromUTF8 ("피치 상한 100센트에 닿았습니다 · 더 깊어지지 않습니다"));
        capNoted = true;
    }
    else if (proc.visCents.load() < 98.0f) capNoted = false;

    const bool moving = std::abs (theta - trace[(size_t) (traceCount % 480)]) > 1.0e-4f || proc.getUiFlags().meter;
    const int hz = moving ? orbit::ui::kActiveTimerHz : orbit::ui::kIdleTimerHz;
    if (std::abs (getTimerInterval() - (1000 / hz)) > 2) startTimerHz (hz);
    trace[(size_t) (traceCount % 480)] = proc.visInstantCents.load();
    traceZone[(size_t) (traceCount % 480)] = zone;
    ++traceCount;
    if (toast.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() >= toastUntil)
    {
        toast.clear();
        refreshLayout();
    }
    for (auto& knob : knobs)
        knob.slider.setMouseDragSensitivity (juce::ModifierKeys::currentModifiers.isShiftDown() ? 900 : 280);
    repaint();
}

void OrbitEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff07090d));
    if (proc.getUiFlags().zones)
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (juce::Colour (0xff10141c));
            g.fillRoundedRectangle (asRect (layout.zoneTile[i]), 10.0f);
        }
    std::map<std::string, int> seen;
    for (const auto& item : layout.items)
    {
        if (item.text == nullptr || skipPaint (item.name)) continue;
        const int nth = seen[item.name]++;
        const auto text = liveText (proc, item.name, nth, item.text);
        const bool hangul = containsHangul (text);
        const bool number = std::strcmp (item.name, "big number") == 0 || std::strcmp (item.name, "zone number") == 0;
        g.setColour (std::strcmp (item.name, "big number") == 0 || std::strcmp (item.name, "zone number") == 0 ? juce::Colour (0xfff7f4ee) : juce::Colour (0xffd5dde8));
        if (std::strcmp (item.name, "toast") == 0)
        {
            g.setColour (juce::Colour (0xf0181d27));
            g.fillRoundedRectangle (asRect (item.box), 8.0f);
            g.setColour (juce::Colour (0xfff4f1ea));
            drawBox (g, asRect (item.box).reduced (8.0f, 4.0f), toast, item.px, true, false, juce::Justification::centredLeft);
            continue;
        }
        drawBox (g, asRect (item.box), std::strcmp (item.name, "toast") == 0 ? toast : text, item.px, hangul || number, number, juce::Justification::centredLeft);
    }

    orbit::Params p = orbit::factoryPresets()[(size_t) juce::jlimit (0, 6, proc.getCurrentProgram())].p;
    p.gravity = proc.apvts.getRawParameterValue ("gravity")->load() * 0.01f;
    p.horizon = proc.apvts.getRawParameterValue ("horizon")->load();
    p.warp = proc.apvts.getRawParameterValue ("warp")->load() * 0.01f;
    p.flutter = proc.apvts.getRawParameterValue ("flutter")->load() * 0.01f;
    p.floor = proc.apvts.getRawParameterValue ("floor")->load() * 0.01f;
    p.chaos = proc.apvts.getRawParameterValue ("chaos")->load() * 0.01f;
    p.radius = proc.apvts.getRawParameterValue ("radius")->load();
    p.ecc = proc.apvts.getRawParameterValue ("ecc")->load();
    p.bhX = proc.visBhX.load();
    p.bhY = proc.visBhY.load();
    p.kepler = proc.apvts.getRawParameterValue ("kepler")->load() > 0.5f;
    if (! layout.depthCurve.empty()) paintDepth (g, asRect (layout.depthCurve), p, proc.visD.load());
    if (! layout.orbitCurve.empty()) paintOrbitCurve (g, asRect (layout.orbitCurve), p, proc.visTheta.load());

    if (proc.getUiFlags().meter && ! proc.getUiFlags().zones)
    {
        auto traceArea = findBox ("trace title");
        traceArea = { layout.drawer.x + 8, traceArea.getBottom() + 4, 440.0f * getWidth() / 1280.0f, layout.drawer.h - 36 };
        g.setColour (juce::Colour (0xff2a3144));
        g.drawLine (traceArea.getX(), traceArea.getCentreY(), traceArea.getRight(), traceArea.getCentreY(), 1.0f);
        const int shown = juce::jmin (480, traceCount);
        for (int i = 1; i < shown; ++i)
        {
            const int a = (traceCount - shown + i - 1 + 4800) % 480;
            const int b = (traceCount - shown + i + 4800) % 480;
            const float x0 = traceArea.getX() + traceArea.getWidth() * (float) (i - 1) / 479.0f;
            const float x1 = traceArea.getX() + traceArea.getWidth() * (float) i / 479.0f;
            const float y0 = traceArea.getCentreY() - traceArea.getHeight() * 0.45f * juce::jlimit (-1.0f, 1.0f, trace[(size_t) a] / 60.0f);
            const float y1 = traceArea.getCentreY() - traceArea.getHeight() * 0.45f * juce::jlimit (-1.0f, 1.0f, trace[(size_t) b] / 60.0f);
            g.setColour (juce::Colour (orbit::ui::zoneColour (traceZone[(size_t) b])));
            g.drawLine (x0, y0, x1, y1, 1.4f);
        }
        auto bar = [&] (const char* valueName, float cents)
        {
            auto value = findBox (valueName);
            auto slot = value.translated (-70.0f * getWidth() / 1280.0f, 2.0f).withWidth (60.0f * getWidth() / 1280.0f).withHeight (8.0f);
            g.setColour (juce::Colour (0xff2a3144));
            g.fillRoundedRectangle (slot, 2.0f);
            g.setColour (juce::Colour (0xff8ec8ff));
            g.fillRoundedRectangle (slot.withWidth (slot.getWidth() * juce::jlimit (0.0f, 1.0f, std::abs (cents) / 10.0f)), 2.0f);
        };
        bar ("wow value", proc.visCWow.load());
        bar ("flutter value", proc.visCFlutter.load());
        bar ("chorus value", proc.visCChorus.load());
    }

    if (proc.getUiFlags().zones)
    {
        const float distances[] = { 0.80f, 0.30f, 0.10f };
        const auto preset = orbit::factoryPresets()[0].p;
        for (int i = 0; i < 3; ++i)
        {
            auto tile = asRect (layout.zoneTile[i]).reduced (8.0f, 70.0f);
            tile = tile.withTrimmedBottom (80.0f);
            const auto mapped = orbit::mapModulation (distances[i], preset.gravity, preset.horizon, preset.warp, preset.flutter, preset.floor, preset.chaos);
            juce::Path wave;
            for (int s = 0; s <= 80; ++s)
            {
                const float t = 4.0f * (float) s / 80.0f;
                const float y = mapped.cWow * std::sin (orbit::kTwoPi * mapped.fWow * t)
                                + mapped.cFlutter * std::sin (orbit::kTwoPi * mapped.fFlutter * t)
                                + mapped.cChorus * std::sin (orbit::kTwoPi * 0.55f * t);
                const float px = tile.getX() + tile.getWidth() * (float) s / 80.0f;
                const float py = tile.getCentreY() - tile.getHeight() * 0.45f * juce::jlimit (-1.0f, 1.0f, y / 25.0f);
                if (s == 0) wave.startNewSubPath (px, py); else wave.lineTo (px, py);
            }
            g.setColour (juce::Colour (orbit::ui::zoneColour (i == 0 ? 2 : (i == 1 ? 1 : 0))));
            g.strokePath (wave, juce::PathStrokeType (1.3f));
        }
    }
}
