// Offline render tests for ORBIT · Gravity Warp engine (no JUCE).
#include "level_match.h"
#include "EditorLayout.h"
#include "engine/WarpEngine.h"
#include "engine/Presets.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <chrono>
#include <complex>

using namespace orbit;
static int fails = 0;
static void check (bool ok, const std::string& what) { std::printf ("[%s] %s\n", ok ? "PASS" : "FAIL", what.c_str()); if (! ok) ++fails; }

static void render (WarpEngine& e, const std::vector<float>& inL, const std::vector<float>& inR,
                    std::vector<float>& oL, std::vector<float>& oR, const Params& p, int block)
{
    const int n = (int) inL.size(); oL.assign ((size_t) n, 0); oR.assign ((size_t) n, 0);
    HostInfo h;
    for (int s = 0; s < n; s += block)
    {
        const int m = std::min (block, n - s);
        e.process (inL.data() + s, inR.data() + s, oL.data() + s, oR.data() + s, m, p, h);
    }
}

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), data = n * 4;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + data); std::fwrite ("WAVEfmt ", 1, 8, f); u32 (16); u16 (1); u16 (2);
    u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16); std::fwrite ("data", 1, 4, f); u32 (data);
    for (uint32_t i = 0; i < n; ++i) for (float v : { L[i], R[i] }) { int16_t q = (int16_t) std::lround (std::clamp (v, -1.0f, 1.0f) * 32767.0f); std::fwrite (&q, 2, 1, f); }
    std::fclose (f);
}

// instantaneous frequency via interpolated upward zero crossings -> cents vs ref
struct CentStats { float maxAbs = 0, rms = 0; };
static CentStats measureCents (const std::vector<float>& x, int sr, double f0, int skip)
{
    std::vector<double> zc;
    for (size_t i = (size_t) skip + 1; i < x.size(); ++i)
        if (x[i - 1] < 0 && x[i] >= 0) zc.push_back ((double) (i - 1) + x[i - 1] / (x[i - 1] - x[i]));
    CentStats st; double acc = 0; int cnt = 0;
    for (size_t k = 1; k < zc.size(); ++k)
    {
        const double f = sr / (zc[k] - zc[k - 1]);
        const double c = 1200.0 * std::log2 (f / f0);
        st.maxAbs = std::max (st.maxAbs, (float) std::abs (c)); acc += c * c; ++cnt;
    }
    st.rms = cnt ? (float) std::sqrt (acc / cnt) : 0.0f;
    return st;
}


static void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[i], a[j]); }
    for (size_t len = 2; len <= n; len <<= 1) { const double ang = -kTwoPi / (double) len; std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len) { std::complex<double> w (1); for (size_t k = 0; k < len / 2; ++k) { auto u = a[i + k], v = a[i + k + len / 2] * w; a[i + k] = u + v; a[i + k + len / 2] = u - v; w *= wl; } } }
}
// worst 1/3-octave band (100 Hz..16 kHz) of mono-sum power relative to mean channel power, dB
static float monoSumWorstDb (const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    const size_t N = 4096; std::vector<double> pSum (N / 2), pCh (N / 2);
    for (size_t off = (size_t) sr; off + N <= L.size(); off += N / 2)
    {
        std::vector<std::complex<double>> a (N), b (N);
        for (size_t i = 0; i < N; ++i) { double w = 0.5 - 0.5 * std::cos (kTwoPi * i / N); a[i] = L[off + i] * w; b[i] = R[off + i] * w; }
        fft (a); fft (b);
        for (size_t k = 0; k < N / 2; ++k) { pSum[k] += std::norm (0.5 * (a[k] + b[k])); pCh[k] += 0.5 * (std::norm (a[k]) + std::norm (b[k])); }
    }
    float worst = 0;
    for (double fc = 100; fc <= 16000; fc *= std::pow (2.0, 1.0 / 3.0))
    {
        double s = 0, c = 0; for (size_t k = 0; k < N / 2; ++k) { double f = (double) k * sr / N; if (f >= fc / 1.122 && f < fc * 1.122) { s += pSum[k]; c += pCh[k]; } }
        if (c > 0) worst = std::min (worst, (float) (10 * std::log10 (s / c)));
    }
    return worst;
}

static Params zoneParams (float d)
{
    Params p; p.hold = true; p.ecc = 0; p.radius = 0.7f; p.phase = 0; p.kepler = false;
    p.gravity = 1; p.warp = 1; p.flutter = 1; p.chaos = 0; p.voices = 1; p.width = 0; p.mix = 1; p.feedback = 0;
    p.bhX = 0.7f - 2.0f * d; p.bhY = 0; // planet held at (0.7,0): normalised distance = d
    return p;
}

static CentStats zone (float d, int sr, Params* override = nullptr)
{
    const int n = sr * 6; std::vector<float> in ((size_t) n), oL, oR;
    for (int i = 0; i < n; ++i) in[(size_t) i] = 0.5f * (float) std::sin (kTwoPi * 1000.0 * i / sr);
    WarpEngine e; e.prepare (sr);
    Params p = override ? *override : zoneParams (d);
    render (e, in, in, oL, oR, p, 512);
    return measureCents (oL, sr, 1000.0, sr); // skip first second
}

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : "renders";
    const int sr = 48000;

    // 1) silent input -> exact silence (all presets + extreme settings)
    {
        auto presets = factoryPresets();
        std::vector<Params> sets; for (auto& pr : presets) sets.push_back (pr.p);
        Params x = zoneParams (0.02f); x.feedback = 0.7f; x.drive = 1; x.bloom = 1; x.chaos = 1; x.tone = 0.8f; x.voices = 4; sets.push_back (x);
        float worst = 0;
        for (auto& p : sets)
        {
            WarpEngine e; e.prepare (sr);
            std::vector<float> z ((size_t) sr * 10, 0.0f), oL, oR;
            render (e, z, z, oL, oR, p, 333);
            for (size_t i = 0; i < oL.size(); ++i) worst = std::max ({ worst, std::abs (oL[i]), std::abs (oR[i]) });
        }
        char b[128]; std::snprintf (b, sizeof b, "silent input -> silence (8 settings x 10 s), peak = %.3g", worst);
        check (worst == 0.0f, b);
    }

    // 2) pitch deviation per zone (1 kHz sine, wet only, 1 voice, chaos 0, Gravity/Wow/Flutter 100%)
    struct Z { const char* name; float d, lo, hi; } zones[] = {
        { "far  (d=0.80)", 0.80f, 0.5f, 4.0f }, { "mid  (d=0.40)", 0.40f, 5.0f, 25.0f }, { "mid  (d=0.30)", 0.30f, 8.0f, 30.0f },
        { "near (d=0.15)", 0.15f, 50.0f, 100.0f }, { "near (d=0.05)", 0.05f, 50.0f, 100.0f } };
    for (auto& z : zones)
    {
        auto st = zone (z.d, sr);
        char b[160]; std::snprintf (b, sizeof b, "zone %s: max |dev| = %6.2f cent, rms = %5.2f (expect %.1f..%.1f)", z.name, st.maxAbs, st.rms, z.lo, z.hi);
        check (st.maxAbs >= z.lo && st.maxAbs <= z.hi, b);
    }
    // cap with chaos 100% + fast settings
    {
        Params p = zoneParams (0.0f); p.chaos = 1.0f; p.shape = 1.0f;
        auto st = zone (0, sr, &p);
        char b[128]; std::snprintf (b, sizeof b, "cap: d=0, chaos 100%%: max |dev| = %.2f cent (expect <= 100)", st.maxAbs);
        check (st.maxAbs <= 100.0f, b);
    }
    // voice 0 instantaneous pitch is observed from the delay slope and is not fed back into the audio path
    {
        const int n = sr; std::vector<float> in ((size_t) n), oL, oR;
        for (int i = 0; i < n; ++i) in[(size_t) i] = 0.5f * (float) std::sin (kTwoPi * 1000.0 * i / sr);
        WarpEngine e; e.prepare (sr); Params p = zoneParams (0.15f); HostInfo h;
        oL.assign ((size_t) n, 0); oR.assign ((size_t) n, 0);
        float lo = 1.0e9f, hi = -1.0e9f;
        for (int s = 0; s < n; s += 256)
        {
            const int m = std::min (256, n - s);
            e.process (in.data() + s, in.data() + s, oL.data() + s, oR.data() + s, m, p, h);
            const float c = e.telemetry().instantCents;
            lo = std::min (lo, c); hi = std::max (hi, c);
        }
        const float width = e.telemetry().cents;
        char b[240]; std::snprintf (b, sizeof b, "instant pitch: %+.2f..%+.2f cent, width %.2f (finite, |instant|<=110, width<=100, moves)", lo, hi, width);
        check (std::isfinite (lo) && std::isfinite (hi) && std::abs (lo) <= 110.0f && std::abs (hi) <= 110.0f && width <= 100.0f && width >= 0.0f && (hi - lo) > 0.5f, b);
    }
    // shared education formula (Low Orbit defaults) — observation only, not a second engine
    {
        const auto mid = mapModulation (0.30f, 0.5f, 0.2f, 0.6f, 0.3f, 0.15f, 0.25f);
        const auto near = mapModulation (0.10f, 0.5f, 0.2f, 0.6f, 0.3f, 0.15f, 0.25f);
        const auto far = mapModulation (0.80f, 0.5f, 0.2f, 0.6f, 0.3f, 0.15f, 0.25f);
        const float midSum = mid.cWow + mid.cFlutter + mid.cChorus;
        const float nearSum = near.cWow + near.cFlutter + near.cChorus;
        const float farSum = far.cWow + far.cFlutter + far.cChorus;
        char b[200];
        std::snprintf (b, sizeof b, "education formula Low Orbit: far %.2f mid %.2f near %.2f (expect 3.1 / 6.2 / 22.9)", farSum, midSum, nearSum);
        check (std::abs (farSum - 3.1f) < 0.15f && std::abs (midSum - 6.2f) < 0.15f && std::abs (nearSum - 22.9f) < 0.2f, b);
    }
    // default preset (Low Orbit) wet-only, far zone
    {
        Params p = factoryPresets()[0].p; p.mix = 1; p.voices = 1; p.hold = true; p.phase = 0.5f; // planet at (-0.7,0) far from BH
        auto st = zone (0, sr, &p);
        char b[128]; std::snprintf (b, sizeof b, "Low Orbit far position: max |dev| = %.2f cent (expect <= 4)", st.maxAbs);
        check (st.maxAbs <= 4.0f, b);
    }

    // 3) sample-rate independence of zone deviation
    for (float d : { 0.8f, 0.4f, 0.15f })
    {
        float mn = 1e9f, mx = 0;
        for (int r : { 44100, 48000, 88200, 96000, 192000 }) { auto st = zone (d, r); mn = std::min (mn, st.rms); mx = std::max (mx, st.rms); }
        char b[128]; std::snprintf (b, sizeof b, "SR 44.1-192k, d=%.2f: rms cent spread = %.3f (target <= 0.5)", d, mx - mn);
        check (mx - mn <= 0.5f, b);
    }

    // 4) determinism: block size 1 / 64 / 517 / 4096 identical
    {
        const int n = sr * 4; std::vector<float> in ((size_t) n);
        XorShift32 r (42); for (auto& v : in) v = 0.3f * r.bipolar();
        Params p = factoryPresets()[2].p; // Worn Cassette (chaos/tape on)
        std::vector<float> refL, refR; { WarpEngine e; e.prepare (sr); render (e, in, in, refL, refR, p, 4096); }
        float worst = 0;
        for (int bs : { 1, 64, 517 })
        {
            WarpEngine e; e.prepare (sr); std::vector<float> oL, oR; render (e, in, in, oL, oR, p, bs);
            for (int i = 0; i < n; ++i) worst = std::max (worst, std::abs (oL[(size_t) i] - refL[(size_t) i]));
        }
        char b[128]; std::snprintf (b, sizeof b, "determinism across block sizes: max diff = %.3g", worst);
        check (worst == 0.0f, b);
        WarpEngine e2; e2.prepare (sr); std::vector<float> oL, oR; render (e2, in, in, oL, oR, p, 4096);
        check (std::memcmp (oL.data(), refL.data(), oL.size() * 4) == 0, "fixed seed: two runs bit-identical");
    }

    // 5) feedback 70% + bloom stays bounded
    {
        Params p = zoneParams (0.05f); p.feedback = 0.7f; p.bloom = 1; p.voices = 4; p.chaos = 1; p.hold = false;
        const int n = sr * 8; std::vector<float> in ((size_t) n, 0.0f), oL, oR; XorShift32 r (7);
        for (int i = 0; i < sr; ++i) in[(size_t) i] = 0.5f * r.bipolar();
        WarpEngine e; e.prepare (sr); render (e, in, in, oL, oR, p, 256);
        float pk = 0, tail = 0; for (int i = 0; i < n; ++i) { pk = std::max (pk, std::abs (oL[(size_t) i])); if (i > n - sr) tail = std::max (tail, std::abs (oL[(size_t) i])); }
        char b[128]; std::snprintf (b, sizeof b, "feedback 70%% stable: peak %.2f, last-second peak %.2g", pk, tail);
        check (pk < 4.0f && tail < 1e-3f && std::isfinite (pk), b);
    }


    // 7) mono-sum: L+R vs mean channel power per 1/3 octave (white noise, mono in -> stereo, each preset)
    {
        const int n = sr * 8; std::vector<float> in ((size_t) n); XorShift32 r (99); for (auto& v : in) v = 0.3f * r.bipolar();
        auto ps = factoryPresets(); float worstAll = 0; std::string who;
        for (auto& pr : ps)
        {
            WarpEngine e; e.prepare (sr); std::vector<float> oL, oR; render (e, in, in, oL, oR, pr.p, 512);
            float w = monoSumWorstDb (oL, oR, sr);
            std::printf ("       mono-sum %-16s worst band %.2f dB\n", pr.name, w);
            if (w < worstAll) { worstAll = w; who = pr.name; }
        }
        char b[160]; std::snprintf (b, sizeof b, "mono-sum: no 1/3-oct band below -2 dB (worst %.2f dB, %s)", worstAll, who.c_str());
        check (worstAll > -2.0f, b);
    }

    // 8) zipper: fast black-hole jumps / param jumps -> no clicks (2nd-difference peak vs steady)
    {
        const int n = sr * 6; std::vector<float> in ((size_t) n);
        for (int i = 0; i < n; ++i) in[(size_t) i] = 0.5f * (float) std::sin (kTwoPi * 1000.0 * i / sr);
        auto d2peak = [&] (const std::vector<float>& y, int from, int to) { float m = 0; for (int i = std::max (2, from); i < to; ++i) m = std::max (m, std::abs (y[(size_t) i] - 2 * y[(size_t) i - 1] + y[(size_t) i - 2])); return m; };
        Params base = factoryPresets()[0].p;
        auto setState = [] (Params& q, bool a) { q.bhX = a ? -1.0f : 0.7f; q.bhY = a ? 1.0f : 0.0f; q.gravity = a ? 0.0f : 1.0f;
            q.mix = a ? 0.0f : 1.0f; q.outputDb = a ? -12.0f : 6.0f; q.voices = a ? 1 : 4; q.phase = a ? 0.1f : 0.9f; q.tone = a ? -1.0f : 1.0f; };
        float steady = 0; // reference = worst steady-state of the two end states (same modulation, no jumps)
        for (bool a : { false, true }) { Params q = base; setState (q, a); WarpEngine e0; e0.prepare (sr); std::vector<float> sL, sR; render (e0, in, in, sL, sR, q, 256); steady = std::max (steady, d2peak (sL, sr, n)); }
        WarpEngine e; e.prepare (sr); std::vector<float> L ((size_t) n), R ((size_t) n); HostInfo h; Params p = base;
        int blk = 0;
        for (int s = 0; s < n; s += 128, ++blk)
        {
            if (s > sr) setState (p, (blk / 8) % 2);
            e.process (in.data() + s, in.data() + s, L.data() + s, R.data() + s, std::min (128, n - s), p, h);
        }
        const float jumpy = d2peak (L, sr, n);
        char b[160]; std::snprintf (b, sizeof b, "zipper (8 params jumping every 21 ms): worst 2nd-diff during jumps %.4f vs steady %.4f (ratio %.2f, expect <= 1.5)", jumpy, steady, jumpy / steady);
        check (jumpy <= 1.5f * steady, b);
    }

    // 9) CPU: 4 voices, worst-case settings, 48 kHz stereo, 512 blocks
    {
        Params p = zoneParams (0.1f); p.voices = 4; p.feedback = 0.5f; p.drive = 0.5f; p.tone = 0.5f; p.chaos = 1; p.hold = false; p.mix = 0.5f;
        const int n = sr * 20; std::vector<float> in ((size_t) n); XorShift32 r (3); for (auto& v : in) v = 0.3f * r.bipolar();
        std::vector<float> L ((size_t) n), R ((size_t) n); WarpEngine e; e.prepare (sr); HostInfo h;
        auto t0 = std::chrono::steady_clock::now();
        for (int s = 0; s < n; s += 512) e.process (in.data() + s, in.data() + s, L.data() + s, R.data() + s, 512, p, h);
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        const double pct = 100.0 * sec / 20.0;
        Params q = factoryPresets()[0].p; WarpEngine e2; e2.prepare (sr);
        t0 = std::chrono::steady_clock::now();
        for (int s = 0; s < n; s += 512) e2.process (in.data() + s, in.data() + s, L.data() + s, R.data() + s, 512, q, h);
        const double pct2 = 100.0 * std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count() / 20.0;
        char b[160]; std::snprintf (b, sizeof b, "CPU (1 core): worst-case 4 voices %.2f %%, default preset %.2f %% (target < 2 %%)", pct, pct2);
        check (pct < 2.0, b);
    }

    // 6) listening renders: chord-ish saw pad through presets + BH sweep
    {
        const int n = sr * 12; std::vector<float> in ((size_t) n);
        const double fr[] = { 220.0, 277.18, 329.63, 440.0 };
        for (int i = 0; i < n; ++i) { double v = 0; for (double f : fr) { double ph = wrap01 (f * i / sr); v += (2 * ph - 1) * 0.08; } in[(size_t) i] = (float) v; }
        auto presets = factoryPresets();
        for (size_t k = 0; k < presets.size(); ++k)
        {
            WarpEngine e; e.prepare (sr); std::vector<float> oL, oR; render (e, in, in, oL, oR, presets[k].p, 512);
            std::string nm = presets[k].name; for (auto& c : nm) if (c == ' ') c = '_';
            writeWav (outDir + "/preset_" + std::to_string (k + 1) + "_" + nm + ".wav", oL, oR, sr);
        }
        // sweep: BH moves from far (-1) through orbit to near over 12 s, wet 100%
        WarpEngine e; e.prepare (sr); std::vector<float> oL ((size_t) n), oR ((size_t) n);
        Params p; p.mix = 1.0f; p.gravity = 1.0f; HostInfo h;
        for (int s = 0; s < n; s += 512) { p.bhX = -1.0f + 1.7f * (float) s / n; p.bhY = 0.0f; e.process (in.data() + s, in.data() + s, oL.data() + s, oR.data() + s, std::min (512, n - s), p, h); }
        writeWav (outDir + "/sweep_far_to_near.wav", oL, oR, sr);
        writeWav (outDir + "/dry_input.wav", in, in, sr);
        std::printf ("[INFO] renders written to %s\n", outDir.c_str());
    }

    // level match: default + 7 presets x {pink, drums, pad}, integrated LUFS within +/-0.5 LU of dry
    {
        double worst = 0, pr = 0; const int over = levelmatch::run (false, worst, pr);
        char b[160]; std::snprintf (b, sizeof b, "level match vs dry: worst |dLUFS| %.2f LU over 24 cases (limit 0.5), worst pink dRMS %+.2f dB (limit +0.5), %d over", worst, pr, over);
        check (over == 0, b);
    }

    std::printf ("\n%s (%d failure%s)\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
