#pragma once

#include <array>

#include "Types.h"

namespace config
{
constexpr int kScreenWidth = 960;
constexpr int kScreenHeight = 720;

constexpr int kGridW = 16;
constexpr int kGridH = 16;
constexpr int kTileSize = 40;

constexpr int kGridPixelW = kGridW * kTileSize;
constexpr int kGridPixelH = kGridH * kTileSize;

constexpr int kGridOriginX = 24;
constexpr int kGridOriginY = 40;

constexpr int kHudX = kGridOriginX + kGridPixelW + 16;
constexpr int kHudY = 20;
constexpr int kHudWidth = kScreenWidth - kHudX - 16;

constexpr float kPlayerMoveSec = 0.12f;
constexpr float kPeekHoldSec = 0.70f;
constexpr float kSubmitHoldSec = 0.80f;
constexpr float kCheatingCaughtSec = 0.20f;
constexpr float kMirrorDurationSec = 5.0f;
constexpr float kAlertDurationSec = 4.0f;

constexpr float kSuspicionMin = 0.0f;
constexpr float kSuspicionMax = 100.0f;
constexpr float kSuspicionDecayPerSec = 6.0f;
constexpr float kSuspicionSeenPerSec = 12.0f;
constexpr float kSuspicionPeekPerSec = 5.0f;

constexpr float kSuspicionThreshold35 = 35.0f;
constexpr float kSuspicionThreshold70 = 70.0f;
constexpr float kSuspicionThreshold90 = 90.0f;

constexpr float kScanIntervalMultiplierAt35 = 0.85f;
constexpr float kTeacherSpeedMultiplierAt70 = 1.20f;
constexpr float kTeacherExtraFovAt70 = 10.0f;

constexpr int kProgressTarget = 100;

constexpr float kAbilityCooldownWhisper = 12.0f;
constexpr float kAbilityCooldownFakeCough = 18.0f;
constexpr float kAbilityCooldownMirror = 20.0f;

constexpr float kInvestigateDurationFakeCough = 2.5f;

inline WeekTuning GetWeekTuning(int weekIndex)
{
    switch (weekIndex)
    {
        case 1: return WeekTuning{0.35f, 70.0f, 5, 4.0f, 1.0f, 9};
        case 2: return WeekTuning{0.28f, 82.0f, 6, 3.2f, 1.12f, 11};
        case 3: return WeekTuning{0.22f, 95.0f, 7, 2.4f, 1.25f, 13};
        default: return WeekTuning{0.22f, 95.0f, 7, 2.4f, 1.35f, 13};
    }
}

inline int PeekGain(StudentTrait trait)
{
    switch (trait)
    {
        case StudentTrait::Helpful: return 14;
        case StudentTrait::Normal: return 12;
        case StudentTrait::Snitchy: return 8;
        default: return 10;
    }
}

inline int WhisperGain(StudentTrait trait)
{
    switch (trait)
    {
        case StudentTrait::Helpful: return 22;
        case StudentTrait::Normal: return 18;
        case StudentTrait::Snitchy: return 8;
        default: return 12;
    }
}

inline float WhisperSuspicion(StudentTrait trait)
{
    switch (trait)
    {
        case StudentTrait::Helpful: return 8.0f;
        case StudentTrait::Normal: return 12.0f;
        case StudentTrait::Snitchy: return 18.0f;
        default: return 12.0f;
    }
}

inline std::array<float, 3> TraitWeights(int weekIndex)
{
    switch (weekIndex)
    {
        case 1: return {0.35f, 0.50f, 0.15f};
        case 2: return {0.30f, 0.45f, 0.25f};
        case 3: return {0.25f, 0.40f, 0.35f};
        default: return {0.20f, 0.40f, 0.40f};
    }
}

inline float AbilityCooldown(AbilityType ability)
{
    switch (ability)
    {
        case AbilityType::Whisper: return kAbilityCooldownWhisper;
        case AbilityType::FakeCough: return kAbilityCooldownFakeCough;
        case AbilityType::Mirror: return kAbilityCooldownMirror;
        default: return 0.0f;
    }
}
} // namespace config
