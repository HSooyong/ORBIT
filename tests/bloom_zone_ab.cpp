// A/B (same preset settings in both engines): bloom zone 0.3*Horizon (old, commit 350c85d engine) vs 0.6*Horizon (new), all factory presets.
#include "ref/Presets_350c85d.h"
#include "engine/Presets.h"
#include <cstdio>
#include <cstring>
static_assert (sizeof (orbit_old::Params) == sizeof (orbit::Params), "same Params layout");
template <class E, class P, class H> static std::vector<float> run (const std::vector<float>& in, P p, float& maxB)
{
    E e; e.prepare (48000); std::vector<float> L (in.size()), R (in.size()); H h; h.hasBpm = true; h.bpm = 120; maxB = 0;
    for (size_t s = 0; s < in.size(); s += 256) { e.process (in.data() + s, in.data() + s, L.data() + s, R.data() + s, 256, p, h); maxB = std::max (maxB, e.telemetry().bloom); }
    L.insert (L.end(), R.begin(), R.end()); return L;
}
int main()
{
    const int sr = 48000, n = sr * 16; std::vector<float> in ((size_t) n);
    for (int i = 0; i < n; ++i) { double v = 0; for (double f : { 220.0, 277.18, 329.63, 440.0 }) v += (2 * orbit::wrap01 (f * i / sr) - 1) * 0.08; in[(size_t) i] = (float) v; }
    auto po = orbit_old::factoryPresets(); auto pn = orbit::factoryPresets();
    std::printf ("%-16s %6s %8s %8s %9s %10s\n", "preset", "bloom", "envOld", "envNew", "dRMS dB", "null dB");
    for (size_t k = 0; k < pn.size(); ++k)
    {
        float bo, bn;
        orbit_old::Params same; std::memcpy (&same, &pn[k].p, sizeof same); // identical settings -> isolates the zone change
        (void) po;
        auto a = run<orbit_old::WarpEngine, orbit_old::Params, orbit_old::HostInfo> (in, same, bo);
        auto b = run<orbit::WarpEngine, orbit::Params, orbit::HostInfo> (in, pn[k].p, bn);
        double ea = 0, eb = 0, ed = 0; for (size_t i = 0; i < a.size(); ++i) { ea += a[i] * a[i]; eb += b[i] * b[i]; ed += (a[i] - b[i]) * (a[i] - b[i]); }
        std::printf ("%-16s %5.0f%% %8.2f %8.2f %9.2f %10.1f\n", pn[k].name, pn[k].p.bloom * 100, bo, bn, 10 * std::log10 (eb / ea), ed > 0 ? 10 * std::log10 (ed / ea) : -999.0);
    }
}
