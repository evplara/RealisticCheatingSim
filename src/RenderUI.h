#pragma once

#include <string>

#include <raylib.h>

#include "Types.h"

namespace renderui
{
void DrawPanelBackground(const Rectangle& panel);
void DrawMeter(const Rectangle& rect, float value, float maxValue, const char* label, Color fillColor);
void DrawAbilityLine(int x, int y, int slot, const char* name, bool unlocked, float cooldownRemaining, bool selected);
void DrawCenteredMessage(const std::string& title, const std::string& subtitle, Color tint);
Color TraitColor(StudentTrait trait, bool consumed);
const char* TraitName(StudentTrait trait);
const char* TeacherStateName(TeacherState state);
} // namespace renderui
