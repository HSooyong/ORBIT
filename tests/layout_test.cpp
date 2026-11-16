// Every supported window size and mode: text stays inside the window,
// neighbouring labels do not intersect, and each string fits its box.
#include "EditorLayout.h"
#include <cstdio>
#include <string>

static int fails = 0;

static void check(bool ok, const std::string& what)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (! ok) ++fails;
}

static void checkLayout(int width, orbit::ui::Flags flags, const char* mode)
{
    const auto layout = orbit::ui::layoutFor(width, flags);
    char head[160];
    std::snprintf(head, sizeof head, "%s %dx%d", mode, layout.width, layout.height);
    bool inside = true, fit = true, apart = true;
    std::string bad;
    for (const auto& item : layout.items)
    {
        if (! item.box.inside((float) layout.width, (float) layout.height) || ! orbit::ui::textFits(item))
        {
            if (! item.box.inside((float) layout.width, (float) layout.height)) inside = false;
            if (! orbit::ui::textFits(item)) fit = false;
            bad += item.name;
            bad += " ";
        }
    }
    for (size_t i = 0; i < layout.items.size() && apart; ++i)
        for (size_t j = i + 1; j < layout.items.size(); ++j)
            if (layout.items[i].box.overlaps(layout.items[j].box))
            {
                apart = false;
                bad += layout.items[i].name;
                bad += "/";
                bad += layout.items[j].name;
                bad += " ";
            }
    bool rows = true;
    for (const auto& item : layout.advancedRows)
        if (! orbit::ui::textFits(item)) { rows = false; bad += item.name; bad += " "; }
    for (size_t i = 0; i < layout.advancedRows.size() && rows; ++i)
        for (size_t j = i + 1; j < layout.advancedRows.size(); ++j)
            if (layout.advancedRows[i].box.overlaps(layout.advancedRows[j].box))
            {
                rows = false;
                bad += "row ";
                bad += layout.advancedRows[i].name;
            }
    check(inside && fit && apart && rows && layout.scene.empty() == flags.zones, std::string(head) + (bad.empty() ? "" : "  " + bad));
}

int main()
{
    const orbit::ui::Flags modes[] = {
        {},
        { true, false, false, false, false },
        { false, true, false, false, false },
        { true, true, false, false, false },
        { false, false, true, false, false },
        { true, false, true, false, true },
        { false, true, true, false, false },
        { false, false, false, true, false },
        { false, true, false, true, true },
        { true, true, false, false, true },
    };
    const char* names[] = { "default", "learn", "meter", "learn+meter", "advanced", "learn+advanced+toast", "meter+advanced", "zones", "zones+meter+toast", "learn+meter+toast" };
    for (int width : { 1100, 1280, 1440, 1600, 1760 })
        for (int i = 0; i < 10; ++i)
            checkLayout(width, modes[i], names[i]);
    std::printf("\n%s (%d failure%s)\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
