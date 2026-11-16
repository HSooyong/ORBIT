#pragma once
// ITU-R BS.1770-4 integrated loudness (K-weighting, 400 ms / 75 % blocks, -70 LUFS abs + -10 LU rel gate)
#include <cmath>
#include <vector>
namespace lufs {
constexpr double kPi = 3.14159265358979323846;
struct Biquad { double b0, b1, b2, a1, a2, z1 = 0, z2 = 0;
    double p (double x) { double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } };
inline void kFilters (double fs, Biquad& s1, Biquad& s2)
{   // pre-filter (high shelf) + RLB high-pass, bilinear-designed for any fs (pyloudnorm / libebur128 parameters)
    { const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
      const double K = std::tan (kPi * f0 / fs), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
      const double a0 = 1.0 + K / Q + K * K;
      s1 = { (Vh + Vb * K / Q + K * K) / a0, 2.0 * (K * K - Vh) / a0, (Vh - Vb * K / Q + K * K) / a0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 }; }
    { const double f0 = 38.13547087602444, Q = 0.5003270373238773, K = std::tan (kPi * f0 / fs);
      const double a0 = 1.0 + K / Q + K * K;
      s2 = { 1.0, -2.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 }; }
}
inline double integrated (const std::vector<float>& L, const std::vector<float>& R, double fs)
{
    std::vector<double> z[2];
    for (int c = 0; c < 2; ++c) { Biquad a, b; kFilters (fs, a, b); const auto& x = c ? R : L; z[c].resize (x.size());
        for (size_t i = 0; i < x.size(); ++i) { double y = b.p (a.p (x[i])); z[c][i] = y * y; } }
    const size_t blk = (size_t) (0.4 * fs), hop = blk / 4; std::vector<double> bl;
    for (size_t s = 0; s + blk <= L.size(); s += hop) { double m = 0; for (int c = 0; c < 2; ++c) for (size_t i = s; i < s + blk; ++i) m += z[c][i]; bl.push_back (m / (double) blk); }
    auto lk = [] (double m) { return -0.691 + 10.0 * std::log10 (m + 1e-30); };
    double sum = 0; int n = 0; for (double m : bl) if (lk (m) > -70) { sum += m; ++n; }
    if (! n) return -200; const double rel = lk (sum / n) - 10.0; sum = 0; n = 0;
    for (double m : bl) if (lk (m) > -70 && lk (m) > rel) { sum += m; ++n; }
    return n ? lk (sum / n) : -200;
}
inline double rmsDb (const std::vector<float>& L, const std::vector<float>& R)
{ double e = 0; for (size_t i = 0; i < L.size(); ++i) e += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]); return 10 * std::log10 (e / (double) L.size() + 1e-30); }
}
