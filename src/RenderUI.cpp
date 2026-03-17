#include "RenderUI.h"

#include <algorithm>

namespace renderui
{
void DrawPanelBackground(const Rectangle& panel)
{
    DrawRectangleRounded(panel, 0.06f, 8, Color{24, 33, 44, 235});
    DrawRectangleRoundedLinesEx(panel, 0.06f, 8, 2.0f, Color{82, 106, 125, 255});
}

void DrawMeter(const Rectangle& rect, float value, float maxValue, const char* label, Color fillColor)
{
    const float clamped = std::max(0.0f, std::min(value, maxValue));
    const float ratio = (maxValue <= 0.0f) ? 0.0f : clamped / maxValue;

    DrawRectangleRounded(rect, 0.2f, 6, Color{18, 22, 28, 255});
    Rectangle fill = rect;
    fill.width = rect.width * ratio;
    DrawRectangleRounded(fill, 0.2f, 6, fillColor);
    DrawRectangleRoundedLinesEx(rect, 0.2f, 6, 1.5f, Color{100, 116, 128, 255});

    DrawText(label, static_cast<int>(rect.x), static_cast<int>(rect.y - 20), 18, RAYWHITE);
    DrawText(TextFormat("%d / %d", static_cast<int>(clamped), static_cast<int>(maxValue)),
             static_cast<int>(rect.x + rect.width - 90), static_cast<int>(rect.y - 20), 18, RAYWHITE);
}

void DrawAbilityLine(int x, int y, int slot, const char* name, bool unlocked, float cooldownRemaining, bool selected)
{
    constexpr int kLineWidth = 320;
    constexpr int kStatusX = 246;
    Color textColor = unlocked ? RAYWHITE : Color{150, 150, 150, 255};
    if (selected && unlocked)
    {
        DrawRectangleRounded(Rectangle{static_cast<float>(x - 6), static_cast<float>(y - 4), static_cast<float>(kLineWidth), 28}, 0.2f, 4,
                             Color{51, 87, 121, 200});
    }

    int nameFontSize = 18;
    while (nameFontSize > 12 && MeasureText(name, nameFontSize) > (kStatusX - 32))
    {
        nameFontSize--;
    }

    DrawText(TextFormat("%d", slot), x, y, 18, Color{181, 198, 214, 255});
    DrawText(name, x + 24, y + ((18 - nameFontSize) / 2), nameFontSize, textColor);

    if (!unlocked)
    {
        DrawText("LOCKED", x + kStatusX, y, 16, Color{170, 120, 120, 255});
        return;
    }

    if (cooldownRemaining > 0.0f)
    {
        DrawText(TextFormat("%.1fs", cooldownRemaining), x + kStatusX, y, 16, Color{255, 194, 118, 255});
    }
    else
    {
        DrawText("READY", x + kStatusX, y, 16, Color{126, 235, 165, 255});
    }
}

void DrawCenteredMessage(const std::string& title, const std::string& subtitle, Color tint)
{
    const int titleFont = 46;
    const int subFont = 22;

    const int titleW = MeasureText(title.c_str(), titleFont);
    const int subW = MeasureText(subtitle.c_str(), subFont);

    const int cx = GetScreenWidth() / 2;
    const int cy = GetScreenHeight() / 2;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{11, 16, 24, 215});
    DrawText(title.c_str(), cx - titleW / 2, cy - 58, titleFont, tint);
    DrawText(subtitle.c_str(), cx - subW / 2, cy + 8, subFont, RAYWHITE);
}

Color TraitColor(StudentTrait trait, bool consumed)
{
    if (consumed)
    {
        return Color{92, 92, 92, 255};
    }

    switch (trait)
    {
        case StudentTrait::Helpful: return Color{98, 210, 116, 255};
        case StudentTrait::Normal: return Color{96, 166, 243, 255};
        case StudentTrait::Snitchy: return Color{236, 121, 96, 255};
        default: return LIGHTGRAY;
    }
}

const char* TraitName(StudentTrait trait)
{
    switch (trait)
    {
        case StudentTrait::Helpful: return "Helpful";
        case StudentTrait::Normal: return "Normal";
        case StudentTrait::Snitchy: return "Snitchy";
        default: return "Unknown";
    }
}

const char* TeacherStateName(TeacherState state)
{
    switch (state)
    {
        case TeacherState::Patrol: return "Patrol";
        case TeacherState::Scan: return "Scan";
        case TeacherState::Investigate: return "Investigate";
        case TeacherState::Alert: return "Alert";
        default: return "Unknown";
    }
}
} // namespace renderui
