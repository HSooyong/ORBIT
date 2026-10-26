// ORBIT · Gravity Warp — 7 factory presets (design doc §8). Shared by plugin + tests.
#pragma once
#include "WarpEngine.h"

namespace orbit
{
struct Preset { const char* name; Params p; };

inline Params defaults() { return Params {}; }

inline std::array<Preset, 7> factoryPresets()
{
    std::array<Preset, 7> ps;
    // sync indices: 0 Free,1 4/1,2 2/1,3 1/1,4 1/2D,5 1/2,6 1/4D,7 1/4,8 1/8,9 1/8T,10 1/16
    Params a = defaults(); a.bhX = 0.9f; a.bhY = 0.0f; a.floor = 0.15f; a.mix = 0.35f; a.voices = 2;
    ps[0] = { "Low Orbit", a };
    Params b = defaults(); b.voices = 3; b.width = 0.85f; b.warp = 0.25f; b.flutter = 0.0f; b.mix = 0.30f;
    ps[1] = { "Vocal Halo", b };
    Params c = defaults(); c.bhX = 0.35f; c.bhY = 0.1f; c.chaos = 0.5f; c.flutter = 0.45f; c.tone = -0.3f; c.drive = 0.3f; c.mix = 0.5f;
    ps[2] = { "Worn Cassette", c };
    Params d = defaults(); d.ecc = 0.8f; d.sync = 3; d.kepler = true; d.bhX = 0.0f; d.bhY = 0.25f; d.gravity = 0.8f; d.mix = 0.45f;
    ps[3] = { "Periapsis", d };
    Params e = defaults(); e.bhX = 0.7f; e.bhY = 0.0f; e.gravity = 1.0f; e.horizon = 0.35f; e.feedback = 0.35f; e.bloom = 0.6f; e.mix = 0.6f;
    ps[4] = { "Event Horizon", e };
    Params f = defaults(); f.stages = 6; f.sync = 8; f.shape = 0.6f; f.bhX = 0.5f; f.gravity = 0.7f; f.mix = 0.5f;
    ps[5] = { "Stepping Stones", f };
    Params g = defaults(); g.rate = 0.05f; g.warp = 0.9f; g.voices = 4; g.width = 0.75f; g.mix = 0.6f; g.bhX = 0.6f; g.gravity = 0.6f; g.bloom = 0.20f;
    ps[6] = { "Tidal Pad", g };
    return ps;
}
} // namespace orbit
