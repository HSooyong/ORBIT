#pragma once

namespace orbit::ui
{

// Education window. Same family size as the other Bearfield plugins.
inline constexpr int kDefaultWidth = 1280;
inline constexpr int kMinWidth = 1100;
inline constexpr int kMaxWidth = 1760;
inline constexpr int kDesignHeight = 660;
inline constexpr int kHeaderHeight = 56;
inline constexpr int kLeftX = 24;
inline constexpr int kLeftWidth = 212;
inline constexpr int kSceneX = 244;
inline constexpr int kRightWidth = 176;
inline constexpr int kRightMargin = 20;

inline constexpr int kIdleTimerHz = 4;
inline constexpr int kActiveTimerHz = 60;

// Zone edges from the education spec: near d < 0.25, mid 0.25–0.6, far d > 0.6.
inline constexpr float kNearEdge = 0.25f;
inline constexpr float kFarEdge = 0.60f;

inline constexpr unsigned kNearColour = 0xffFF7A6Bu;
inline constexpr unsigned kMidColour = 0xffFFD27Au;
inline constexpr unsigned kFarColour = 0xff6FD3B8u;

// 0 near, 1 mid, 2 far.
inline constexpr int zoneIndex(float d) noexcept
{
    return d < kNearEdge ? 0 : (d <= kFarEdge ? 1 : 2);
}

inline constexpr unsigned zoneColour(int zone) noexcept
{
    return zone == 0 ? kNearColour : (zone == 1 ? kMidColour : kFarColour);
}

// Positive half-up rounding of width * 660 / 1280.
inline constexpr int heightForWidth(int width) noexcept
{
    return static_cast<int>((static_cast<long long>(width) * kDesignHeight + (kDefaultWidth / 2)) / kDefaultWidth);
}

static_assert(heightForWidth(kDefaultWidth) == kDesignHeight, "design frame");
static_assert(heightForWidth(kMinWidth) == 567, "minimum frame");
static_assert(heightForWidth(kMaxWidth) == 908, "maximum frame");
static_assert(zoneIndex(0.10f) == 0, "near");
static_assert(zoneIndex(0.25f) == 1, "mid includes the near edge");
static_assert(zoneIndex(0.60f) == 1, "mid includes 0.6");
static_assert(zoneIndex(0.80f) == 2, "far");

} // namespace orbit::ui
