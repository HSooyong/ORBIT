// ORBIT · Gravity Warp — DSP engine (JUCE-free, deterministic, header-only)
// OrbitEngine (orbit position) + GravityMapper (distance -> g -> mod targets)
// + WarpDelay (1-4 voice modulated delay) + TapeColor (tone / tape).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace orbit
{
constexpr double kPi    = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
constexpr float  kCentsPerLn = 1731.2340f; // 1200 / ln 2
constexpr float  kMaxCents   = 100.0f;     // hard pitch-deviation cap

// Sync: Free, 4/1, 2/1, 1/1, 1/2D, 1/2, 1/4D, 1/4, 1/8, 1/8T, 1/16 (beats per orbit)
constexpr int    kNumSync = 11;
constexpr double kSyncBeats[kNumSync] = { 0.0, 16.0, 8.0, 4.0, 3.0, 2.0, 1.5, 1.0, 0.5, 1.0 / 3.0, 0.25 };
constexpr int    kStageTable[9] = { 0, 2, 3, 4, 5, 6, 8, 12, 16 };

struct Params
{
    float rate = 0.35f; int sync = 0; float phase = 0.0f; float shape = 0.2f; int stages = 0;
    float ecc = 0.3f; float radius = 0.7f; float bhX = 0.85f; float bhY = 0.0f;
    float gravity = 0.5f; float horizon = 0.2f; float floor = 0.15f; float warp = 0.6f;
    float flutter = 0.3f; float chaos = 0.25f; int voices = 2; float width = 0.7f;
    float feedback = 0.0f; float tone = 0.0f; float drive = 0.0f; bool kepler = true;
    float mix = 0.35f; float outputDb = 0.0f; bool hold = false; float bloom = 0.0f;
};

struct HostInfo { bool hasBpm = false; double bpm = 120.0; bool playing = false; bool hasPpq = false; double ppq = 0.0; };

struct Telemetry { float px = 0, py = 0, bhX = 0, bhY = 0, d = 1, g = 0, cents = 0, wowHz = 0, bloom = 0; float voiceCents[4] {}; float theta = 0; };

struct XorShift32
{
    uint32_t s;
    explicit XorShift32 (uint32_t seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float bipolar() { return (float) ((next() >> 8) * (1.0 / 8388608.0)) - 1.0f; } // [-1,1)
};

struct OnePole // simple LP: y += a (x - y)
{
    float y = 0.0f;
    static float coef (double fc, double sr) { return (float) (1.0 - std::exp (-kTwoPi * fc / sr)); }
    float process (float x, float a) { y += a * (x - y); return y; }
};

inline float hermite (float xm1, float x0, float x1, float x2, float t)
{
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + x0;
}

inline double wrap01 (double x) { return x - std::floor (x); }
inline double wrapDiff (double d) { d = wrap01 (d + 0.5) - 0.5; return d; } // [-0.5,0.5)

// Reused from ORBIT v0.3 evaluateOrbitLfo(): sine<->triangle morph (wave part only)
inline float orbitWave (double phaseCycles, float shape)
{
    const double th = wrap01 (phaseCycles) * kTwoPi;
    const double s = std::sin (th);
    const double tri = (2.0 / kPi) * std::asin (std::clamp (s, -1.0, 1.0));
    return (float) (s + (double) shape * (tri - s));
}

class WarpEngine
{
public:
    static constexpr int kMaxVoices = 4;
    static constexpr int kCtrl = 16; // control-rate interval (samples)

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        const int need = (int) std::ceil (0.05 * sr) + 8;
        int sz = 1; while (sz < need) sz <<= 1;
        for (auto& b : buf) b.assign ((size_t) sz, 0.0f);
        mask = sz - 1;
        reset();
    }

    void reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        w = 0; tick = 0; first = true;
        u = 0.0; thetaS = 0.0; phaseS = 0.0; wowPh = 0.0; fl1 = fl2 = 0.0;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            rng[i] = XorShift32 (0x1234567u + 7919u * (uint32_t) i);
            nPos[i] = 0; nA[i] = 0; nB[i] = rng[i].bipolar(); chPh[i] = 0.25 * i; vGain[i] = 0; dCur[i] = dInc[i] = 0;
        }
        flRng = XorShift32 (0xC0FFEEu); flNoise = 0;
        for (int c = 0; c < 2; ++c) { fbHp[c] = fbLp[c] = toneLp[c] = shelfLp[c] = tapeLp[c] = OnePole(); fbSig[c] = 0; dcX[c] = dcY[c] = 0; }
        keplerEcc = -1.0f;
    }

    const Telemetry& telemetry() const { return tele; }

    // in/out may alias. inR may be null (mono in); outR may be null (mono out).
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, const Params& p, const HostInfo& host)
    {
        const double ctrlSec = kCtrl / sr;
        const float a30 = (float) (1.0 - std::exp (-ctrlSec / 0.010)); // ~30 ms settle (3 tau)
        const float a40 = (float) (1.0 - std::exp (-ctrlSec / 0.0133)); // ~40 ms
        const float a20 = (float) (1.0 - std::exp (-ctrlSec / 0.0067)); // ~20 ms
        const float a5  = (float) (1.0 - std::exp (-ctrlSec / 0.0017)); // ~5 ms
        const float aSyncSlew = (float) (1.0 - std::exp (-ctrlSec / 0.0033)); // ~10 ms
        const float perSample20 = (float) (1.0 - std::exp (-1.0 / (0.0067 * sr)));
        const float fadeStep = (float) (1.0 / (0.010 * sr));

        const int nv = std::clamp (p.voices, 1, kMaxVoices);
        const double beats = kSyncBeats[std::clamp (p.sync, 0, kNumSync - 1)];
        double rateHz = p.rate;
        if (beats > 0.0 && host.hasBpm && host.bpm > 0.0) rateHz = (host.bpm / 60.0) / beats;
        const bool lockPpq = beats > 0.0 && host.playing && host.hasPpq && host.hasBpm;

        const float mixT = std::clamp (p.mix, 0.0f, 1.0f);
        const float outT = std::pow (10.0f, std::clamp (p.outputDb, -12.0f, 12.0f) / 20.0f);
        if (first)
        {
            snapSmoothers (p);
            mixS = mixT; outS = outT;
            for (int i = 0; i < kMaxVoices; ++i) vGain[i] = i < nv ? 1.0f : 0.0f;
        }

        for (int s = 0; s < n; ++s)
        {
            if (tick == 0)
                controlTick (p, nv, rateHz, lockPpq, host, beats, s, a30, a40, a20, a5, aSyncSlew);
            tick = (tick + 1) % kCtrl;

            const float xl = inL[s];
            const float xr = inR ? inR[s] : xl;

            // ---- WarpDelay: read voices
            float wl = 0, wr = 0, gsum = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                const float tg = i < nv ? 1.0f : 0.0f;
                if (vGain[i] < tg) vGain[i] = std::min (tg, vGain[i] + fadeStep);
                else if (vGain[i] > tg) vGain[i] = std::max (tg, vGain[i] - fadeStep);
                dCur[i] += dInc[i];
                if (vGain[i] <= 0.0f) continue;
                const float tl = readTap (0, dCur[i]);
                const float tr = readTap (1, dCur[i]);
                const float pn = pan[i];
                float L, R;
                if (pn >= 0) { L = tl * (1 - pn); R = tr + tl * pn; }
                else         { R = tr * (1 + pn); L = tl - tr * pn; }
                wl += L * vGain[i]; wr += R * vGain[i]; gsum += vGain[i];
            }
            const float norm = 1.0f / std::sqrt (std::max (1.0f, gsum));
            wl *= norm; wr *= norm;

            // ---- feedback path (HP 120 Hz, LP tone-linked, tanh)
            const float wet2[2] = { wl, wr };
            for (int c = 0; c < 2; ++c)
            {
                float f = wet2[c] - fbHp[c].process (wet2[c], cHp120);
                f = fbLp[c].process (f, cFbLp);
                fbSig[c] = std::tanh (f * fbS);
            }
            buf[0][(size_t) w] = xl + fbSig[0];
            buf[1][(size_t) w] = xr + fbSig[1];
            w = (w + 1) & mask;

            // ---- TapeColor on wet
            float wet[2] = { wl, wr };
            for (int c = 0; c < 2; ++c)
            {
                float y = toneLp[c].process (wet[c], cToneLp);
                if (toneS > 0) y -= toneS * 0.6f * shelfLp[c].process (y, cShelf);
                if (driveS > 0.0005f)
                {
                    const float k = 1.0f + 4.0f * driveS, b = 0.15f * driveS;
                    const float tb = std::tanh (k * b);
                    float z = (std::tanh (k * (y + b)) - tb) / (k * (1.0f - tb * tb));
                    z = tapeLp[c].process (z, cTapeLp);
                    const float dc = z - dcX[c] + cDc * dcY[c]; dcX[c] = z; dcY[c] = dc;
                    y = dc;
                }
                wet[c] = y;
            }

            mixS += perSample20 * (mixT - mixS);
            outS += perSample20 * (outT - outS);
            const float dg = std::cos (mixS * 1.5707964f);
            const float wg = std::sin (mixS * 1.5707964f) * bloomGain;
            const float ol = (xl * dg + wet[0] * wg) * outS;
            const float orr = (xr * dg + wet[1] * wg) * outS;
            if (outR) { outL[s] = ol; outR[s] = orr; }
            else outL[s] = 0.5f * (ol + orr);
        }
        first = false;
    }

private:
    struct Sm { float v = 0; void step (float t, float a) { v += a * (t - v); } };

    float readTap (int c, float d) const
    {
        const float pos = (float) w - d;
        const float fl = std::floor (pos);
        const int ip = (int) fl;
        const float t = pos - fl;
        const auto& b = buf[c];
        auto at = [&] (int k) { return b[(size_t) (k & mask)]; };
        return hermite (at (ip - 1), at (ip), at (ip + 1), at (ip + 2), t);
    }

    void snapSmoothers (const Params& p)
    {
        gravS.v = p.gravity; warpS.v = p.warp; flutS.v = p.flutter; chaosS.v = p.chaos; floorS.v = p.floor;
        horS.v = p.horizon; radS.v = p.radius; eccS.v = p.ecc; widthS.v = p.width; fbSm.v = p.feedback;
        toneSm.v = p.tone; driveSm.v = p.drive; bhx = p.bhX; bhy = p.bhY; phaseS = p.phase; bloomS.v = 0;
    }

    void buildKepler (float e)
    {
        // dt/dθ ∝ ρ(θ)^2 (equal-area-ish), normalised so one orbit keeps the Rate/Sync period
        const double b = 1.0 - e;
        cum[0] = 0.0;
        for (int j = 0; j < kK; ++j)
        {
            const double th = (j + 0.5) / kK * kTwoPi;
            const double rho = std::max (0.3, std::sqrt (std::cos (th) * std::cos (th) + b * b * std::sin (th) * std::sin (th)));
            cum[j + 1] = cum[j] + rho * rho;
        }
        for (int j = 1; j <= kK; ++j) cum[j] /= cum[kK];
        keplerEcc = e;
    }

    double keplerTheta (double uu) const // uniform time fraction -> angle (cycles)
    {
        const double* it = std::upper_bound (cum.data(), cum.data() + kK + 1, uu);
        int j = (int) (it - cum.data()) - 1; j = std::clamp (j, 0, kK - 1);
        const double span = cum[j + 1] - cum[j];
        const double fr = span > 0 ? (uu - cum[j]) / span : 0.0;
        return (j + fr) / kK;
    }

    void controlTick (const Params& p, int nv, double rateHz, bool lockPpq, const HostInfo& host, double beats, int sOff,
                      float a30, float a40, float a20, float a5, float aSync)
    {
        // smoothing of macro params (30 ms), BH 40 ms, phase 20 ms (wrapped)
        gravS.step (p.gravity, a30); warpS.step (p.warp, a30); flutS.step (p.flutter, a30); chaosS.step (p.chaos, a30);
        floorS.step (p.floor, a30); horS.step (p.horizon, a30); radS.step (p.radius, a30); eccS.step (p.ecc, a30);
        widthS.step (p.width, a30); fbSm.step (p.feedback, a30); toneSm.step (p.tone, a30); driveSm.step (p.drive, a30);
        bhx += a40 * (p.bhX - bhx); bhy += a40 * (p.bhY - bhy);
        phaseS = wrap01 (phaseS + a20 * wrapDiff (p.phase - phaseS));

        // ---- OrbitEngine: uniform orbit phase u
        const double ctrlSec = kCtrl / sr;
        if (! p.hold)
        {
            u += rateHz * ctrlSec;
            if (lockPpq)
            {
                const double ppqNow = host.ppq + (sOff / sr) * host.bpm / 60.0;
                const double target = wrap01 (ppqNow / beats);
                u += aSync * wrapDiff (target - u);
            }
            u = wrap01 (u);
        }
        const float e = std::clamp (eccS.v, 0.0f, 0.9f);
        double thetaC;
        if (p.kepler) { if (std::abs (e - keplerEcc) > 1e-4f) buildKepler (e); thetaC = keplerTheta (u); }
        else thetaC = u;
        const int stages = kStageTable[std::clamp (p.stages, 0, 8)];
        if (stages > 1)
        {
            const double q = std::floor (thetaC * stages + 1e-9) / stages;
            thetaS = first ? q : wrap01 (thetaS + a5 * wrapDiff (q - thetaS));
        }
        else thetaS = thetaC;
        const double th = (thetaS + phaseS) * kTwoPi;
        const float R = std::clamp (radS.v, 0.2f, 1.0f);
        const float px = R * (float) std::cos (th);
        const float py = R * (1.0f - e) * (float) std::sin (th);

        // ---- GravityMapper
        const float d = std::clamp (std::hypot (px - bhx, py - bhy) / 2.0f, 0.0f, 1.0f);
        const float rs = std::clamp (horS.v, 0.05f, 0.5f);
        const float g = std::clamp (gravS.v, 0.0f, 1.0f) / (1.0f + std::pow (d / rs, 2.5f));
        const float bloomT = std::clamp (p.bloom, 0.0f, 1.0f) * std::clamp ((rs * 0.3f - d) / (rs * 0.15f), 0.0f, 1.0f);
        bloomS.step (bloomT, a30);

        const float fWow = 0.25f + 1.2f * g;
        float cWow = std::clamp (warpS.v, 0.0f, 1.0f) * 120.0f * std::pow (g, 1.5f);
        const float chaosAmt = std::clamp (chaosS.v, 0.0f, 1.0f) * std::pow (g, 1.5f);
        float cFl = std::clamp (flutS.v, 0.0f, 1.0f) * 35.0f * g * g * g;
        float cCh = 20.0f * std::clamp (floorS.v, 0.0f, 1.0f) * (0.3f + 0.7f * (1.0f - g));
        const float est = cWow * (1.0f + 0.5f * chaosAmt) + cFl + cCh;
        const float capT = 0.88f * kMaxCents; // headroom for rate-mod / interpolation overshoot
        const float sc = est > capT ? capT / est : 1.0f;
        cWow *= sc; cFl *= sc; cCh *= sc;

        flNoise += 0.05f * (flRng.bipolar() - flNoise);
        const float fFl = (6.0f + 8.0f * g) * (1.0f + 0.2f * std::clamp (flNoise * 4.0f, -1.0f, 1.0f));

        const float base = 0.007f + 0.008f * g; // seconds
        const float k2pi = kCentsPerLn * (float) kTwoPi;
        const float aWow = std::min (cWow / (k2pi * fWow), base - 0.0015f);
        const float aFl = cFl / (k2pi * fFl);

        wowPh = wrap01 (wowPh + fWow * ctrlSec);
        fl1 = wrap01 (fl1 + fFl * ctrlSec);
        fl2 = wrap01 (fl2 + fFl * 1.618 * ctrlSec);
        const float flv = 0.7f * (float) std::sin (fl1 * kTwoPi) + 0.3f * (float) std::sin (fl2 * kTwoPi);

        const float cN = cWow * 0.5f * chaosAmt; // smoothed-noise wow (tape instability)
        const float width = std::clamp (widthS.v, 0.0f, 1.0f);
        float centsMax = 0;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            const double fCh = 0.55 + 0.21 * i;
            chPh[i] = wrap01 (chPh[i] + fCh * ctrlSec);
            // band-limited random: cosine interpolation between random points every T s
            const double T = 0.55 + 0.07 * i;
            nPos[i] += ctrlSec / T;
            if (nPos[i] >= 1.0) { nPos[i] -= 1.0; nA[i] = nB[i]; nB[i] = rng[i].bipolar(); }
            const float nz = nA[i] + (nB[i] - nA[i]) * (float) (0.5 - 0.5 * std::cos (kPi * nPos[i]));
            const float aNz = cN / (kCentsPerLn * (float) (kPi / (2.0 * T)));
            const float aCh = cCh / (k2pi * (float) fCh);
            // voice spread phase / pan glide when Voices changes (no zipper)
            vPhOff[i] = wrap01 (vPhOff[i] + (first ? 1.0f : a30) * wrapDiff ((double) i / nv - vPhOff[i]));
            const float wv = orbitWave (wowPh + vPhOff[i], std::clamp (p.shape, 0.0f, 1.0f));
            const float off = 0.0011f * i;
            float dSec = base + off + aWow * wv + aNz * nz + aFl * flv
                         + aCh * (float) std::sin (chPh[i] * kTwoPi);
            dSec = std::clamp (dSec, 0.0005f, 0.045f);
            const float target = dSec * (float) sr;
            if (first) { dCur[i] = target; dInc[i] = 0; }
            else dInc[i] = std::clamp ((target - dCur[i]) / (float) kCtrl, -kMaxSlew, kMaxSlew); // hard pitch ceiling

            float pn = nv > 1 ? (-1.0f + 2.0f * i / (nv - 1)) * width : 0.0f;
            const float panT = std::clamp (pn + 0.15f * px, -1.0f, 1.0f);
            pan[i] = first ? panT : pan[i] + a30 * (panT - pan[i]);
            tele.voiceCents[i] = cWow + cFl + cCh;
            centsMax = std::max (centsMax, tele.voiceCents[i]);
        }

        // filters / gains
        fbS = std::clamp (fbSm.v + 0.10f * bloomS.v, 0.0f, 0.70f);
        bloomGain = std::pow (10.0f, -1.5f * bloomS.v / 20.0f);
        toneS = std::clamp (toneSm.v, -1.0f, 1.0f);
        driveS = std::clamp (driveSm.v, 0.0f, 1.0f);
        const double toneCut = toneS < 0 ? 18000.0 - 14000.0 * (-toneS) : 18000.0;
        const double gCut = 18000.0 - 12000.0 * g;
        const double nyq = 0.45 * sr;
        cToneLp = OnePole::coef (std::min ({ toneCut, gCut, nyq }), sr);
        cFbLp = OnePole::coef (std::min (toneCut, nyq), sr);
        cHp120 = OnePole::coef (120.0, sr);
        cShelf = OnePole::coef (250.0, sr);
        cTapeLp = OnePole::coef (std::min (16000.0 - 8000.0 * driveS, nyq), sr);
        cDc = (float) std::exp (-kTwoPi * 10.0 / sr);

        tele = Telemetry { px, py, bhx, bhy, d, g, std::min (centsMax, kMaxCents), fWow, bloomS.v,
                           { tele.voiceCents[0], tele.voiceCents[1], tele.voiceCents[2], tele.voiceCents[3] }, (float) thetaS };
        first = false;
    }

    double sr = 48000.0;
    std::array<std::vector<float>, 2> buf;
    int mask = 0, w = 0, tick = 0;
    bool first = true;

    static constexpr float kMaxSlew = 0.0594f; // |d delay/dt| <= 1 - 2^(-100/1200)
    std::array<double, 4> vPhOff {};
    double u = 0, thetaS = 0, phaseS = 0, wowPh = 0, fl1 = 0, fl2 = 0;
    float bhx = 0, bhy = 0;
    Sm gravS, warpS, flutS, chaosS, floorS, horS, radS, eccS, widthS, fbSm, toneSm, driveSm, bloomS;
    float mixS = 0, outS = 1;

    static constexpr int kK = 256;
    std::array<double, kK + 1> cum {};
    float keplerEcc = -1;

    XorShift32 rng[kMaxVoices], flRng;
    float nA[kMaxVoices] {}, nB[kMaxVoices] {}, flNoise = 0;
    double nPos[kMaxVoices] {};
    double chPh[kMaxVoices] {};
    float vGain[kMaxVoices] {}, dCur[kMaxVoices] {}, dInc[kMaxVoices] {}, pan[kMaxVoices] {};

    OnePole fbHp[2], fbLp[2], toneLp[2], shelfLp[2], tapeLp[2];
    float fbSig[2] {}, dcX[2] {}, dcY[2] {};
    float fbS = 0, bloomGain = 1, toneS = 0, driveS = 0;
    float cToneLp = 1, cFbLp = 1, cHp120 = 0, cShelf = 0, cTapeLp = 1, cDc = 0.999f;
    Telemetry tele;
};
} // namespace orbit
