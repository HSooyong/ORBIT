#include "OrbitScene.h"
#include "EditorLayout.h"

namespace
{
constexpr float kTau = juce::MathConstants<float>::twoPi;

juce::Image makeHole (int size)
{
    juce::Image image (juce::Image::ARGB, size, size, true);
    const float cx = size * 0.5f, cy = size * 0.5f, norm = size * 0.5f;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const float nx = ((float) x - cx) / norm, ny = ((float) y - cy) / norm;
            const float radius = std::hypot (nx, ny);
            const float disk = (nx * nx) / (0.78f * 0.78f) + (ny * ny) / (0.11f * 0.11f);
            float alpha = 0.0f, hot = 0.0f;
            if (disk < 1.0f && radius > 0.16f)
            {
                const float edge = std::pow (1.0f - disk, 1.4f);
                hot = 1.0f - juce::jlimit (0.0f, 1.0f, (radius - 0.16f) / 0.42f);
                alpha = edge * (0.25f + 0.75f * hot) * (ny > -0.015f ? 1.0f : 0.22f);
            }
            const float ring = std::exp (-std::pow ((radius - 0.175f) / 0.012f, 2.0f));
            const float arc = std::exp (-std::pow ((std::hypot (nx * 0.9f, ny + 0.01f) - 0.23f) / 0.02f, 2.0f));
            if (ny < 0.08f) alpha = std::max (alpha, arc * 0.75f);
            alpha = std::max (alpha, ring);
            hot = std::max (hot, ring);
            if (radius < 0.155f)
            {
                image.setPixelAt (x, y, juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, (0.155f - radius) / 0.02f)));
                continue;
            }
            if (alpha < 0.015f) continue;
            const float r = juce::jlimit (0.0f, 1.0f, 0.55f + 0.45f * hot);
            const float g = juce::jlimit (0.0f, 1.0f, 0.22f + 0.65f * hot * hot);
            const float b = juce::jlimit (0.0f, 1.0f, 0.08f + 0.45f * ring);
            image.setPixelAt (x, y, juce::Colour::fromFloatRGBA (r, g, b, std::pow (alpha, 0.9f)));
        }
    return image;
}
} // namespace

OrbitScene::OrbitScene (OrbitAudioProcessor& p) : proc (p)
{
    juce::Random rng (20261116);
    stars.resize (520);
    for (auto& s : stars)
    {
        const float depth = std::pow (rng.nextFloat(), 2.2f);
        s.x = rng.nextFloat() * 3.2f - 1.6f;
        s.y = rng.nextFloat() * 2.4f - 1.2f;
        s.bright = 0.15f + 0.85f * depth;
        s.size = depth > 0.92f ? 2.2f : (depth > 0.6f ? 1.4f : 0.9f);
    }
    setOpaque (true);
}

void OrbitScene::resized() { holeKey = -1.0f; }

void OrbitScene::rebuildHole()
{
    const float key = shadowRadius();
    if (hole.isValid() && std::abs (key - holeKey) < 1.5f) return;
    holeKey = key;
    const int size = juce::jlimit (180, 520, (int) (key * 8.0f));
    hole = makeHole (size);
}

juce::Point<float> OrbitScene::toScreen (float x, float y) const
{
    const float scale = 0.42f * (float) juce::jmin (getWidth(), getHeight());
    return { getWidth() * 0.46f + x * scale, getHeight() * 0.52f - y * scale };
}

juce::Point<float> OrbitScene::toField (juce::Point<float> p) const
{
    const float scale = 0.42f * (float) juce::jmin (getWidth(), getHeight());
    return { (p.x - getWidth() * 0.46f) / scale, (getHeight() * 0.52f - p.y) / scale };
}

juce::Point<float> OrbitScene::lens (juce::Point<float> p, juce::Point<float> bh, float radius) const
{
    const auto delta = p - bh;
    const float r2 = delta.x * delta.x + delta.y * delta.y + 1.0f;
    return p + delta * (radius * radius / r2);
}

float OrbitScene::plain (const char* id) const
{
    if (auto* parameter = proc.apvts.getParameter (id))
        return parameter->convertFrom0to1 (parameter->getValue());
    return 0.0f;
}

void OrbitScene::setPlain (const char* id, float value)
{
    if (auto* parameter = proc.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

float OrbitScene::shadowRadius() const
{
    const float unit = 0.46f * (float) juce::jmin (getWidth(), getHeight());
    const float horizon = plain ("horizon");
    const float gravity = plain ("gravity") * 0.01f;
    return unit * (0.20f + 0.16f * horizon) * (0.82f + 0.18f * gravity);
}

void OrbitScene::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff07090d));
    rebuildHole();
    const float bhX = proc.visBhX.load(), bhY = proc.visBhY.load();
    const float px = proc.visPx.load(), py = proc.visPy.load();
    const auto bh = toScreen (bhX, bhY);
    const float shadow = shadowRadius();
    const float radius = plain ("radius"), ecc = plain ("ecc");

    for (int i = 0; i < 18; ++i)
    {
        const float cx = getWidth() * (0.15f + 0.07f * (float) (i % 5));
        const float cy = getHeight() * (0.2f + 0.08f * (float) (i % 4));
        const float rad = 40.0f + 18.0f * (float) (i % 6);
        juce::Colour dust = (i % 2) ? juce::Colour (0xff1a2430) : juce::Colour (0xff2a2118);
        g.setGradientFill (juce::ColourGradient (dust.withAlpha (0.35f), cx, cy, dust.withAlpha (0.0f), cx + rad, cy, true));
        g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
    }

    for (const auto& s : stars)
    {
        auto p = lens (toScreen (s.x, s.y), bh, shadow * 1.35f);
        if (p.getDistanceFrom (bh) < shadow * 0.98f) continue;
        if (! getLocalBounds().toFloat().contains (p)) continue;
        g.setColour (juce::Colour (0xfff4f7fb).withAlpha (s.bright * 0.9f));
        g.fillEllipse (p.x - s.size * 0.5f, p.y - s.size * 0.5f, s.size, s.size);
    }

    for (float edge : { orbit::ui::kNearEdge, orbit::ui::kFarEdge })
    {
        juce::Path dashes;
        const float fieldR = edge * 2.0f;
        for (int i = 0; i < 64; i += 2)
        {
            const float a0 = kTau * (float) i / 64.0f, a1 = kTau * (float) (i + 1) / 64.0f;
            const auto p0 = toScreen (bhX + fieldR * std::cos (a0), bhY + fieldR * std::sin (a0));
            const auto p1 = toScreen (bhX + fieldR * std::cos (a1), bhY + fieldR * std::sin (a1));
            dashes.startNewSubPath (p0);
            dashes.lineTo (p1);
        }
        g.setColour (juce::Colour (orbit::ui::zoneColour (edge < 0.3f ? 0 : 1)).withAlpha (0.35f));
        g.strokePath (dashes, juce::PathStrokeType (1.0f));
    }

    for (int i = 0; i < 160; ++i)
    {
        const float t0 = kTau * (float) i / 160.0f;
        const float t1 = kTau * (float) (i + 1) / 160.0f;
        const float x0 = radius * std::cos (t0), y0 = radius * (1.0f - ecc) * std::sin (t0);
        const float x1 = radius * std::cos (t1), y1 = radius * (1.0f - ecc) * std::sin (t1);
        const float d = std::hypot (x0 - bhX, y0 - bhY) * 0.5f;
        const auto p0 = toScreen (x0, y0), p1 = toScreen (x1, y1);
        g.setColour (juce::Colour (orbit::ui::zoneColour (orbit::ui::zoneIndex (d))));
        g.drawLine (p0.x, p0.y, p1.x, p1.y, 2.0f);
    }

    if (hole.isValid())
        g.drawImage (hole, juce::Rectangle<float> (bh.x - shadow * 4.2f, bh.y - shadow * 4.2f, shadow * 8.4f, shadow * 8.4f),
                     juce::RectanglePlacement::stretchToFit, false);
    g.setColour (juce::Colour (0xffffb45c).withAlpha (0.45f));
    g.fillEllipse (bh.x - shadow * 2.35f, bh.y - shadow * 0.22f, shadow * 4.7f, shadow * 0.44f);
    g.setColour (juce::Colour (0xffffe2b0).withAlpha (0.9f));
    g.drawEllipse (bh.x - shadow * 2.5f, bh.y - shadow * 0.42f, shadow * 5.0f, shadow * 0.84f, 2.6f);
    g.setColour (juce::Colour (0xfffff6ea).withAlpha (0.95f));
    g.drawEllipse (bh.x - shadow * 1.18f, bh.y - shadow * 1.18f, shadow * 2.36f, shadow * 2.36f, 1.8f);
    {
        juce::Path arc;
        arc.addCentredArc (bh.x, bh.y - shadow * 0.05f, shadow * 1.55f, shadow * 1.35f, 0.0f, -2.4f, 0.7f, true);
        g.setColour (juce::Colour (0xfffff1d2).withAlpha (0.7f));
        g.strokePath (arc, juce::PathStrokeType (2.2f));
    }
    g.setColour (juce::Colours::black);
    g.fillEllipse (bh.x - shadow, bh.y - shadow, shadow * 2.0f, shadow * 2.0f);

    const float theta = proc.visTheta.load();
    const auto planet = toScreen (px, py);
    for (int i = 16; i >= 1; --i)
    {
        const float th = theta * kTau - (float) i * 0.055f;
        const auto p = toScreen (radius * std::cos (th), radius * (1.0f - ecc) * std::sin (th));
        const float a = 0.55f * (1.0f - (float) i / 16.0f);
        const float r = 1.2f + 3.5f * a;
        g.setColour (juce::Colour (0xffd7e7f5).withAlpha (a));
        g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
    }
    const float pr = juce::jmax (5.0f, shadow * 0.22f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7fbff), planet.x - pr * 0.3f, planet.y - pr * 0.4f,
                                              juce::Colour (0xff6d7c8c), planet.x + pr, planet.y + pr, true));
    g.fillEllipse (planet.x - pr, planet.y - pr, pr * 2.0f, pr * 2.0f);

    const int voices = (int) plain ("voices") + 1;
    const float width = plain ("width") * 0.01f;
    for (int i = 1; i < voices; ++i)
    {
        const float a = kTau * (theta * 2.0f + (float) i / (float) voices);
        const auto s = planet + juce::Point<float> (std::cos (a), std::sin (a) * 0.55f) * (pr * (2.2f + 1.4f * width));
        g.setColour (juce::Colour (0xffd5dde8));
        g.fillEllipse (s.x - 2.0f, s.y - 2.0f, 4.0f, 4.0f);
    }
}

void OrbitScene::mouseMove (const juce::MouseEvent& e)
{
    overPlanet = e.position.getDistanceFrom (toScreen (proc.visPx.load(), proc.visPy.load())) < 16.0f;
    overHole = ! overPlanet && e.position.getDistanceFrom (toScreen (proc.visBhX.load(), proc.visBhY.load())) < shadowRadius() * 1.3f;
    setMouseCursor (overPlanet || overHole ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void OrbitScene::mouseDown (const juce::MouseEvent& e)
{
    mouseMove (e);
    dragField = toField (e.position);
    dragDistance = proc.visD.load();
    dragCents = proc.visCents.load();
    if (overPlanet)
    {
        drag = Drag::planet;
        holdWas = plain ("hold") > 0.5f;
        for (const char* id : { "phase", "radius" }) proc.apvts.getParameter (id)->beginChangeGesture();
        if (! holdWas)
        {
            auto* hold = proc.apvts.getParameter ("hold");
            hold->beginChangeGesture();
            hold->setValueNotifyingHost (1.0f);
            hold->endChangeGesture();
        }
    }
    else if (overHole)
    {
        drag = Drag::hole;
        for (const char* id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->beginChangeGesture();
    }
}

void OrbitScene::mouseDrag (const juce::MouseEvent& e)
{
    auto field = toField (e.position);
    if (e.mods.isShiftDown()) field = dragField + (field - dragField) * 0.25f;
    dragField = field;
    if (drag == Drag::planet)
    {
        const float ecc = plain ("ecc"), b = juce::jmax (0.1f, 1.0f - ecc);
        const float radius = juce::jlimit (0.2f, 1.0f, std::hypot (field.x, field.y / b));
        float phase = std::atan2 (field.y / b, field.x) / kTau - proc.visTheta.load();
        phase -= std::floor (phase);
        setPlain ("radius", radius);
        setPlain ("phase", phase);
    }
    else if (drag == Drag::hole)
    {
        setPlain ("bh_x", juce::jlimit (-1.0f, 1.0f, field.x));
        setPlain ("bh_y", juce::jlimit (-1.0f, 1.0f, field.y));
    }
}

void OrbitScene::mouseUp (const juce::MouseEvent&)
{
    if (drag == Drag::planet)
    {
        for (const char* id : { "phase", "radius" }) proc.apvts.getParameter (id)->endChangeGesture();
        if (! holdWas)
        {
            auto* hold = proc.apvts.getParameter ("hold");
            hold->beginChangeGesture();
            hold->setValueNotifyingHost (0.0f);
            hold->endChangeGesture();
        }
        if (onToast)
        {
            const float d1 = proc.visD.load(), c1 = proc.visCents.load();
            const bool closer = d1 < dragDistance;
            onToast (juce::String (closer ? "행성을 블랙홀 쪽으로 옮겼습니다 · 거리 " : "행성을 블랙홀에서 멀리 옮겼습니다 · 거리 ")
                     + juce::String (dragDistance, 2) + " → " + juce::String (d1, 2)
                     + " · 흔들림 ±" + juce::String (dragCents, 1) + " → ±" + juce::String (c1, 1) + " 센트");
        }
    }
    else if (drag == Drag::hole)
        for (const char* id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->endChangeGesture();
    drag = Drag::none;
}

void OrbitScene::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto bh = toScreen (proc.visBhX.load(), proc.visBhY.load());
    const float shadow = shadowRadius();
    const float dist = e.position.getDistanceFrom (bh);
    const auto preset = orbit::factoryPresets()[(size_t) proc.getCurrentProgram()].p;
    if (dist > shadow * 0.8f && dist < shadow * 2.6f)
    {
        for (const char* id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->beginChangeGesture();
        setPlain ("bh_x", preset.bhX);
        setPlain ("bh_y", preset.bhY);
        for (const char* id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->endChangeGesture();
        if (onToast) onToast ("블랙홀을 장면 위치로 되돌렸습니다");
        return;
    }
    if (overPlanet || overHole) return;
    const auto field = toField (e.position);
    for (const char* id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->beginChangeGesture();
    setPlain ("bh_x", juce::jlimit (-1.0f, 1.0f, field.x));
    setPlain ("bh_y", juce::jlimit (-1.0f, 1.0f, field.y));
    for (const char* id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->endChangeGesture();
}

void OrbitScene::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.position.getDistanceFrom (toScreen (proc.visBhX.load(), proc.visBhY.load())) > shadowRadius() * 2.2f) return;
    const float before = plain ("gravity");
    auto* parameter = proc.apvts.getParameter ("gravity");
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, parameter->getValue() + wheel.deltaY * 0.12f));
    parameter->endChangeGesture();
    const float after = plain ("gravity");
    if (onToast && std::abs (after - before) > 0.4f)
        onToast ("중력 " + juce::String (juce::roundToInt (before)) + " → " + juce::String (juce::roundToInt (after))
                 + " % · 흔들림 ±" + juce::String (proc.visCents.load(), 1) + " 센트");
}
