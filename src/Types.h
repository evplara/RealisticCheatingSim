#pragma once

#include <cstdint>
#include <vector>

enum class GameState
{
    MainMenu,
    HelpMenu,
    WeekIntro,
    InWeek,
    WeekClear,
    WeekUpgrade,
    RunSuccess,
    RunFail
};

enum class TileType
{
    Floor,
    Desk,
    TeacherDesk,
    Submit,
    Wall
};

enum class StudentTrait
{
    Helpful,
    Normal,
    Snitchy
};

enum class TeacherState
{
    Patrol,
    Scan,
    Investigate,
    Alert
};

enum class AbilityType
{
    Whisper = 0,
    FakeCough = 1,
    Mirror = 2
};

enum class UpgradeType
{
    PeekRange,
    PeekSuspicionDown,
    PeekProgressUp,
    Distraction
};

struct GridPos
{
    int x = 0;
    int y = 0;

    bool operator==(const GridPos& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const GridPos& other) const
    {
        return !(*this == other);
    }
};

struct Student
{
    GridPos pos{};
    StudentTrait trait = StudentTrait::Normal;
    bool consumed = false;
};

struct Player
{
    GridPos pos{};
    bool mirrorActive = false;
    float mirrorTimer = 0.0f;
};

struct Teacher
{
    GridPos pos{};
    int waypointIndex = 0;
    TeacherState state = TeacherState::Patrol;
    float stateTimer = 0.0f;
    float facingDeg = 0.0f;
};

struct WeekTuning
{
    float teacherMoveSec = 0.35f;
    float fovDeg = 70.0f;
    int fovRangeTiles = 5;
    float scanIntervalSec = 4.0f;
    float suspicionGainMult = 1.0f;
    int studentCount = 9;
};

struct WeekData
{
    std::vector<TileType> tiles{};
    int w = 0;
    int h = 0;
    GridPos playerSpawn{};
    GridPos submitPos{};
    std::vector<Student> students{};
    std::vector<GridPos> patrol{};
    uint32_t seed = 0;
};
