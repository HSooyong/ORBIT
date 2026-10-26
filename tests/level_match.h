// Level match (shared by orbit_loudness_test and orbit_render_test): output vs dry (RMS + BS.1770 integrated LUFS) for default + 7 presets x 3 program materials.
#include "engine/Presets.h"
#include "loudness.h"
#pragma once
#include <algorithm>
#include <cstdio>
#include <string>
namespace levelmatch {
using namespace orbit;
static std::vector<float> pink (int n, uint32_t seed)
{   XorShift32 r (seed); std::vector<float> y ((size_t) n); double b[7] {};
    for (int i = 0; i < n; ++i) { double w = r.bipolar(); b[0] = 0.99886 * b[0] + w * 0.0555179; b[1] = 0.99332 * b[1] + w * 0.0750759; b[2] = 0.96900 * b[2] + w * 0.1538520;
        b[3] = 0.86650 * b[3] + w * 0.3104856; b[4] = 0.55000 * b[4] + w * 0.5329522; b[5] = -0.7616 * b[5] - w * 0.0168980;
        y[(size_t) i] = (float) (0.05 * (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362)); b[6] = w * 0.115926; } return y; }
static std::vector<float> drums (int n, int sr)
{   std::vector<float> y ((size_t) n); XorShift32 r (5); const int step = sr * 60 / 120 / 4; // 16ths at 120 bpm
    for (int i = 0; i < n; ++i) { const int k = (i / step) % 16, t = i % step; const double ts = (double) t / sr; double v = 0;
        if (k % 4 == 0) v += 0.9 * std::sin (kTwoPi * (50 + 120 * std::exp (-ts * 30)) * ts) * std::exp (-ts * 9);    // kick
        if (k % 8 == 4) v += 0.5 * r.bipolar() * std::exp (-ts * 18) + 0.3 * std::sin (kTwoPi * 190 * ts) * std::exp (-ts * 25); // snare
        v += 0.12 * r.bipolar() * std::exp (-ts * 80);                                                                // hat
        y[(size_t) i] = (float) (0.5 * v); } return y; }
static std::vector<float> pad (int n, int sr)
{   std::vector<float> y ((size_t) n); const double f[] = { 130.81, 196.0, 261.63, 329.63, 392.0 }; double ph[5] {};
    for (int i = 0; i < n; ++i) { const double t = (double) i / sr, vib = 1 + 0.004 * std::sin (kTwoPi * 5.2 * t); double v = 0;
        for (int k = 0; k < 5; ++k) { ph[k] = wrap01 (ph[k] + f[k] * vib / sr); for (int h = 1; h <= 12; ++h) v += std::sin (kTwoPi * h * ph[k]) / h * (1.0 / (1 + std::pow ((h * f[k] - 700) / 400, 2))); } // formant ~700 Hz
        y[(size_t) i] = (float) (0.06 * v * (0.8 + 0.2 * std::sin (kTwoPi * 0.25 * t))); } return y; }

// returns number of failing cases: |dLUFS| > 0.5 LU, or pink-noise plain RMS > +0.5 dB over dry
inline int run (bool print, double& worst, double& worstPinkRms)
{
    const int sr = 48000, n = sr * 12;
    struct M { const char* name; std::vector<float> x; } mats[] = { { "pink", pink (n, 9) }, { "drums", drums (n, sr) }, { "pad", pad (n, sr) } };
    std::vector<Preset> list; Preset def { "Default (Mix 35%)", Params {} }; list.push_back (def);
    for (auto& p : factoryPresets()) list.push_back (p);
    if (print) std::printf ("%-18s %-6s %8s %8s\n", "preset", "prog", "dRMS dB", "dLUFS");
    worst = 0; worstPinkRms = -99; int fails = 0, rmsFails = 0;
    for (auto& pr : list)
        for (auto& m : mats)
        {
            WarpEngine e; e.prepare (sr); std::vector<float> L ((size_t) n), R ((size_t) n); HostInfo h; h.hasBpm = true; h.bpm = 120;
            for (int s = 0; s < n; s += 512) e.process (m.x.data() + s, m.x.data() + s, L.data() + s, R.data() + s, std::min (512, n - s), pr.p, h);
            const double dR = lufs::rmsDb (L, R) - lufs::rmsDb (m.x, m.x);
            const double dL = lufs::integrated (L, R, sr) - lufs::integrated (m.x, m.x, sr);
            if (print) std::printf ("%-18s %-6s %+8.2f %+8.2f%s%s\n", pr.name, m.name, dR, dL, std::abs (dL) > 0.5 ? "  <-- >0.5 LU" : "", (std::string (m.name) == "pink" && dR > 0.5) ? "  <-- pink RMS >+0.5 dB" : "");
            worst = std::max (worst, std::abs (dL)); fails += std::abs (dL) > 0.5; if (std::string (m.name) == "pink") { worstPinkRms = std::max (worstPinkRms, dR); rmsFails += dR > 0.5; }
        }
    if (print) std::printf ("%s level match: worst |dLUFS| = %.2f LU (limit 0.5), worst pink dRMS %+.2f dB (limit +0.5), %d cases over\n", fails + rmsFails ? "[FAIL]" : "[PASS]", worst, worstPinkRms, fails + rmsFails);
    return fails + rmsFails;
}
}
