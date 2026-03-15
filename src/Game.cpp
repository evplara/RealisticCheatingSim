#include "Game.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>

#include "Config.h"
#include "RenderUI.h"
#include "StealthSystems.h"

namespace
{
constexpr GridPos kMoveDirs[4] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
constexpr float kDegToRad = 3.14159265f / 180.0f;
constexpr int kAbilityCount = 3;

float ClampF(float value, float minValue, float maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

Vector2 LerpPos(GridPos from, GridPos to, float t)
{
    const float clamped = ClampF(t, 0.0f, 1.0f);
    const float fx = static_cast<float>(from.x) + static_cast<float>(to.x - from.x) * clamped;
    const float fy = static_cast<float>(from.y) + static_cast<float>(to.y - from.y) * clamped;
    return Vector2{config::kGridOriginX + fx * config::kTileSize + config::kTileSize * 0.5f,
                   config::kGridOriginY + fy * config::kTileSize + config::kTileSize * 0.5f};
}

Rectangle TileRect(GridPos pos)
{
    return Rectangle{static_cast<float>(config::kGridOriginX + pos.x * config::kTileSize),
                     static_cast<float>(config::kGridOriginY + pos.y * config::kTileSize),
                     static_cast<float>(config::kTileSize),
                     static_cast<float>(config::kTileSize)};
}

int TraitHintIndex(const WeekData& week, GridPos playerPos)
{
    int best = -1;
    int bestDist = std::numeric_limits<int>::max();
    for (int i = 0; i < static_cast<int>(week.students.size()); ++i)
    {
        const int dist = stealth::ManhattanDistance(playerPos, week.students[i].pos);
        if (dist < bestDist)
        {
            best = i;
            bestDist = dist;
        }
    }
    return best;
}

const char* UpgradeName(UpgradeType upgrade)
{
    switch (upgrade)
    {
        case UpgradeType::PeekRange: return "Long Reach";
        case UpgradeType::PeekSuspicionDown: return "Quiet Hands";
        case UpgradeType::PeekProgressUp: return "Quick Study";
        case UpgradeType::UnlockWhisper: return "Whisper";
        case UpgradeType::UnlockFakeCough: return "Fake Cough";
        case UpgradeType::UnlockMirror: return "Mirror";
        case UpgradeType::Distraction: return "Distraction";
        default: return "Upgrade";
    }
}

const char* UpgradeDescription(UpgradeType upgrade)
{
    switch (upgrade)
    {
        case UpgradeType::PeekRange: return "+1 peek range";
        case UpgradeType::PeekSuspicionDown: return "-20% suspicion a peek";
        case UpgradeType::PeekProgressUp: return "+3 progress a peek";
        case UpgradeType::UnlockWhisper: return "Unlock this ability";
        case UpgradeType::UnlockFakeCough: return "Unlock this ability";
        case UpgradeType::UnlockMirror: return "Unlock this ability";
        case UpgradeType::Distraction: return "+1 weekly distraction charge";
        default: return "";
    }
}

Color UpgradeColor(UpgradeType upgrade)
{
    switch (upgrade)
    {
        case UpgradeType::PeekRange: return Color{119, 210, 255, 255};
        case UpgradeType::PeekSuspicionDown: return Color{130, 241, 170, 255};
        case UpgradeType::PeekProgressUp: return Color{255, 208, 108, 255};
        case UpgradeType::UnlockWhisper: return Color{97, 191, 255, 255};
        case UpgradeType::UnlockFakeCough: return Color{245, 173, 108, 255};
        case UpgradeType::UnlockMirror: return Color{180, 213, 255, 255};
        case UpgradeType::Distraction: return Color{248, 141, 109, 255};
        default: return RAYWHITE;
    }
}

AbilityType AbilityByIndex(int index)
{
    switch (index)
    {
        case 0: return AbilityType::Whisper;
        case 1: return AbilityType::FakeCough;
        case 2: return AbilityType::Mirror;
        default: return AbilityType::Whisper;
    }
}

int CountUnlockedAbilities(const std::array<bool, kAbilityCount>& unlocked)
{
    return static_cast<int>(std::count(unlocked.begin(), unlocked.end(), true));
}

Rectangle MenuButtonRect(int index)
{
    constexpr float buttonWidth = 420.0f;
    constexpr float buttonHeight = 96.0f;
    constexpr float gapY = 30.0f;
    const float startY = config::kScreenHeight * 0.34f;
    return Rectangle{(config::kScreenWidth - buttonWidth) * 0.5f,
                     startY + static_cast<float>(index) * (buttonHeight + gapY),
                     buttonWidth,
                     buttonHeight};
}

Rectangle HelpBackButtonRect()
{
    return Rectangle{static_cast<float>(config::kScreenWidth - 300), static_cast<float>(config::kScreenHeight - 84), 230.0f,
                     60.0f};
}

Rectangle RunFailButtonRect(int index)
{
    constexpr float buttonWidth = 280.0f;
    constexpr float buttonHeight = 84.0f;
    constexpr float gap = 34.0f;
    const float totalWidth = buttonWidth * 2.0f + gap;
    const float startX = (config::kScreenWidth - totalWidth) * 0.5f;
    const float y = config::kScreenHeight * 0.64f;
    return Rectangle{startX + static_cast<float>(index) * (buttonWidth + gap), y, buttonWidth, buttonHeight};
}

Color ButtonFillColor(int index)
{
    switch (index)
    {
        case 0: return Color{140, 228, 161, 255};
        case 1: return Color{121, 205, 255, 255};
        case 2: return Color{255, 173, 129, 255};
        default: return RAYWHITE;
    }
}

Color ButtonOutlineColor(int index)
{
    switch (index)
    {
        case 0: return Color{59, 126, 72, 255};
        case 1: return Color{52, 105, 160, 255};
        case 2: return Color{170, 91, 54, 255};
        default: return DARKGRAY;
    }
}

void DrawPosterBackground()
{
    ClearBackground(Color{249, 241, 191, 255});
    const int marginX = 108;
    const int smileX = config::kScreenWidth - 118;
    const int bottomY = config::kScreenHeight - 64;
    const int rightTextX = config::kScreenWidth - 320;

    for (int y = 36; y < config::kScreenHeight; y += 34)
    {
        DrawLine(0, y, config::kScreenWidth, y, Color{191, 221, 255, 255});
    }

    DrawLine(marginX, 0, marginX, config::kScreenHeight, Color{244, 132, 132, 255});
    DrawCircle(marginX + 14, 104, 42.0f, Color{255, 215, 112, 255});
    DrawCircle(marginX, 96, 6.0f, BLACK);
    DrawCircle(marginX + 20, 96, 6.0f, BLACK);
    DrawCircleLines(marginX + 14, 104, 42.0f, Color{208, 144, 54, 255});
    DrawCircleSectorLines(
        Vector2{static_cast<float>(marginX + 14), 104.0f},
        36.0f,
        0.0f,
        180.0f,
        32,
        Color{118, 82, 52, 255});
    DrawCircle(smileX, 126, 52.0f, Color{255, 242, 175, 255});
    DrawCircle(smileX - 16, 118, 6.0f, BLACK);
    DrawCircle(smileX + 16, 118, 6.0f, BLACK);
    DrawCircleSectorLines(
        Vector2{static_cast<float>(smileX), 144.0f},
        18.0f,
        0.0f,
        180.0f,
        32,
        Color{118, 82, 52, 255});

    DrawCircle(156, config::kScreenHeight - 92, 32.0f, Color{152, 225, 255, 255});
    DrawCircleLines(156, config::kScreenHeight - 92, 32.0f, Color{62, 127, 172, 255});
    DrawText("WOW!", 128, config::kScreenHeight - 104, 22, Color{62, 127, 172, 255});

    DrawText("A+ vibes only", rightTextX, 28, 28, Color{89, 144, 104, 255});
    DrawText("Stay curious!", marginX + 14, bottomY, 26, Color{89, 144, 104, 255});
    DrawText("School is Cool!", rightTextX - 10, bottomY, 26, Color{89, 144, 104, 255});

}

void DrawPosterButton(const Rectangle& rect, const char* label, const char* subtitle, Color fill, Color outline, bool hovered)
{
    const Rectangle shadow{rect.x + 8.0f, rect.y + 8.0f, rect.width, rect.height};
    const Color shadowColor = hovered ? Color{104, 84, 62, 150} : Color{104, 84, 62, 110};
    const Color panelColor = hovered ? Color{255, 255, 255, 255} : fill;

    DrawRectangleRounded(shadow, 0.24f, 8, shadowColor);
    DrawRectangleRounded(rect, 0.24f, 8, panelColor);
    DrawRectangleRoundedLinesEx(rect, 0.24f, 8, 4.0f, outline);

    DrawCircle(static_cast<int>(rect.x + 34.0f), static_cast<int>(rect.y + rect.height * 0.5f), 12.0f, outline);
    DrawCircle(static_cast<int>(rect.x + rect.width - 34.0f), static_cast<int>(rect.y + rect.height * 0.5f), 12.0f, outline);

    const int titleSize = 34;
    const int subtitleSize = 18;
    const int titleWidth = MeasureText(label, titleSize);
    const int subtitleWidth = MeasureText(subtitle, subtitleSize);
    DrawText(label, static_cast<int>(rect.x + rect.width * 0.5f - titleWidth * 0.5f), static_cast<int>(rect.y + 13.0f), titleSize,
             outline);
    DrawText(subtitle, static_cast<int>(rect.x + rect.width * 0.5f - subtitleWidth * 0.5f), static_cast<int>(rect.y + 49.0f),
             subtitleSize, Color{80, 76, 72, 255});
}

void DrawSectionCard(const Rectangle& rect, const char* title, Color fill, Color outline)
{
    DrawRectangleRounded(Rectangle{rect.x + 6.0f, rect.y + 6.0f, rect.width, rect.height}, 0.10f, 8, Color{104, 84, 62, 85});
    DrawRectangleRounded(rect, 0.10f, 8, fill);
    DrawRectangleRoundedLinesEx(rect, 0.10f, 8, 3.0f, outline);
    DrawText(title, static_cast<int>(rect.x + 16.0f), static_cast<int>(rect.y + 12.0f), 26, outline);
}

void DrawBullet(int x, int y, Color color, const char* text)
{
    DrawCircle(x, y + 8, 5.0f, color);
    DrawText(text, x + 16, y, 18, Color{62, 62, 62, 255});
}
} // namespace

void Game::Init()
{
    SetRandomSeed(static_cast<unsigned int>(GetTime()));

    InitAudioDevice();
    audioReady_ = IsAudioDeviceReady();
    if (audioReady_)
    {
        if (FileExists("resources/audio/peek.wav"))
        {
            sfxPeek_.sound = LoadSound("resources/audio/peek.wav");
            sfxPeek_.loaded = true;
        }
        if (FileExists("resources/audio/ability.wav"))
        {
            sfxAbility_.sound = LoadSound("resources/audio/ability.wav");
            sfxAbility_.loaded = true;
        }
        if (FileExists("resources/audio/caught.wav"))
        {
            sfxCaught_.sound = LoadSound("resources/audio/caught.wav");
            sfxCaught_.loaded = true;
        }
    }

    state_ = GameState::MainMenu;
}

void Game::Update(float dt)
{
    if (IsKeyPressed(KEY_F3))
    {
        showDebug_ = !showDebug_;
    }

    switch (state_)
    {
        case GameState::MainMenu:
            UpdateMenuInput();
            break;
        case GameState::HelpMenu:
            UpdateHelpInput();
            break;
        case GameState::WeekIntro:
            weekMessageTimer_ += dt;
            UpdateWeekIntroInput();
            break;
        case GameState::InWeek:
            UpdateInWeek(dt);
            break;
        case GameState::WeekClear:
            UpdateWeekClearInput();
            break;
        case GameState::WeekUpgrade:
            UpdateWeekUpgradeInput();
            break;
        case GameState::RunSuccess:
        case GameState::RunFail:
            UpdateEndScreenInput();
            break;
        default:
            break;
    }
}

void Game::Draw() const
{
    ClearBackground(Color{16, 22, 30, 255});

    if (state_ == GameState::MainMenu)
    {
        DrawMainMenu();
        return;
    }

    if (state_ == GameState::HelpMenu)
    {
        DrawHelpScreen();
        return;
    }

    const Rectangle boardBounds{
        static_cast<float>(config::kGridOriginX - 8),
        static_cast<float>(config::kGridOriginY - 8),
        static_cast<float>(config::kGridPixelW + 16),
        static_cast<float>(config::kGridPixelH + 16),
    };
    DrawRectangleRounded(boardBounds, 0.03f, 6, Color{30, 39, 49, 255});

    for (int y = 0; y < week_.h; ++y)
    {
        for (int x = 0; x < week_.w; ++x)
        {
            const GridPos p{x, y};
            const TileType tile = week_.tiles[stealth::ToIndex(week_, p)];
            Color c = Color{54, 66, 78, 255};
            switch (tile)
            {
                case TileType::Floor: c = Color{54, 66, 78, 255}; break;
                case TileType::Desk: c = Color{120, 95, 71, 255}; break;
                case TileType::SpawnDesk: c = Color{106, 133, 82, 255}; break;
                case TileType::TeacherDesk: c = Color{96, 74, 56, 255}; break;
                case TileType::Submit: c = Color{63, 110, 164, 255}; break;
                case TileType::Wall: c = Color{35, 43, 52, 255}; break;
                default: break;
            }
            DrawRectangleRec(TileRect(p), c);
            DrawRectangleLinesEx(TileRect(p), 1.0f, Color{26, 34, 41, 200});
        }
    }

    // Teacher FOV cone.
    Vector2 teacherPos = LerpPos(teacherMoveFrom_, teacherMoveTo_, teacherMoveT_);
    const float teacherFov = EffectiveTeacherFovDeg();
    const float teacherRangePx = static_cast<float>(tuning_.fovRangeTiles * config::kTileSize);
    DrawCircleSector(teacherPos, teacherRangePx, teacher_.facingDeg - (teacherFov * 0.5f),
                     teacher_.facingDeg + (teacherFov * 0.5f), 28, Color{255, 238, 143, 40});

    for (const Student& s : week_.students)
    {
        const Vector2 sc = Vector2{
            static_cast<float>(config::kGridOriginX + s.pos.x * config::kTileSize + config::kTileSize / 2),
            static_cast<float>(config::kGridOriginY + s.pos.y * config::kTileSize + config::kTileSize / 2)};
        DrawCircleV(sc, 11.0f, renderui::TraitColor(s.trait, s.consumed));
    }

    Vector2 playerPos = LerpPos(playerMoveFrom_, playerMoveTo_, playerMoveT_);
    DrawCircleV(playerPos, 12.0f, Color{122, 221, 244, 255});
    if (player_.mirrorActive)
    {
        DrawCircleLines(static_cast<int>(playerPos.x), static_cast<int>(playerPos.y), 16.0f, Color{185, 235, 255, 255});
    }

    DrawCircleV(teacherPos, 13.0f, Color{249, 92, 106, 255});
    const float theta = teacher_.facingDeg * kDegToRad;
    const Vector2 dir{std::cos(theta) * 16.0f, std::sin(theta) * 16.0f};
    DrawLineEx(teacherPos, Vector2{teacherPos.x + dir.x, teacherPos.y + dir.y}, 3.0f, Color{250, 220, 220, 255});

    DrawText("HOME", config::kGridOriginX + week_.playerSpawn.x * config::kTileSize + 7,
             config::kGridOriginY + week_.playerSpawn.y * config::kTileSize + 14, 12, Color{214, 236, 182, 255});
    DrawText("SUBMIT", config::kGridOriginX + week_.submitPos.x * config::kTileSize + 4,
             config::kGridOriginY + week_.submitPos.y * config::kTileSize + 10, 12, Color{219, 236, 255, 255});

    const Rectangle panel{static_cast<float>(config::kHudX), static_cast<float>(config::kHudY),
                          static_cast<float>(config::kHudWidth), static_cast<float>(config::kScreenHeight - 2 * config::kHudY)};
    renderui::DrawPanelBackground(panel);

    int y = config::kHudY + 16;
    DrawText(TextFormat("Week %d / %d", weekIndex_, config::kMaxWeeks), config::kHudX + 14, y, 24, RAYWHITE);
    y += 30;
    DrawText(TextFormat("State: %s", renderui::TeacherStateName(teacher_.state)), config::kHudX + 14, y, 18,
             Color{191, 212, 227, 255});
    y += 26;
    DrawText(TextFormat("Seed: %u", week_.seed), config::kHudX + 14, y, 16, Color{156, 178, 194, 255});
    y += 36;

    renderui::DrawMeter(Rectangle{static_cast<float>(config::kHudX + 14), static_cast<float>(y),
                                  static_cast<float>(config::kHudWidth - 28), 20.0f},
                        quizProgress_, static_cast<float>(config::kProgressTarget), "Quiz Progress",
                        Color{126, 207, 147, 255});
    y += 54;

    renderui::DrawMeter(Rectangle{static_cast<float>(config::kHudX + 14), static_cast<float>(y),
                                  static_cast<float>(config::kHudWidth - 28), 20.0f},
                        suspicion_, 100.0f, "Suspicion", Color{236, 119, 101, 255});
    y += 54;

    DrawText("Abilities (press 1/2/3)", config::kHudX + 14, y, 18, RAYWHITE);
    y += 26;
    renderui::DrawAbilityLine(config::kHudX + 14, y, 1, "Whisper", IsAbilityUnlocked(AbilityType::Whisper),
                              abilityCooldowns_[AbilityIndex(AbilityType::Whisper)],
                              selectedAbility_ == AbilityType::Whisper);
    y += 28;
    renderui::DrawAbilityLine(config::kHudX + 14, y, 2, "Fake Cough", IsAbilityUnlocked(AbilityType::FakeCough),
                              abilityCooldowns_[AbilityIndex(AbilityType::FakeCough)],
                              selectedAbility_ == AbilityType::FakeCough);
    y += 28;
    renderui::DrawAbilityLine(config::kHudX + 14, y, 3, "Mirror", IsAbilityUnlocked(AbilityType::Mirror),
                              abilityCooldowns_[AbilityIndex(AbilityType::Mirror)],
                              selectedAbility_ == AbilityType::Mirror);
    y += 34;

    if (CountUnlockedAbilities(unlockedAbilities_) == 0)
    {
        DrawText("Draft an ability from weekly rewards", config::kHudX + 14, y, 16, Color{170, 190, 207, 255});
        y += 24;
    }

    DrawText(TextFormat("4  Distraction %s", HasDistractionCharge() ? "(ready)" : "(none)"), config::kHudX + 14, y, 18,
             HasDistractionCharge() ? Color{249, 189, 133, 255} : Color{134, 140, 147, 255});
    y += 26;
    DrawText(TextFormat("Charges this week: %d", distractionCharges_), config::kHudX + 38, y, 16, Color{191, 212, 227, 255});
    y += 30;

    if (player_.mirrorActive)
    {
        DrawText(TextFormat("Mirror Active: %.1fs", player_.mirrorTimer), config::kHudX + 14, y, 18,
                 Color{180, 225, 253, 255});
    }
    else
    {
        DrawText("Mirror Active: No", config::kHudX + 14, y, 18, Color{150, 170, 186, 255});
    }
    y += 30;

    DrawText(TextFormat("Peek Range: %d", EffectivePeekRange()), config::kHudX + 14, y, 18, Color{174, 224, 255, 255});
    y += 24;
    DrawText(TextFormat("Peek Suspicion x%.2f", PeekSuspicionMultiplier()), config::kHudX + 14, y, 18,
             Color{145, 232, 176, 255});
    y += 24;
    DrawText(TextFormat("Peek Progress Bonus: +%d", PeekProgressBonus()), config::kHudX + 14, y, 18,
             Color{255, 209, 119, 255});
    y += 30;

    const int hintIdx = TraitHintIndex(week_, player_.pos);
    if (hintIdx >= 0)
    {
        const Student& s = week_.students[hintIdx];
        DrawText(TextFormat("Nearest: %s", renderui::TraitName(s.trait)), config::kHudX + 14, y, 18,
                 renderui::TraitColor(s.trait, s.consumed));
    }
    y += 30;
    DrawText("WASD move/hold, Hold SPACE peek/submit", config::kHudX + 14, y, 16, Color{165, 189, 207, 255});
    y += 22;
    DrawText("Cheating in FOV for 0.2s = run over", config::kHudX + 14, y, 16, Color{220, 158, 130, 255});
    y += 22;
    DrawText("Seen from 1 tile away = instant loss", config::kHudX + 14, y, 16, Color{244, 132, 132, 255});

    if (showDebug_)
    {
        DrawRectangle(8, 8, 340, 108, Color{7, 10, 15, 215});
        DrawText(TextFormat("Debug: seesPlayer=%s", teacherSeesPlayer_ ? "true" : "false"), 14, 16, 16, RAYWHITE);
        DrawText(TextFormat("Cheating=%s seenTimer=%.2f", IsCheatingActionActive() ? "true" : "false", cheatingSeenTimer_),
                 14, 36, 16, RAYWHITE);
        DrawText(TextFormat("SuspRate=%.2f FOV=%.1f moveSec=%.2f", suspicionRate_, EffectiveTeacherFovDeg(),
                            EffectiveTeacherMoveSec()),
                 14, 56, 16, RAYWHITE);
        DrawText(TextFormat("Pos P(%d,%d) T(%d,%d)", player_.pos.x, player_.pos.y, teacher_.pos.x, teacher_.pos.y), 14, 76,
                 16, RAYWHITE);

        if (teacherSeesPlayer_)
        {
            DrawLineEx(teacherPos, playerPos, 2.0f, Color{246, 120, 102, 255});
        }
    }

    if (state_ == GameState::WeekIntro)
    {
        renderui::DrawCenteredMessage(TextFormat("Week %d Quiz", weekIndex_), "Reach 100 progress and submit.", WHITE);
    }
    else if (state_ == GameState::WeekClear)
    {
        renderui::DrawCenteredMessage(TextFormat("Week %d Cleared", weekIndex_), "Press ENTER for next week.", WHITE);
    }
    else if (state_ == GameState::WeekUpgrade)
    {
        const float cardWidth = 260.0f;
        const float cardHeight = 250.0f;
        const float gap = 34.0f;
        const float cardsStartX = (config::kScreenWidth - (cardWidth * 3.0f + gap * 2.0f)) * 0.5f;
        const int titleY = 118;
        const int subtitleY = 168;
        const int unlockY = 206;
        const std::string weekClearTitle = TextFormat("Week %d Cleared", weekIndex_);
        DrawRectangle(0, 0, config::kScreenWidth, config::kScreenHeight, Color{8, 12, 18, 170});
        DrawText(weekClearTitle.c_str(), config::kScreenWidth / 2 - MeasureText(weekClearTitle.c_str(), 44) / 2, titleY, 44, RAYWHITE);
        DrawText("Pick 1 of 3 random rewards",
                 config::kScreenWidth / 2 - MeasureText("Pick 1 of 3 random rewards", 28) / 2, subtitleY, 28, Color{179, 202, 219, 255});
        DrawText("Stat boosts and ability unlocks can both appear here.",
                 config::kScreenWidth / 2 - MeasureText("Stat boosts and ability unlocks can both appear here.", 24) / 2, unlockY, 24,
                 Color{180, 235, 199, 255});

        for (int i = 0; i < 3; ++i)
        {
            const float x = cardsStartX + static_cast<float>(i) * (cardWidth + gap);
            const Rectangle card{x, 286.0f, cardWidth, cardHeight};
            const UpgradeType upgrade = offeredUpgrades_[i];
            const Color accent = UpgradeColor(upgrade);
            DrawRectangleRounded(card, 0.08f, 8, Color{24, 33, 44, 245});
            DrawRectangleRoundedLinesEx(card, 0.08f, 8, 3.0f, accent);
            DrawText(TextFormat("%d", i + 1), static_cast<int>(x + 18.0f), 310, 30, accent);
            DrawText(UpgradeName(upgrade), static_cast<int>(x + 18.0f), 360, 30, RAYWHITE);
            DrawText(UpgradeDescription(upgrade), static_cast<int>(x + 18.0f), 414, 22, accent);
        }

        DrawText("Press 1, 2, or 3 to continue to the next week",
                 config::kScreenWidth / 2 - MeasureText("Press 1, 2, or 3 to continue to the next week", 24) / 2, 592, 24, RAYWHITE);
    }
    else if (state_ == GameState::RunFail)
    {
        DrawRunFailScreen();
    }
    else if (state_ == GameState::RunSuccess)
    {
        renderui::DrawCenteredMessage("Run Complete",
                                      TextFormat("Score: %d  |  ENTER: Main Menu", ComputeScore()),
                                      Color{141, 255, 172, 255});
    }
}

bool Game::ShouldExit() const
{
    return shouldExit_;
}

void Game::DrawMainMenu() const
{
    DrawPosterBackground();

    const Rectangle poster{(config::kScreenWidth - 760.0f) * 0.5f, 72.0f, 760.0f, config::kScreenHeight - 180.0f};
    const int titleSize = 46;
    const int subtitleSize = 26;
    const int sectionSize = 28;
    const char* title = "Realistic Cheating Simulator";
    const char* subtitle = "Reach for the stars. Borrow the answers.";
    const char* section = "Choose your inspirational poster:";
    const int titleX = static_cast<int>(poster.x + poster.width * 0.5f - MeasureText(title, titleSize) * 0.5f);
    const int subtitleX = static_cast<int>(poster.x + poster.width * 0.5f - MeasureText(subtitle, subtitleSize) * 0.5f);
    const int sectionX = static_cast<int>(poster.x + poster.width * 0.5f - MeasureText(section, sectionSize) * 0.5f);

    DrawRectangleRounded(Rectangle{poster.x + 10.0f, poster.y + 12.0f, poster.width, poster.height}, 0.04f, 8,
                         Color{104, 84, 62, 95});
    DrawRectangleRounded(poster, 0.04f, 8, Color{255, 252, 239, 255});
    DrawRectangleRoundedLinesEx(poster, 0.04f, 8, 4.0f, Color{111, 139, 189, 255});

    DrawText(title, titleX + 6, static_cast<int>(poster.y + 28.0f), titleSize, Color{255, 255, 255, 255});
    DrawText(title, titleX, static_cast<int>(poster.y + 20.0f), titleSize, Color{212, 88, 88, 255});
    DrawText(subtitle, subtitleX, static_cast<int>(poster.y + 86.0f), subtitleSize, Color{94, 129, 95, 255});
    DrawText(section, sectionX, static_cast<int>(poster.y + 144.0f), sectionSize, Color{63, 108, 162, 255});

    const Vector2 mouse = GetMousePosition();
    DrawPosterButton(MenuButtonRect(0), "START", "Begin your academic adventure", ButtonFillColor(0), ButtonOutlineColor(0),
                     CheckCollisionPointRec(mouse, MenuButtonRect(0)));
    DrawPosterButton(MenuButtonRect(1), "HELP", "Learn the sneaky school rules", ButtonFillColor(1), ButtonOutlineColor(1),
                     CheckCollisionPointRec(mouse, MenuButtonRect(1)));
    DrawPosterButton(MenuButtonRect(2), "EXIT", "Retreat with dignity and snacks", ButtonFillColor(2), ButtonOutlineColor(2),
                     CheckCollisionPointRec(mouse, MenuButtonRect(2)));

}

void Game::DrawHelpScreen() const
{
    DrawPosterBackground();

    const char* title = "How To Be A Totally Normal Student";
    const int titleX = config::kScreenWidth / 2 - MeasureText(title, 42) / 2;
    const float leftX = 92.0f;
    const float gapX = 44.0f;
    const float colWidth = (config::kScreenWidth - leftX * 2.0f - gapX);
    const float leftWidth = colWidth * 0.5f;
    const float rightX = leftX + leftWidth + gapX;

    DrawText(title, titleX + 6, 46, 42, Color{255, 255, 255, 255});
    DrawText(title, titleX, 38, 42, Color{69, 124, 178, 255});

    const Rectangle playCard{leftX, 122.0f, leftWidth, 222.0f};
    const Rectangle progressCard{rightX, 122.0f, leftWidth, 222.0f};
    const Rectangle abilityCard{leftX, 370.0f, leftWidth, 320.0f};
    const Rectangle studentCard{rightX, 370.0f, leftWidth, 154.0f};
    const Rectangle resultCard{rightX, 554.0f, leftWidth, 136.0f};
    const Rectangle resultCard2{rightX, 554.0f, leftWidth, 150.0f};

    DrawSectionCard(playCard, "How To Play", Color{255, 251, 239, 255}, Color{88, 150, 103, 255});
    DrawBullet(static_cast<int>(playCard.x + 18.0f), 166, Color{88, 150, 103, 255}, "W A S D moves; hold to keep stepping.");
    DrawBullet(static_cast<int>(playCard.x + 18.0f), 200, Color{88, 150, 103, 255}, "Hold SPACE near a student to peek answers.");
    DrawBullet(static_cast<int>(playCard.x + 18.0f), 234, Color{88, 150, 103, 255}, "Return to your HOME desk to cool off suspicion.");
    DrawBullet(static_cast<int>(playCard.x + 18.0f), 268, Color{88, 150, 103, 255}, "Stand on SUBMIT with 100 progress and hold SPACE.");
    DrawBullet(static_cast<int>(playCard.x + 18.0f), 302, Color{88, 150, 103, 255}, "Press 1, 2, or 3 to fire unlocked abilities instantly.");

    DrawSectionCard(progressCard, "How You Progress", Color{250, 249, 255, 255}, Color{72, 129, 198, 255});
    DrawBullet(static_cast<int>(progressCard.x + 18.0f), 166, Color{72, 129, 198, 255}, "Each run lasts 10 progressively harder weeks.");
    DrawBullet(static_cast<int>(progressCard.x + 18.0f), 200, Color{72, 129, 198, 255}, "Later weeks need more peeks and more students appear.");
    DrawBullet(static_cast<int>(progressCard.x + 18.0f), 234, Color{72, 129, 198, 255},
               "After each week, pick one random upgrade.");
    DrawBullet(static_cast<int>(progressCard.x + 18.0f), 268, Color{72, 129, 198, 255},
               "The pool mixes 3 stat boosts with 3 ability unlocks.");

    DrawSectionCard(abilityCard, "Abilities And Tricks", Color{255, 247, 233, 255}, Color{213, 128, 68, 255});
    DrawBullet(static_cast<int>(abilityCard.x + 18.0f), 412, Color{213, 128, 68, 255}, "Whisper: grab a nearby answer instantly while not raising suspicion.");
    DrawBullet(static_cast<int>(abilityCard.x + 18.0f), 450, Color{213, 128, 68, 255}, "Fake Cough: lure the teacher to your tile if unseen.");
    DrawBullet(static_cast<int>(abilityCard.x + 18.0f), 488, Color{213, 128, 68, 255},
               "Mirror: for 5 seconds, peek one extra tile away.");
    DrawBullet(static_cast<int>(abilityCard.x + 18.0f), 526, Color{213, 128, 68, 255},
               "Distraction: spend a charge to turn the teacher away.");
    DrawBullet(static_cast<int>(abilityCard.x + 18.0f), 564, Color{213, 128, 68, 255},
               "Abilities have cooldowns, so use them wisely.");
    DrawBullet(static_cast<int>(abilityCard.x + 18.0f), 602, Color{213, 128, 68, 255},
               "Helpful students give Whisper more progress than snitchy ones.");

    DrawSectionCard(studentCard, "Student Types", Color{241, 252, 243, 255}, Color{98, 161, 114, 255});
    DrawBullet(static_cast<int>(studentCard.x + 18.0f), 414, Color{98, 210, 116, 255}, "Helpful: best progress, lowest suspicion.");
    DrawBullet(static_cast<int>(studentCard.x + 18.0f), 452, Color{96, 166, 243, 255}, "Normal: balanced risk and reward.");
    DrawBullet(static_cast<int>(studentCard.x + 18.0f), 490, Color{236, 121, 96, 255}, "Snitches: weaker gains and much riskier to copy.");

    DrawSectionCard(resultCard2, "How To Win/Lose", Color{255, 241, 241, 255}, Color{215, 95, 95, 255});
    DrawBullet(static_cast<int>(resultCard.x + 18.0f), 598, Color{89, 170, 108, 255}, "Win: Clear all 10 weeks without getting caught.");
    DrawBullet(static_cast<int>(resultCard.x + 18.0f), 636, Color{215, 95, 95, 255}, "Lose: Cheat in the teacher's view for 0.2 seconds.");
    DrawBullet(static_cast<int>(resultCard.x + 18.0f), 674, Color{215, 95, 95, 255}, "Instantly if the teacher spots you from one tile away.");

    const Rectangle backRect = HelpBackButtonRect();
    DrawPosterButton(backRect, "BACK", "", Color{255, 237, 153, 255}, Color{164, 129, 37, 255},
                     CheckCollisionPointRec(GetMousePosition(), backRect));
}

void Game::DrawRunFailScreen() const
{
    DrawRectangle(0, 0, config::kScreenWidth, config::kScreenHeight, Color{14, 18, 26, 235});

    const Rectangle panel{(config::kScreenWidth - 860.0f) * 0.5f, 138.0f, 860.0f, 520.0f};
    DrawRectangleRounded(Rectangle{panel.x + 10.0f, panel.y + 12.0f, panel.width, panel.height}, 0.06f, 8, Color{83, 47, 37, 120});
    DrawRectangleRounded(panel, 0.06f, 8, Color{42, 29, 36, 255});
    DrawRectangleRoundedLinesEx(panel, 0.06f, 8, 4.0f, Color{224, 117, 88, 255});

    const char* title = "Caught. Run Over";
    const std::string reasonText = failReason_.empty() ? std::string{"The teacher was not fooled."} : failReason_;
    const std::string scoreText = TextFormat("Score: %d", ComputeScore());
    const char* hintText = "Restart jumps back to Week 1. Main Menu returns to the poster screen.";
    const Vector2 mouse = GetMousePosition();

    DrawText(title, config::kScreenWidth / 2 - MeasureText(title, 48) / 2, 192, 48, Color{255, 160, 132, 255});
    DrawText(reasonText.c_str(), config::kScreenWidth / 2 - MeasureText(reasonText.c_str(), 28) / 2, 284, 28, RAYWHITE);
    DrawText(scoreText.c_str(), config::kScreenWidth / 2 - MeasureText(scoreText.c_str(), 30) / 2, 332, 30, Color{255, 225, 152, 255});
    DrawText(hintText, config::kScreenWidth / 2 - MeasureText(hintText, 22) / 2, 394, 22, Color{197, 206, 216, 255});

    DrawPosterButton(RunFailButtonRect(0), "RESTART", "Jump back to Week 1", Color{146, 229, 170, 255}, Color{63, 127, 73, 255},
                     CheckCollisionPointRec(mouse, RunFailButtonRect(0)));
    DrawPosterButton(RunFailButtonRect(1), "MAIN MENU", "Return to the poster wall", Color{125, 206, 255, 255},
                     Color{54, 104, 166, 255}, CheckCollisionPointRec(mouse, RunFailButtonRect(1)));

    
}

void Game::StartNewRun()
{
    generator_ = WeekGenerator{};
    runSeed_ = static_cast<uint32_t>(GetRandomValue(1000, 999999999));
    weeksCleared_ = 0;
    totalProgress_ = 0.0f;
    peakSuspicion_ = 0.0f;
    timesSeenCheating_ = 0;
    failReason_.clear();
    abilityCooldowns_ = {0.0f, 0.0f, 0.0f};
    peekRangeRanks_ = 0;
    peekSuspicionRanks_ = 0;
    peekProgressRanks_ = 0;
    distractionRanks_ = 1;
    distractionCharges_ = 1;
    unlockedAbilities_ = {false, false, false};

    StartWeek(1);
}

void Game::StartWeek(int weekIndex)
{
    weekIndex_ = weekIndex;
    tuning_ = config::GetWeekTuning(weekIndex_);
    week_ = generator_.Generate(weekIndex_, runSeed_ + static_cast<uint32_t>(weekIndex_ * 9973));

    player_.pos = week_.playerSpawn;
    player_.mirrorActive = false;
    player_.mirrorTimer = 0.0f;
    distractionCharges_ = distractionRanks_;

    if (week_.patrol.empty())
    {
        week_.patrol = {GridPos{3, 3}, GridPos{12, 3}, GridPos{12, 12}, GridPos{3, 12}, GridPos{3, 3}};
    }

    teacher_.pos = week_.patrol.front();
    teacher_.waypointIndex = (week_.patrol.size() > 1) ? 1 : 0;
    teacher_.state = TeacherState::Patrol;
    teacher_.stateTimer = 0.0f;
    teacher_.facingDeg = 90.0f;

    investigateTarget_ = teacher_.pos;
    investigateRemainingSec_ = 0.0f;

    suspicion_ = 0.0f;
    suspicionRate_ = 0.0f;
    cheatingSeenTimer_ = 0.0f;
    wasSeeingCheatingLastFrame_ = false;
    lastSuspiciousPos_ = player_.pos;

    quizProgress_ = 0.0f;
    teacherSeesPlayer_ = false;
    selectedAbility_ = AbilityType::Whisper;
    if (!IsAbilityUnlocked(selectedAbility_))
    {
        for (int i = 0; i < kAbilityCount; ++i)
        {
            const AbilityType ability = AbilityByIndex(i);
            if (IsAbilityUnlocked(ability))
            {
                selectedAbility_ = ability;
                break;
            }
        }
    }

    playerMoving_ = false;
    playerMoveFrom_ = player_.pos;
    playerMoveTo_ = player_.pos;
    playerMoveT_ = 1.0f;

    teacherMoveFrom_ = teacher_.pos;
    teacherMoveTo_ = teacher_.pos;
    teacherMoveT_ = 1.0f;
    teacherStepTimer_ = 0.0f;

    peekHoldSec_ = 0.0f;
    submitHoldSec_ = 0.0f;
    spaceActionTriggered_ = false;
    peeking_ = false;
    cheatingActionTimer_ = 0.0f;

    weekMessageTimer_ = 0.0f;
    state_ = GameState::WeekIntro;
}

void Game::AdvanceWeekOrEndRun()
{
    weeksCleared_++;
    if (weekIndex_ >= config::kMaxWeeks)
    {
        state_ = GameState::RunSuccess;
        return;
    }
    RollUpgradeChoices();
    state_ = GameState::WeekUpgrade;
}

void Game::RollUpgradeChoices()
{
    std::vector<UpgradeType> pool{
        UpgradeType::PeekRange,
        UpgradeType::PeekSuspicionDown,
        UpgradeType::PeekProgressUp,
    };
    if (!IsAbilityUnlocked(AbilityType::Whisper))
    {
        pool.push_back(UpgradeType::UnlockWhisper);
    }
    if (!IsAbilityUnlocked(AbilityType::FakeCough))
    {
        pool.push_back(UpgradeType::UnlockFakeCough);
    }
    if (!IsAbilityUnlocked(AbilityType::Mirror))
    {
        pool.push_back(UpgradeType::UnlockMirror);
    }

    std::mt19937 rng(runSeed_ + static_cast<uint32_t>(weekIndex_ * 7919 + weeksCleared_ * 313));
    std::shuffle(pool.begin(), pool.end(), rng);
    for (int i = 0; i < 3; ++i)
    {
        offeredUpgrades_[i] = pool[i];
    }
}

void Game::ApplyUpgrade(UpgradeType upgrade)
{
    switch (upgrade)
    {
        case UpgradeType::PeekRange:
            peekRangeRanks_++;
            break;
        case UpgradeType::PeekSuspicionDown:
            peekSuspicionRanks_++;
            break;
        case UpgradeType::PeekProgressUp:
            peekProgressRanks_++;
            break;
        case UpgradeType::UnlockWhisper:
            unlockedAbilities_[AbilityIndex(AbilityType::Whisper)] = true;
            selectedAbility_ = AbilityType::Whisper;
            break;
        case UpgradeType::UnlockFakeCough:
            unlockedAbilities_[AbilityIndex(AbilityType::FakeCough)] = true;
            selectedAbility_ = AbilityType::FakeCough;
            break;
        case UpgradeType::UnlockMirror:
            unlockedAbilities_[AbilityIndex(AbilityType::Mirror)] = true;
            selectedAbility_ = AbilityType::Mirror;
            break;
        case UpgradeType::Distraction:
            distractionRanks_++;
            distractionCharges_++;
            break;
        default:
            break;
    }
}

void Game::EnterFail(const std::string& reason)
{
    failReason_ = reason;
    state_ = GameState::RunFail;
    PlayCaughtSfx();
}

void Game::UpdateMenuInput()
{
    const Vector2 mouse = GetMousePosition();
    const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked && CheckCollisionPointRec(mouse, MenuButtonRect(0)))
    {
        StartNewRun();
        return;
    }
    if (clicked && CheckCollisionPointRec(mouse, MenuButtonRect(1)))
    {
        state_ = GameState::HelpMenu;
        return;
    }
    if (clicked && CheckCollisionPointRec(mouse, MenuButtonRect(2)))
    {
        shouldExit_ = true;
        return;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_S))
    {
        StartNewRun();
        return;
    }
    if (IsKeyPressed(KEY_H))
    {
        state_ = GameState::HelpMenu;
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_X))
    {
        shouldExit_ = true;
    }
}

void Game::UpdateHelpInput()
{
    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, HelpBackButtonRect()))
    {
        state_ = GameState::MainMenu;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ENTER))
    {
        state_ = GameState::MainMenu;
    }
}

void Game::UpdateWeekIntroInput()
{
    if (IsKeyPressed(KEY_ENTER) || weekMessageTimer_ >= 1.0f)
    {
        state_ = GameState::InWeek;
    }
}

void Game::UpdateWeekClearInput()
{
    if (IsKeyPressed(KEY_ENTER))
    {
        StartWeek(weekIndex_ + 1);
    }
}

void Game::UpdateWeekUpgradeInput()
{
    if (IsKeyPressed(KEY_ONE))
    {
        ApplyUpgrade(offeredUpgrades_[0]);
        StartWeek(weekIndex_ + 1);
    }
    else if (IsKeyPressed(KEY_TWO))
    {
        ApplyUpgrade(offeredUpgrades_[1]);
        StartWeek(weekIndex_ + 1);
    }
    else if (IsKeyPressed(KEY_THREE))
    {
        ApplyUpgrade(offeredUpgrades_[2]);
        StartWeek(weekIndex_ + 1);
    }
}

void Game::UpdateEndScreenInput()
{
    if (state_ == GameState::RunFail)
    {
        const Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(mouse, RunFailButtonRect(0)))
            {
                StartNewRun();
                return;
            }
            if (CheckCollisionPointRec(mouse, RunFailButtonRect(1)))
            {
                state_ = GameState::MainMenu;
                return;
            }
        }

        if (IsKeyPressed(KEY_R))
        {
            StartNewRun();
            return;
        }
        if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
        {
            state_ = GameState::MainMenu;
        }
        return;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        state_ = GameState::MainMenu;
    }
}

void Game::UpdateInWeek(float dt)
{
    HandleAbilityHotkeys();
    HandleMovementInput();
    UpdateMovement(dt);
    UpdateTeacher(dt);
    UpdateInteractions(dt);
    UpdateSuspicion(dt);
    UpdateMirror(dt);
    UpdateAbilityCooldowns(dt);
}

void Game::UpdateMovement(float dt)
{
    if (playerMoving_)
    {
        playerMoveT_ += dt / config::kPlayerMoveSec;
        if (playerMoveT_ >= 1.0f)
        {
            playerMoveT_ = 1.0f;
            playerMoving_ = false;
        }
    }

    if (teacherMoveT_ < 1.0f)
    {
        teacherMoveT_ += dt / std::max(0.1f, EffectiveTeacherMoveSec());
        if (teacherMoveT_ > 1.0f)
        {
            teacherMoveT_ = 1.0f;
        }
    }
}

void Game::UpdateTeacher(float dt)
{
    teacher_.stateTimer += dt;
    teacherStepTimer_ += dt;

    const float moveSec = EffectiveTeacherMoveSec();
    const float scanInterval = EffectiveTeacherScanInterval();

    switch (teacher_.state)
    {
        case TeacherState::Patrol:
        {
            if (teacher_.stateTimer >= scanInterval)
            {
                teacher_.state = TeacherState::Scan;
                teacher_.stateTimer = 0.0f;
                break;
            }

            GridPos target = week_.patrol[teacher_.waypointIndex];
            if (teacher_.pos == target)
            {
                teacher_.waypointIndex = (teacher_.waypointIndex + 1) % static_cast<int>(week_.patrol.size());
                target = week_.patrol[teacher_.waypointIndex];
            }

            if (teacherStepTimer_ >= moveSec)
            {
                teacherStepTimer_ = 0.0f;
                StepTeacherToward(target, moveSec);
            }
        }
        break;

        case TeacherState::Scan:
        {
            teacher_.facingDeg += 110.0f * dt;
            if (teacher_.facingDeg >= 360.0f)
            {
                teacher_.facingDeg -= 360.0f;
            }
            if (teacher_.stateTimer >= 1.0f)
            {
                teacher_.state = TeacherState::Patrol;
                teacher_.stateTimer = 0.0f;
            }
        }
        break;

        case TeacherState::Investigate:
        {
            investigateRemainingSec_ = std::max(0.0f, investigateRemainingSec_ - dt);
            if (teacherStepTimer_ >= moveSec)
            {
                teacherStepTimer_ = 0.0f;
                StepTeacherToward(investigateTarget_, moveSec);
            }
            if (teacher_.pos == investigateTarget_ || investigateRemainingSec_ <= 0.0f)
            {
                teacher_.state = TeacherState::Scan;
                teacher_.stateTimer = 0.0f;
            }
        }
        break;

        case TeacherState::Alert:
        {
            investigateRemainingSec_ = std::max(0.0f, investigateRemainingSec_ - dt);
            if (teacherStepTimer_ >= moveSec * 0.9f)
            {
                teacherStepTimer_ = 0.0f;
                StepTeacherToward(investigateTarget_, moveSec * 0.9f);
            }
            if (investigateRemainingSec_ <= 0.0f)
            {
                teacher_.state = TeacherState::Patrol;
                teacher_.stateTimer = 0.0f;
            }
        }
        break;

        default:
            break;
    }
}

void Game::UpdateSuspicion(float dt)
{
    const bool cheatingNow = IsCheatingActionActive();
    const bool seesPlayerNow = TeacherSeesPlayerNow();
    teacherSeesPlayer_ = seesPlayerNow;

    if (seesPlayerNow && stealth::ManhattanDistance(player_.pos, teacher_.pos) <= 1)
    {
        EnterFail("Teacher saw you at point-blank range.");
        return;
    }

    suspicionRate_ = 0.0f;
    if (seesPlayerNow && cheatingNow)
    {
        suspicionRate_ = 18.0f * tuning_.suspicionGainMult;
        AddSuspicion(suspicionRate_ * dt);
    }
    else if (seesPlayerNow && !cheatingNow)
    {
        suspicionRate_ = config::kSuspicionSeenPerSec * tuning_.suspicionGainMult;
        AddSuspicion(suspicionRate_ * dt);
    }
    else if (!seesPlayerNow && !cheatingNow && player_.pos == week_.playerSpawn)
    {
        suspicionRate_ = -config::kSuspicionDecayPerSec;
        AddSuspicion(suspicionRate_ * dt);
    }

    if (seesPlayerNow && cheatingNow)
    {
        cheatingSeenTimer_ += dt;
        if (!wasSeeingCheatingLastFrame_)
        {
            timesSeenCheating_++;
        }
        if (cheatingSeenTimer_ >= config::kCheatingCaughtSec)
        {
            EnterFail("Teacher spotted a cheat action.");
        }
    }
    else
    {
        cheatingSeenTimer_ = 0.0f;
    }

    wasSeeingCheatingLastFrame_ = seesPlayerNow && cheatingNow;
    cheatingActionTimer_ = std::max(0.0f, cheatingActionTimer_ - dt);
}

void Game::UpdateInteractions(float dt)
{
    peeking_ = false;

    if (!IsKeyDown(KEY_SPACE))
    {
        peekHoldSec_ = 0.0f;
        submitHoldSec_ = 0.0f;
        spaceActionTriggered_ = false;
        return;
    }

    if (playerMoving_)
    {
        return;
    }

    if (player_.pos == week_.submitPos && quizProgress_ >= config::kProgressTarget)
    {
        submitHoldSec_ += dt;
        if (submitHoldSec_ >= config::kSubmitHoldSec && !spaceActionTriggered_)
        {
            spaceActionTriggered_ = true;
            AdvanceWeekOrEndRun();
        }
        return;
    }

    const int target = FindPeekTarget(false);
    if (target < 0)
    {
        peekHoldSec_ = 0.0f;
        return;
    }

    peeking_ = true;
    AddSuspicion(config::kSuspicionPeekPerSec * PeekSuspicionMultiplier() * dt);
    cheatingActionTimer_ = std::max(cheatingActionTimer_, 0.05f);
    peekHoldSec_ += dt;

    if (peekHoldSec_ >= config::kPeekHoldSec && !spaceActionTriggered_)
    {
        spaceActionTriggered_ = true;
        Student& student = week_.students[target];
        if (!student.consumed)
        {
            student.consumed = true;
            AddProgress(config::PeekGain(weekIndex_, student.trait) + PeekProgressBonus());
            AddSuspicion(4.0f * PeekSuspicionMultiplier());
            PlayPeekSfx();
        }
    }
}

void Game::UpdateAbilityCooldowns(float dt)
{
    for (float& cooldown : abilityCooldowns_)
    {
        cooldown = std::max(0.0f, cooldown - dt);
    }
}

void Game::UpdateMirror(float dt)
{
    if (!player_.mirrorActive)
    {
        return;
    }
    player_.mirrorTimer = std::max(0.0f, player_.mirrorTimer - dt);
    if (player_.mirrorTimer <= 0.0f)
    {
        player_.mirrorActive = false;
    }
}

void Game::HandleMovementInput()
{
    if (playerMoving_)
    {
        return;
    }

    if (IsKeyDown(KEY_W))
    {
        TryMovePlayer(0, -1);
    }
    else if (IsKeyDown(KEY_S))
    {
        TryMovePlayer(0, 1);
    }
    else if (IsKeyDown(KEY_A))
    {
        TryMovePlayer(-1, 0);
    }
    else if (IsKeyDown(KEY_D))
    {
        TryMovePlayer(1, 0);
    }
}

void Game::HandleAbilityHotkeys()
{
    if (IsKeyPressed(KEY_ONE))
    {
        CastAbility(AbilityType::Whisper);
    }
    else if (IsKeyPressed(KEY_TWO))
    {
        CastAbility(AbilityType::FakeCough);
    }
    else if (IsKeyPressed(KEY_THREE))
    {
        CastAbility(AbilityType::Mirror);
    }

    if (IsKeyPressed(KEY_FOUR) && TriggerDistraction())
    {
        PlayAbilitySfx();
    }
}

void Game::TryMovePlayer(int dx, int dy)
{
    const GridPos next{player_.pos.x + dx, player_.pos.y + dy};
    if (!stealth::InBounds(week_, next))
    {
        return;
    }
    if (!stealth::IsWalkable(stealth::GetTile(week_, next)))
    {
        return;
    }

    playerMoveFrom_ = player_.pos;
    playerMoveTo_ = next;
    playerMoveT_ = 0.0f;
    playerMoving_ = true;
    player_.pos = next;
}

void Game::CastAbility(AbilityType ability)
{
    selectedAbility_ = ability;

    if (!IsAbilityUnlocked(ability))
    {
        return;
    }

    const int idx = AbilityIndex(ability);
    if (abilityCooldowns_[idx] > 0.0f)
    {
        return;
    }

    bool success = false;
    switch (ability)
    {
        case AbilityType::Whisper:
            success = TriggerWhisper();
            break;
        case AbilityType::FakeCough:
            if (!TeacherSeesPlayerNow())
            {
                success = TriggerFakeCough();
            }
            break;
        case AbilityType::Mirror:
            success = TriggerMirror();
            break;
        default:
            break;
    }

    if (!success)
    {
        return;
    }

    abilityCooldowns_[idx] = config::AbilityCooldown(ability);
    cheatingActionTimer_ = std::max(cheatingActionTimer_, 0.25f);
    PlayAbilitySfx();
}

bool Game::TriggerWhisper()
{
    const int target = FindPeekTarget(true);
    if (target < 0)
    {
        return false;
    }

    Student& student = week_.students[target];
    if (student.consumed)
    {
        return false;
    }

    student.consumed = true;
    AddProgress(config::WhisperGain(student.trait));
    return true;
}

bool Game::TriggerFakeCough()
{
    investigateTarget_ = player_.pos;
    investigateRemainingSec_ = config::kInvestigateDurationFakeCough;
    teacher_.state = TeacherState::Investigate;
    teacher_.stateTimer = 0.0f;
    AddSuspicion(15.0f);
    return true;
}

bool Game::TriggerMirror()
{
    player_.mirrorActive = true;
    player_.mirrorTimer = config::kMirrorDurationSec;
    AddSuspicion(6.0f);
    return true;
}

bool Game::TriggerDistraction()
{
    if (!HasDistractionCharge())
    {
        return false;
    }

    const GridPos awayTarget{teacher_.pos.x - (player_.pos.x - teacher_.pos.x),
                             teacher_.pos.y - (player_.pos.y - teacher_.pos.y)};
    teacher_.facingDeg = stealth::DirectionDeg(teacher_.pos, awayTarget);
    teacher_.state = TeacherState::Scan;
    teacher_.stateTimer = 0.0f;
    distractionCharges_--;
    AddSuspicion(config::kDistractionSuspicionCost);
    cheatingActionTimer_ = std::max(cheatingActionTimer_, 0.15f);
    return true;
}

bool Game::IsAbilityUnlocked(AbilityType ability) const
{
    const int idx = AbilityIndex(ability);
    return idx >= 0 && idx < static_cast<int>(unlockedAbilities_.size()) && unlockedAbilities_[idx];
}

int Game::AbilityIndex(AbilityType ability) const
{
    return static_cast<int>(ability);
}

int Game::FindPeekTarget(bool requireAdjacentOnly) const
{
    int bestIdx = -1;
    int bestDist = std::numeric_limits<int>::max();
    const int maxPeekRange = requireAdjacentOnly ? 1 : EffectivePeekRange();

    for (int i = 0; i < static_cast<int>(week_.students.size()); ++i)
    {
        const Student& student = week_.students[i];
        if (student.consumed)
        {
            continue;
        }

        const int dist = stealth::ManhattanDistance(player_.pos, student.pos);
        bool valid = false;
        if (dist >= 1 && dist <= maxPeekRange && stealth::HasLineOfSight(week_, player_.pos, student.pos, 0))
        {
            valid = true;
        }
        else if (!requireAdjacentOnly && player_.mirrorActive && CanMirrorPeekStudent(student))
        {
            valid = true;
        }

        if (!valid)
        {
            continue;
        }

        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx = i;
        }
    }

    return bestIdx;
}

int Game::FindAdjacentStudent() const
{
    return FindPeekTarget(true);
}

bool Game::CanMirrorPeekStudent(const Student& student) const
{
    const int dist = stealth::ManhattanDistance(player_.pos, student.pos);
    if (dist > 2)
    {
        return false;
    }
    return stealth::HasLineOfSight(week_, player_.pos, student.pos, 1);
}

bool Game::IsCheatingActionActive() const
{
    return peeking_ || (cheatingActionTimer_ > 0.0f);
}

void Game::AddSuspicion(float amount)
{
    const float before = suspicion_;
    suspicion_ = ClampF(suspicion_ + amount, config::kSuspicionMin, config::kSuspicionMax);
    if (suspicion_ > peakSuspicion_)
    {
        peakSuspicion_ = suspicion_;
    }

    if (amount > 0.0f)
    {
        lastSuspiciousPos_ = player_.pos;
    }

    if (before < config::kSuspicionMax && suspicion_ >= config::kSuspicionMax)
    {
        EnterFail("Suspicion hit 100.");
        return;
    }

    if (before < config::kSuspicionThreshold90 && suspicion_ >= config::kSuspicionThreshold90 &&
        teacher_.state != TeacherState::Alert)
    {
        investigateTarget_ = lastSuspiciousPos_;
        investigateRemainingSec_ = config::kAlertDurationSec;
        teacher_.state = TeacherState::Alert;
        teacher_.stateTimer = 0.0f;
    }
}

void Game::AddProgress(int amount)
{
    quizProgress_ = std::min(static_cast<float>(config::kProgressTarget), quizProgress_ + static_cast<float>(amount));
    totalProgress_ += static_cast<float>(amount);
}

void Game::StepTeacherToward(GridPos target, float moveIntervalSec)
{
    auto path = stealth::BuildPath(week_, teacher_.pos, target);
    if (path.size() < 2)
    {
        return;
    }

    const GridPos from = teacher_.pos;
    const GridPos to = path[1];

    teacherMoveFrom_ = from;
    teacherMoveTo_ = to;
    teacherMoveT_ = 0.0f;

    teacher_.pos = to;
    teacherStepTimer_ = std::max(0.0f, teacherStepTimer_ - moveIntervalSec);
    SetTeacherFacing(from, to);
}

void Game::SetTeacherFacing(GridPos from, GridPos to)
{
    teacher_.facingDeg = stealth::DirectionDeg(from, to);
}

float Game::EffectiveTeacherMoveSec() const
{
    float move = tuning_.teacherMoveSec;
    if (suspicion_ >= config::kSuspicionThreshold70)
    {
        move /= config::kTeacherSpeedMultiplierAt70;
    }
    return std::max(0.10f, move);
}

float Game::EffectiveTeacherScanInterval() const
{
    float interval = tuning_.scanIntervalSec;
    if (suspicion_ >= config::kSuspicionThreshold35)
    {
        interval *= config::kScanIntervalMultiplierAt35;
    }
    return interval;
}

float Game::EffectiveTeacherFovDeg() const
{
    float fov = tuning_.fovDeg;
    if (suspicion_ >= config::kSuspicionThreshold70)
    {
        fov += config::kTeacherExtraFovAt70;
    }
    return fov;
}

bool Game::TeacherSeesPlayerNow() const
{
    return stealth::IsInTeacherFov(week_, teacher_, player_.pos, EffectiveTeacherFovDeg(), tuning_.fovRangeTiles);
}

int Game::EffectivePeekRange() const
{
    return 1 + peekRangeRanks_ * config::kPeekRangeBonusPerRank;
}

float Game::PeekSuspicionMultiplier() const
{
    const float multiplier = 1.0f - static_cast<float>(peekSuspicionRanks_) * config::kPeekSuspicionReductionPerRank;
    return std::max(0.2f, multiplier);
}

int Game::PeekProgressBonus() const
{
    return peekProgressRanks_ * config::kPeekProgressBonusPerRank;
}

bool Game::HasDistractionCharge() const
{
    return distractionCharges_ > 0;
}

int Game::ComputeScore() const
{
    int score = weeksCleared_ * 1000 + static_cast<int>(totalProgress_ * 5.0f) - static_cast<int>(peakSuspicion_ * 3.0f) -
                timesSeenCheating_ * 100;
    if (score < 0)
    {
        score = 0;
    }
    return score;
}

void Game::PlayPeekSfx()
{
    if (audioReady_ && sfxPeek_.loaded)
    {
        PlaySound(sfxPeek_.sound);
    }
}

void Game::PlayAbilitySfx()
{
    if (audioReady_ && sfxAbility_.loaded)
    {
        PlaySound(sfxAbility_.sound);
    }
}

void Game::PlayCaughtSfx()
{
    if (audioReady_ && sfxCaught_.loaded)
    {
        PlaySound(sfxCaught_.sound);
    }
}
