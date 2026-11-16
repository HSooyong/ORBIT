#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace orbit::ui
{

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
inline constexpr int kMeterDrawer = 186;
inline constexpr int kAdvancedWidth = 500;
inline constexpr float kNearEdge = 0.25f;
inline constexpr float kFarEdge = 0.60f;
inline constexpr unsigned kNearColour = 0xffFF7A6Bu;
inline constexpr unsigned kMidColour = 0xffFFD27Au;
inline constexpr unsigned kFarColour = 0xff6FD3B8u;

inline constexpr int zoneIndex(float d) noexcept
{
    return d < kNearEdge ? 0 : (d <= kFarEdge ? 1 : 2);
}

inline constexpr unsigned zoneColour(int zone) noexcept
{
    return zone == 0 ? kNearColour : (zone == 1 ? kMidColour : kFarColour);
}

inline constexpr int heightForWidth(int width) noexcept
{
    return static_cast<int>((static_cast<long long>(width) * kDesignHeight + (kDefaultWidth / 2)) / kDefaultWidth);
}

struct R
{
    float x = 0, y = 0, w = 0, h = 0;
    bool empty() const noexcept { return w <= 1.0f || h <= 1.0f; }
    bool overlaps(R o) const noexcept
    {
        return x + 1.0f < o.x + o.w && o.x + 1.0f < x + w && y + 1.0f < o.y + o.h && o.y + 1.0f < y + h;
    }
    bool inside(float W, float H) const noexcept
    {
        return x >= -0.5f && y >= -0.5f && x + w <= W + 0.5f && y + h <= H + 0.5f;
    }
};

struct Flags
{
    bool learn = false, meter = false, advanced = false, zones = false, toast = false;
};

struct Item
{
    R box;
    const char* text = nullptr;
    float px = 0;
    bool wrap = false;
    const char* name = "";
};

struct Layout
{
    int width = 0, height = 0;
    Flags flags;
    R scene, drawer, advancedView, toast;
    R knob[3] {};
    R zoneTile[3] {};
    R depthCurve, orbitCurve, flow, conceptCard;
    std::vector<Item> items;
    std::vector<Item> advancedRows;
};

inline int utf8Next(const char*& s)
{
    const auto* u = reinterpret_cast<const unsigned char*>(s);
    if (*u == 0) return 0;
    if (*u < 0x80) { s += 1; return *u; }
    if ((*u & 0xE0) == 0xC0) { s += 2; return 0x80; }
    if ((*u & 0xF0) == 0xE0) { s += 3; return 0x800; }
    s += 4;
    return 0x10000;
}

inline float glyphWidth(int cp, float px) noexcept
{
    return cp < 0x250 ? px * 0.62f : px * 1.02f;
}

inline bool textFits(const Item& item)
{
    if (item.text == nullptr) return true;
    if (item.px < 1.0f || item.box.w < 4.0f || item.box.h < item.px) return false;
    const char* s = item.text;
    float line = 0;
    int lines = 1;
    const float unit = item.box.w / item.px;
    while (const int cp = utf8Next(s))
    {
        if (cp == '\n') { ++lines; line = 0; continue; }
        const float gw = glyphWidth(cp, 1.0f);
        if (item.wrap && line > 0.0f && line + gw > unit)
        {
            ++lines;
            line = gw;
        }
        else line += gw;
    }
    if (! item.wrap && line > unit + 0.02f) return false;
    return lines * 1.15f <= (item.box.h / item.px) + 0.08f;
}

inline Layout layoutFor(int width, Flags flags)
{
    Layout L;
    L.width = width;
    L.height = heightForWidth(width);
    L.flags = flags;
    const float sx = (float) width / (float) kDefaultWidth;
    const float sy = (float) L.height / (float) kDesignHeight;
    auto box = [&](float x, float y, float w, float h) -> R { return { x * sx, y * sy, w * sx, h * sy }; };
    auto add = [&](float x, float y, float w, float h, const char* text, float px, bool wrap, const char* name)
    {
        L.items.push_back({ box(x, y, w, h), text, px * sy, wrap, name });
    };
    const float contentBottom = flags.meter ? (float) kDesignHeight - (float) kMeterDrawer : (float) kDesignHeight;
    float columnBottom = contentBottom - 12.0f;
    if (flags.toast) columnBottom -= 58.0f;

    add(22, 8, 150, 22, "ORBIT", 18, false, "brand");
    add(22, 30, 250, 14, "GRAVITY WARP  ·  BEARFIELD", 10, false, "brand sub");

    float right = 1268.0f;
    auto take = [&](float w, float gap = 6.0f)
    {
        right -= w;
        R r { right, 14, w, 28 };
        right -= gap;
        return r;
    };
    const R menu = take(28);
    const R advanced = take(96);
    const R meter = take(78);
    const R learn = take(92);
    const R zones = take(112);
    const R undo = take(28);
    const R pillB = take(26, 2);
    const R pillA = take(26);
    add(menu.x, menu.y, menu.w, menu.h, "...", 14, false, "menu");
    add(advanced.x, advanced.y, advanced.w, advanced.h, "고급 ADVANCED", 11, false, "advanced");
    add(meter.x, meter.y, meter.w, meter.h, "미터 METER", 11, false, "meter");
    add(learn.x, learn.y, learn.w, learn.h, "배우기 LEARN", 11, false, "learn");
    add(zones.x, zones.y, zones.w, zones.h, "구역 비교 ZONES", 11, false, "zones");
    add(undo.x, undo.y, undo.w, undo.h, "U", 12, false, "undo");
    add(pillB.x, pillB.y, pillB.w, pillB.h, "B", 12, false, "B");
    add(pillA.x, pillA.y, pillA.w, pillA.h, "A", 12, false, "A");

    const float sceneLeft = 280.0f;
    const float sceneRight = right - 8.0f;
    const float cluster = 200.0f;
    const float cx = sceneLeft + std::max(0.0f, (sceneRight - sceneLeft - cluster) * 0.5f);
    add(cx, 14, 28, 28, "<", 14, false, "scene prev");
    add(cx + cluster - 28, 14, 28, 28, ">", 14, false, "scene next");
    add(cx + 32, 6, cluster - 64, 12, "장면", 9, false, "scene caption");
    add(cx + 32, 18, cluster - 64, 22, "Stepping Stones", 12, false, "scene name");

    if (flags.meter)
    {
        add(560, contentBottom + 4, 160, 16, "미터 닫기", 10, false, "meter close");
        const float panelsY = contentBottom + 24;
        const float panelsH = (float) kMeterDrawer - 36;
        add(16, panelsY, 460, 16, "피치 편차 기록  PITCH TRACE", 11, false, "trace title");
        add(484, panelsY, 250, 16, "흔들림 구성  COMPONENTS", 11, false, "comp title");
        add(742, panelsY, 250, 16, "레벨  LEVEL", 11, false, "level title");
        add(1000, panelsY, 260, 16, "기록  HISTORY", 11, false, "history title");
        add(484, panelsY + 36, 140, 14, "와우  WOW", 11, false, "wow row");
        add(630, panelsY + 36, 90, 14, "±100.0", 11, false, "wow value");
        add(484, panelsY + 58, 140, 14, "플러터  FLUTTER", 11, false, "flutter row");
        add(630, panelsY + 58, 90, 14, "±100.0", 11, false, "flutter value");
        add(484, panelsY + 80, 140, 14, "코러스  CHORUS", 11, false, "chorus row");
        add(630, panelsY + 80, 90, 14, "±100.0", 11, false, "chorus value");
        add(742, panelsY + 36, 120, 14, "입력  IN", 11, false, "in label");
        add(870, panelsY + 36, 100, 14, "-00.0 dB", 11, false, "in value");
        add(742, panelsY + 58, 120, 14, "출력  OUT", 11, false, "out label");
        add(870, panelsY + 58, 100, 14, "-00.0 dB", 11, false, "out value");
        add(742, panelsY + 80, 150, 14, "자동 보정  MATCH", 11, false, "match label");
        add(900, panelsY + 80, 80, 14, "+0.0 dB", 11, false, "match value");
        add(1000, panelsY + 36, 250, 14, "중력 50 → 100 %", 11, false, "history row");
        L.drawer = box(12, contentBottom, 1256, (float) kMeterDrawer - 8);
        (void) panelsH;
    }
    else
        add(560, 636, 160, 16, "미터", 10, false, "meter handle");

    if (flags.toast)
        add(24, contentBottom - 56, 340, 44, "행성을 블랙홀 쪽으로 옮겼습니다\n거리 0.45 → 0.30 · 흔들림 ±100.0 → ±100.0 센트", 12, true, "toast");

    if (flags.zones)
    {
        const float top = 70.0f;
        const float curveH = 118.0f;
        const float tileBottom = columnBottom - curveH - 8.0f;
        const float tileW = (1232.0f - 16.0f) / 3.0f;
        const char* names[] = { "원거리  CLEAR SPACE", "중거리  DRIFT", "근접  EVENT HORIZON" };
        const char* lines[] = {
            "옅은 코러스만 남는 구간",
            "느린 와우가 커지는 구간",
            "깊은 와우에 플러터가 겹칩니다" };
        for (int i = 0; i < 3; ++i)
        {
            const float x = 24.0f + (float) i * (tileW + 8.0f);
            L.zoneTile[i] = box(x, top, tileW, tileBottom - top);
            add(x + 10, top + 8, tileW - 20, 18, names[i], 12, false, "zone chip");
            add(x + 10, top + 30, tileW - 20, 36, "±100.0", 28, false, "zone number");
            add(x + 10, tileBottom - 78, tileW - 20, 16, lines[i], 11, false, "zone line");
            add(x + 10, tileBottom - 58, tileW - 20, 14, "와우 ±100.0 ct · 0.00 Hz", 11, false, "zone wow");
            add(x + 10, tileBottom - 42, tileW - 20, 14, "플러터 ±100.0 ct · 00.0 Hz", 11, false, "zone flutter");
            add(x + 10, tileBottom - 26, tileW - 20, 14, "코러스 ±100.0 ct · 보이스 4", 11, false, "zone chorus");
        }
        add(24, tileBottom + 8, 400, 16, "거리에 따른 흔들림 깊이  DISTANCE", 12, false, "zone curve title");
        L.depthCurve = box(24, tileBottom + 26, 1232, curveH - 30);
    }
    else
    {
        add(36, 76, 180, 18, "피치 흔들림", 13, false, "pitch ko");
        add(36, 96, 180, 14, "PITCH WOBBLE", 9, false, "pitch en");
        const float numberPx = flags.learn ? 32.0f : 42.0f;
        const float numberY = flags.learn ? 114.0f : 116.0f;
        add(36, numberY, 176, numberPx + 6, "±100.0", numberPx, false, "big number");
        add(36, numberY + numberPx + 10, 80, 16, "센트", 12, false, "cent unit");
        const float chipY = numberY + numberPx + 30;
        add(36, chipY, 176, 20, "중거리   DRIFT", 12, false, "zone chip");
        add(36, chipY + 28, 110, 14, "블랙홀까지 거리", 11, false, "distance label");
        add(150, chipY + 28, 62, 14, "0.00", 12, false, "distance value");
        add(36, chipY + 64, 70, 12, "가까움", 10, false, "near end");
        add(142, chipY + 64, 70, 12, "멀", 10, false, "far end");

        float y = chipY + 84;
        if (flags.learn)
        {
            add(36, y, 176, 14, "어떻게 흔들리나", 12, false, "flow title");
            add(36, y + 16, 176, 12, "HOW IT WOBBLES", 9, false, "flow en");
            const char* flow[] = { "궤도 각도  000°", "거리  0.00", "중력 끌림  100 %", "지연  00.0 ms", "피치  ±100.0 ct" };
            for (int i = 0; i < 5; ++i) add(36, y + 32 + i * 16.0f, 176, 14, flow[i], 11, false, "flow row");
            y += 32 + 5 * 16 + 8;
            L.flow = box(28, y - (32 + 5 * 16.0f + 8) + 32, 188, 5 * 16);
            const float room = columnBottom - 8.0f - y;
            if (room >= 72.0f)
            {
                add(36, y, 176, 70, "소리를 아주 짧게 늦춘 뒤, 그 늦추는 시간을 주기적으로 늘였다 줄이면 음이 살짝 올라갔다 내려갑니다. 이것이 모듈레이션입니다.", 11, true, "concept");
                L.conceptCard = box(28, y, 188, 70);
            }
        }

        const float knobX = flags.advanced ? -1000.0f : 1084.0f;
        if (! flags.advanced)
        {
            const float knobTop = 74.0f;
            const float knobH = ((contentBottom - 18.0f - knobTop) - 12.0f) / 3.0f;
            const char* ko[] = { "중력", "속도", "믹스" };
            const char* en[] = { "GRAVITY", "RATE", "MIX" };
            const char* val[] = { "100 %", "10.0 Hz", "100 %" };
            const char* hint[] = { "블랙홀의 질량", "한 바퀴 도는 빠르기", "원음과 흔들린 소리의 비율" };
            for (int i = 0; i < 3; ++i)
            {
                const float yk = knobTop + (float) i * (knobH + 6.0f);
                L.knob[i] = box(knobX, yk, 176, knobH);
                add(knobX + 8, yk, 160, 16, ko[i], 13, false, "knob ko");
                add(knobX + 8, yk + 16, 160, 12, en[i], 9, false, "knob en");
                add(knobX + 28, yk + knobH - (flags.meter ? 36 : 52), 120, 16, val[i], 12, false, "knob value");
                if (! flags.meter)
                    add(knobX + 8, yk + knobH - 32, 160, 28, hint[i], 11, true, "knob hint");
            }
        }

        float sceneR = flags.advanced ? 1280.0f - (float) kAdvancedWidth - 12.0f : 1072.0f;
        float sceneL = 250.0f;
        float sceneT = 68.0f;
        float sceneB = contentBottom - 16.0f;
        if (flags.learn && ! flags.advanced)
        {
            sceneR = 860.0f;
            sceneB = contentBottom - 168.0f;
            add(sceneL, sceneB + 8, 280, 16, "거리에 따른 흔들림 깊이", 12, false, "depth title");
            L.depthCurve = box(sceneL, sceneB + 26, sceneR - sceneL, 130);
            add(876, 76, 190, 16, "한 바퀴 동안 깊이", 12, false, "orbit title");
            add(876, 92, 190, 12, "ONE ORBIT", 9, false, "orbit en");
            L.orbitCurve = box(876, 108, 190, 150);
            add(876, 268, 190, 16, "지금 성분", 12, false, "parts title");
            add(876, 284, 190, 12, "COMPONENTS", 9, false, "parts en");
            add(876, 302, 190, 14, "와우 깊이  ±100.0 ct", 11, false, "part wow");
            add(876, 318, 190, 14, "와우 속도  00.00 Hz", 11, false, "part wow hz");
            add(876, 334, 190, 14, "플러터 깊이  ±100.0 ct", 11, false, "part fl");
            add(876, 350, 190, 14, "플러터 속도  00.0 Hz", 11, false, "part fl hz");
            add(876, 366, 190, 14, "코러스 깊이  ±100.0 ct", 11, false, "part ch");
            add(876, 382, 190, 14, "보이스  4", 11, false, "part voices");
            const float callY = 96.0f;
            add(sceneL + 8, callY, 150, 16, "1  행성 = 소리", 11, false, "callout 1");
            add(sceneL + 170, callY, 120, 16, "2  블랙홀", 11, false, "callout 2");
            add(sceneL + 300, callY, 140, 16, "3  구역 경계", 11, false, "callout 3");
            add(sceneL + 8, callY + 22, 230, 16, "4  꼬리 = 끌림 · 떨림", 11, false, "callout 4");
            add(sceneL + 250, callY + 22, 160, 16, "5  위성 = 보이스", 11, false, "callout 5");
            add(sceneL + 420, callY + 22, 140, 16, "6  궤도 색", 11, false, "callout 6");
        }
        if (flags.meter)
            add(sceneL + 8, sceneT + 4, 420, 16, "거리 0.00 · 끌림 100% · ±100.0 ct · 와우 00.00 Hz", 11, false, "meter line");
        L.scene = box(sceneL, sceneT + (flags.meter ? 22.0f : 0.0f) + (flags.learn && ! flags.advanced ? 46.0f : 0.0f),
                      sceneR - sceneL,
                      sceneB - sceneT - (flags.meter ? 22.0f : 0.0f) - (flags.learn && ! flags.advanced ? 46.0f : 0.0f));

        if (flags.advanced)
        {
            const float ax = 1280.0f - (float) kAdvancedWidth;
            L.advancedView = box(ax, 112, (float) kAdvancedWidth - 12, contentBottom - 124);
            add(ax + 12, 68, 200, 18, "고급 설정", 14, false, "advanced title");
            add(ax + 12, 88, 220, 12, "ADVANCED", 9, false, "advanced en");
            const char* rows[][2] = {
                { "중력", "gravity" }, { "속도", "rate" }, { "믹스", "mix" },
                { "박자 동기", "sync" }, { "파형", "shape" }, { "계단", "stages" },
                { "타원율", "ecc" }, { "케플러", "kepler" }, { "멈춤", "hold" },
                { "영향권", "horizon" }, { "지평선 효과", "bloom" }, { "블랙홀 X", "bh_x" },
                { "블랙홀 Y", "bh_y" }, { "궤도 위치", "phase" }, { "궤도 크기", "radius" },
                { "와우", "warp" }, { "플러터", "flutter" }, { "불안정", "chaos" },
                { "코러스", "floor" }, { "보이스", "voices" }, { "넓이", "width" },
                { "피드백", "feedback" }, { "톤", "tone" }, { "테이프", "drive" }, { "출력", "output" } };
            const float rowW = (float) kAdvancedWidth - 36;
            for (int i = 0; i < 25; ++i)
            {
                const float rowY = (float) i * 20.0f;
                Item ko { { 0, rowY, 110, 18 }, rows[i][0], 12, false, "adv ko" };
                Item en { { 114, rowY, 78, 18 }, rows[i][1], 10, false, "adv en" };
                Item value { { rowW - 72, rowY, 72, 18 }, "±100.0", 11, false, "adv value" };
                L.advancedRows.push_back(ko);
                L.advancedRows.push_back(en);
                L.advancedRows.push_back(value);
            }
        }
    }

    return L;
}

inline constexpr int kDefaultWidthCheck = kDefaultWidth;
static_assert(heightForWidth(kDefaultWidth) == kDesignHeight, "design frame");
static_assert(heightForWidth(kMinWidth) == 567, "minimum frame");
static_assert(heightForWidth(kMaxWidth) == 908, "maximum frame");
static_assert(zoneIndex(0.10f) == 0, "near");
static_assert(zoneIndex(0.25f) == 1, "mid edge");
static_assert(zoneIndex(0.60f) == 1, "far edge");
static_assert(zoneIndex(0.80f) == 2, "far");

} // namespace orbit::ui
