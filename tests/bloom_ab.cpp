#include "engine/Presets.h"
#include <cstdio>
#include <string>
using namespace orbit;
void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr);
int main (int, char** argv)
{
    const int sr = 48000, n = sr * 16; std::vector<float> in ((size_t) n);
    const double fr[] = { 220.0, 277.18, 329.63, 440.0 };
    for (int i = 0; i < n; ++i) { double v = 0; for (double f : fr) v += (2 * wrap01 (f * i / sr) - 1) * 0.08; in[(size_t) i] = (float) v; }
    auto ps = factoryPresets();
    for (int k : { 3, 4, 5, 6 })
        for (float bl : { 0.0f, 0.15f, 0.20f, 0.25f, 0.60f })
        {
            Params p = ps[(size_t) k].p; p.bloom = bl;
            WarpEngine e; e.prepare (sr); std::vector<float> L ((size_t) n), R ((size_t) n); HostInfo h; h.hasBpm = true; h.bpm = 120;
            float maxB = 0, minD = 1; double eWet = 0;
            for (int s = 0; s < n; s += 256) { e.process (in.data() + s, in.data() + s, L.data() + s, R.data() + s, 256, p, h);
                maxB = std::max (maxB, e.telemetry().bloom); minD = std::min (minD, e.telemetry().d); }
            for (float v : L) eWet += (double) v * v;
            std::printf ("%-16s bloom %3.0f%%: min d %.3f, max bloom env %.2f, rms %.4f\n", ps[(size_t) k].name, bl * 100, minD, maxB, std::sqrt (eWet / n));
            if (bl == 0.0f || bl == 0.20f || bl == 0.60f) { std::string nm = ps[(size_t) k].name; for (auto& c : nm) if (c == ' ') c = '_';
                writeWav (std::string (argv[1]) + "/ab_" + nm + "_bloom" + std::to_string ((int) (bl * 100)) + ".wav", L, R, sr); }
        }
}
void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), data = n * 4;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + data); std::fwrite ("WAVEfmt ", 1, 8, f); u32 (16); u16 (1); u16 (2);
    u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16); std::fwrite ("data", 1, 4, f); u32 (data);
    for (uint32_t i = 0; i < n; ++i) for (float v : { L[i], R[i] }) { int16_t q = (int16_t) std::lround (std::clamp (v, -1.0f, 1.0f) * 32767.0f); std::fwrite (&q, 2, 1, f); }
    std::fclose (f);
}
