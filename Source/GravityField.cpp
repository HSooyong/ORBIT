#include "GravityField.h"
#include "OrbitLookAndFeel.h"

namespace { constexpr float kTau = juce::MathConstants<float>::twoPi; }

GravityField::GravityField (OrbitAudioProcessor& p) : proc (p)
{
    juce::Random r (7);
    for (int i = 0; i < 420; ++i)
    {
        const float b = std::pow (r.nextFloat(), 3.0f);
        stars.push_back ({ r.nextFloat() * 2.4f - 1.2f, r.nextFloat() * 2.4f - 1.2f, 0.15f + 0.85f * b, b > 0.85f ? 1.6f : (b > 0.5f ? 1.1f : 0.8f) });
    }
    setOpaque (true);
    startTimerHz (30);
}
GravityField::~GravityField() { stopTimer(); }

void GravityField::resized()
{
    // soft, desaturated nebula (deep teal / warm dust) rendered once
    nebula = juce::Image (juce::Image::ARGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), true);
    juce::Graphics g (nebula);
    g.fillAll (juce::Colour (0xff05070a));
    juce::Random r (11);
    for (int i = 0; i < 26; ++i)
    {
        const float cx = r.nextFloat() * getWidth(), cy = r.nextFloat() * getHeight(), rad = 60.0f + r.nextFloat() * 220.0f;
        const auto c = (i % 3 == 0) ? juce::Colour (0xff3a2a1e) : juce::Colour (0xff142430);
        juce::ColourGradient cg (c.withAlpha (0.10f + 0.10f * r.nextFloat()), cx, cy, c.withAlpha (0.0f), cx + rad, cy, true);
        g.setGradientFill (cg); g.fillEllipse (cx - rad, cy - rad, rad * 2, rad * 2);
    }
}

juce::Point<float> GravityField::toScreen (float x, float y) const
{
    const float s = 0.46f * (float) juce::jmin (getWidth(), getHeight());
    return { getWidth() * 0.5f + x * s, getHeight() * 0.5f - y * s };
}
juce::Point<float> GravityField::toField (juce::Point<float> p) const
{
    const float s = 0.46f * (float) juce::jmin (getWidth(), getHeight());
    return { (p.x - getWidth() * 0.5f) / s, (getHeight() * 0.5f - p.y) / s };
}
juce::Point<float> GravityField::lens (juce::Point<float> p, juce::Point<float> bh, float rE) const
{
    auto v = p - bh; const float r2 = v.x * v.x + v.y * v.y + 1e-3f;
    return p + v * (rE * rE / r2); // point-mass lens: images pushed outward, ring at ~rE
}

float GravityField::getParam (const char* id) const
{
    auto* prm = proc.apvts.getParameter (id);
    return prm ? prm->convertFrom0to1 (prm->getValue()) : 0.0f;
}
void GravityField::setParam (const char* id, float v)
{
    if (auto* prm = proc.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

void GravityField::timerCallback()
{
    px = proc.visPx.load(); py = proc.visPy.load(); bhX = proc.visBhX.load(); bhY = proc.visBhY.load();
    d = proc.visD.load(); g = proc.visG.load(); cents = proc.visCents.load(); bloom = proc.visBloom.load();
    theta = proc.visTheta.load(); wowHz = proc.visWowHz.load();
    phaseClock += 1.0f / 30.0f;

    // planet trail; pulled toward the hole proportionally to g (suck-in)
    for (int i = (int) trail.size() - 1; i > 0; --i) trail[(size_t) i] = trail[(size_t) i - 1];
    trail[0] = { px, py }; trailN = juce::jmin (trailN + 1, (int) trail.size());
    for (int i = 1; i < trailN; ++i)
    {
        auto& t = trail[(size_t) i]; const juce::Point<float> to { bhX - t.x, bhY - t.y };
        t += to * (0.010f + 0.05f * g * g);
    }
    // dust stripped from the planet, spiralling into the horizon; amount ~ g, bloom adds glow
    const int spawn = (int) std::round (g * 6.0f + bloom * 4.0f);
    for (int i = 0; i < spawn && dust.size() < 260; ++i)
    {
        const float dx = px - bhX, dy = py - bhY;
        dust.push_back ({ std::atan2 (dy, dx) + (rng.nextFloat() - 0.5f) * 0.3f, std::hypot (dx, dy) * (0.9f + 0.2f * rng.nextFloat()), 1.0f, 0.5f + rng.nextFloat() });
    }
    const float unitPx = 0.46f * (float) juce::jmin (getWidth(), getHeight());
    const float shadowPx = unitPx * (0.05f + 0.35f * getParam ("horizon")) * (0.55f + 0.45f * getParam ("gravity") * 0.01f);
    for (auto& p : dust)
    {
        p.ang += (0.05f + 0.25f * p.speed) / juce::jmax (0.08f, p.rad);
        p.rad *= 1.0f - (0.02f + 0.05f * p.speed) * (0.4f + g);
        p.life -= 0.012f;
        if (p.rad * unitPx < shadowPx * 1.05f) p.life = 0;
    }
    dust.erase (std::remove_if (dust.begin(), dust.end(), [] (const Dust& q) { return q.life <= 0; }), dust.end());
    if (isShowing()) repaint();
}

void GravityField::paint (juce::Graphics& gr)
{
    const auto b = getLocalBounds().toFloat();
    if (nebula.isValid()) gr.drawImageAt (nebula, 0, 0); else gr.fillAll (juce::Colour (0xff05070a));

    const auto bhS = toScreen (bhX, bhY);
    const float unit = 0.46f * juce::jmin (b.getWidth(), b.getHeight());
    const float grav = getParam ("gravity") * 0.01f;
    const float hor = getParam ("horizon");
    const float rH = unit * (0.05f + 0.35f * hor) * (0.55f + 0.45f * grav); // shadow radius in px
    const float rE = rH * 1.35f;

    // background stars, gravitationally lensed
    for (auto& s : stars)
    {
        auto p = lens (toScreen (s.x, s.y), bhS, rE);
        if (p.getDistanceFrom (bhS) < rH * 1.02f) continue;
        const float tw = 0.85f + 0.15f * std::sin (phaseClock * (1.0f + s.b * 3.0f) + s.x * 40.0f);
        gr.setColour (juce::Colour (0xffdfe6ee).withAlpha (s.b * tw * 0.9f));
        gr.fillEllipse (p.x - s.sz * 0.5f, p.y - s.sz * 0.5f, s.sz, s.sz);
    }

    // orbit path (warped toward the hole by local gravity)
    {
        const float R = getParam ("radius"), e = getParam ("ecc");
        juce::Path orbit;
        for (int i = 0; i <= 160; ++i)
        {
            const float th = kTau * i / 160.0f;
            juce::Point<float> f { R * std::cos (th), R * (1.0f - e) * std::sin (th) };
            const float dd = f.getDistanceFrom ({ bhX, bhY }) * 0.5f;
            const float gl = grav / (1.0f + std::pow (dd / juce::jmax (0.05f, hor), 2.5f));
            f += (juce::Point<float> { bhX, bhY } - f) * (0.12f * gl);
            const auto sp = toScreen (f.x, f.y);
            if (i == 0) orbit.startNewSubPath (sp); else orbit.lineTo (sp);
        }
        gr.setColour (juce::Colour (0xff8a95a3).withAlpha (0.28f));
        const float dashes[] = { 3.0f, 5.0f };
        juce::Path dashed; juce::PathStrokeType (1.0f).createDashedStroke (dashed, orbit, dashes, 2);
        gr.fillPath (dashed);
        const int st = (int) getParam ("stages");
        static const int tbl[] = { 1, 2, 3, 4, 5, 6, 8, 12, 16 };
        if (st > 0)
            for (int k = 0; k < tbl[st]; ++k)
            {
                const float th = kTau * (k / (float) tbl[st] + getParam ("phase"));
                const auto sp = toScreen (R * std::cos (th), R * (1.0f - e) * std::sin (th));
                gr.setColour (juce::Colour (0xff8a95a3).withAlpha (0.55f)); gr.fillEllipse (sp.x - 2, sp.y - 2, 4, 4);
            }
    }

    // accretion disc: thin, warm, tilted (back half), drawn before shadow
    auto disc = [&] (bool front)
    {
        const float rx = rH * 2.6f, ry = rH * 0.55f, rot = -0.22f;
        const auto tr = juce::AffineTransform::rotation (rot).translated (bhS);
        for (int ring = 0; ring < 7; ++ring)
        {
            const float k = 1.0f + ring * 0.11f;
            juce::Path p; p.addCentredArc (0, 0, rx * k, ry * k, 0.0f, front ? juce::MathConstants<float>::halfPi : -juce::MathConstants<float>::halfPi,
                                          front ? juce::MathConstants<float>::halfPi * 3 : juce::MathConstants<float>::halfPi, true);
            const float a = (0.30f - ring * 0.035f) * (0.5f + 0.5f * grav) * (1.0f + 0.8f * bloom);
            gr.setColour (juce::Colour (0xffe0b47a).interpolatedWith (juce::Colour (0xff9a6a44), ring / 7.0f).withAlpha (juce::jlimit (0.0f, 1.0f, a)));
            gr.strokePath (p, juce::PathStrokeType (1.6f), tr);
        }
    };
    disc (false);
    // lensed top arc of the disc (Gargantua look): the far side bent over the shadow
    {
        juce::Path arc; arc.addCentredArc (bhS.x, bhS.y, rH * 1.25f, rH * 1.18f, 0.0f, -1.35f, 1.35f, true);
        gr.setColour (juce::Colour (0xffd8b07c).withAlpha (0.22f + 0.25f * grav + 0.2f * bloom));
        gr.strokePath (arc, juce::PathStrokeType (2.2f));
    }
    // shadow + photon ring
    {
        juce::ColourGradient halo (juce::Colours::black, bhS.x, bhS.y, juce::Colours::black.withAlpha (0.0f), bhS.x + rH * 1.9f, bhS.y, true);
        gr.setGradientFill (halo); gr.fillEllipse (bhS.x - rH * 1.9f, bhS.y - rH * 1.9f, rH * 3.8f, rH * 3.8f);
        gr.setColour (juce::Colours::black); gr.fillEllipse (bhS.x - rH, bhS.y - rH, rH * 2, rH * 2);
        gr.setColour (juce::Colour (0xffeed7b0).withAlpha (0.35f + 0.3f * g + 0.25f * bloom));
        gr.drawEllipse (bhS.x - rH * 1.04f, bhS.y - rH * 1.04f, rH * 2.08f, rH * 2.08f, 1.0f);
        if (hoverHole || drag == Drag::hole)
        { gr.setColour (OrbitLookAndFeel::muted().withAlpha (0.5f)); gr.drawEllipse (bhS.x - rH * 2.2f, bhS.y - rH * 2.2f, rH * 4.4f, rH * 4.4f, 0.8f); }
    }
    disc (true);

    // dust being sucked in
    for (auto& p : dust)
    {
        const auto sp = toScreen (bhX + p.rad * std::cos (p.ang), bhY + p.rad * std::sin (p.ang));
        gr.setColour (juce::Colour (0xffe6c9a0).withAlpha (0.55f * p.life));
        gr.fillEllipse (sp.x - 0.9f, sp.y - 0.9f, 1.8f, 1.8f);
    }

    // planet trail (stretched toward the hole when close)
    for (int i = trailN - 1; i > 0; --i)
    {
        const auto a = toScreen (trail[(size_t) i].x, trail[(size_t) i].y), c = toScreen (trail[(size_t) i - 1].x, trail[(size_t) i - 1].y);
        gr.setColour (juce::Colour (0xffa9bccb).withAlpha (0.35f * (1.0f - i / (float) trailN)));
        gr.drawLine ({ a, c }, 1.2f + 1.5f * g * (1.0f - i / (float) trailN));
    }
    // planet: tidally stretched sphere, lit from the disc side
    {
        const auto ps = toScreen (px, py);
        const float r = 9.0f;
        const float stretch = 1.0f + 0.9f * g;
        const float ang = std::atan2 (bhS.y - ps.y, bhS.x - ps.x);
        const auto tr = juce::AffineTransform::scale (stretch, 1.0f / std::sqrt (stretch)).rotated (ang).translated (ps);
        juce::Path sphere; sphere.addEllipse (-r, -r, 2 * r, 2 * r);
        const auto lit = juce::Point<float> (std::cos (ang), std::sin (ang)) * (r * 0.5f);
        juce::ColourGradient sh (juce::Colour (0xffcfd8e0), ps.x + lit.x, ps.y + lit.y, juce::Colour (0xff1b2530), ps.x - lit.x * 1.6f, ps.y - lit.y * 1.6f, true);
        gr.setGradientFill (sh); gr.fillPath (sphere, tr);
        gr.setColour (juce::Colours::black.withAlpha (0.5f)); gr.strokePath (sphere, juce::PathStrokeType (1.0f), tr);
        if (hoverPlanet || drag == Drag::planet)
        { gr.setColour (OrbitLookAndFeel::muted().withAlpha (0.6f)); gr.drawEllipse (ps.x - r * 2, ps.y - r * 2, r * 4, r * 4, 0.8f); }
        // satellites = voices
        const int nv = (int) getParam ("voices") + 1;
        const float w = getParam ("width") * 0.01f;
        for (int i = 0; i < nv && nv > 1; ++i)
        {
            const float a2 = kTau * (theta * 3.0f + i / (float) nv);
            const auto sp = ps + juce::Point<float> (std::cos (a2), std::sin (a2) * 0.45f) * (r * (1.8f + 1.6f * w));
            gr.setColour (juce::Colour (0xffb8c4ce).withAlpha (0.8f)); gr.fillEllipse (sp.x - 1.6f, sp.y - 1.6f, 3.2f, 3.2f);
        }
    }

    // vignette + readout
    juce::ColourGradient vig (juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY(), juce::Colours::black.withAlpha (0.55f), b.getX(), b.getY(), true);
    gr.setGradientFill (vig); gr.fillRect (b);
    gr.setColour (OrbitLookAndFeel::muted());
    gr.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain)));
    gr.drawText (juce::String::formatted ("DIST %.2f   PULL %3d%%   WARP ", d, (int) std::round (g * 100)) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb1"))
                 + juce::String::formatted ("%.1f ct   WOW %.2f Hz%s", cents, wowHz, bloom > 0.01f ? "   BLOOM" : ""),
                 b.reduced (12, 8).removeFromBottom (14), juce::Justification::bottomLeft);
    gr.drawText (getParam ("hold") > 0.5f ? "HOLD" : "", b.reduced (12, 8).removeFromTop (14), juce::Justification::topRight);
    gr.setColour (OrbitLookAndFeel::metal()); gr.drawRect (b, 1.0f);
}

void GravityField::mouseMove (const juce::MouseEvent& e)
{
    const auto f = e.position;
    hoverPlanet = f.getDistanceFrom (toScreen (px, py)) < 16.0f;
    hoverHole = ! hoverPlanet && f.getDistanceFrom (toScreen (bhX, bhY)) < 26.0f;
    setMouseCursor (hoverPlanet || hoverHole ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void GravityField::mouseDown (const juce::MouseEvent& e)
{
    mouseMove (e);
    if (hoverPlanet)
    {
        drag = Drag::planet;
        holdWasOn = getParam ("hold") > 0.5f;
        for (auto id : { "phase", "radius" }) proc.apvts.getParameter (id)->beginChangeGesture();
        if (! holdWasOn) { auto* h = proc.apvts.getParameter ("hold"); h->beginChangeGesture(); h->setValueNotifyingHost (1.0f); h->endChangeGesture(); }
    }
    else if (hoverHole)
    {
        drag = Drag::hole;
        for (auto id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->beginChangeGesture();
    }
}

void GravityField::mouseDrag (const juce::MouseEvent& e)
{
    const auto f = toField (e.position);
    if (drag == Drag::planet)
    {
        const float ecc = getParam ("ecc"), b = 1.0f - ecc;
        const float R = juce::jlimit (0.2f, 1.0f, std::hypot (f.x, f.y / b));
        float ph = std::atan2 (f.y / b, f.x) / kTau - theta; // engine angle = theta_stage + phase
        ph -= std::floor (ph);
        setParam ("radius", R); setParam ("phase", ph);
    }
    else if (drag == Drag::hole)
    {
        setParam ("bh_x", juce::jlimit (-1.0f, 1.0f, f.x)); setParam ("bh_y", juce::jlimit (-1.0f, 1.0f, f.y));
    }
}

void GravityField::mouseUp (const juce::MouseEvent&)
{
    if (drag == Drag::planet)
    {
        for (auto id : { "phase", "radius" }) proc.apvts.getParameter (id)->endChangeGesture();
        if (! holdWasOn) { auto* h = proc.apvts.getParameter ("hold"); h->beginChangeGesture(); h->setValueNotifyingHost (0.0f); h->endChangeGesture(); }
    }
    else if (drag == Drag::hole)
        for (auto id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->endChangeGesture();
    drag = Drag::none;
}

void GravityField::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (hoverPlanet || hoverHole) return;
    const auto f = toField (e.position); // double-click empty space: move the hole there
    for (auto id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->beginChangeGesture();
    setParam ("bh_x", juce::jlimit (-1.0f, 1.0f, f.x)); setParam ("bh_y", juce::jlimit (-1.0f, 1.0f, f.y));
    for (auto id : { "bh_x", "bh_y" }) proc.apvts.getParameter (id)->endChangeGesture();
}

void GravityField::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.position.getDistanceFrom (toScreen (bhX, bhY)) > 60.0f) return;
    auto* prm = proc.apvts.getParameter ("gravity");
    prm->beginChangeGesture();
    prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + w.deltaY * 0.15f));
    prm->endChangeGesture();
}
