#pragma once

#include <array>
#include <string>

#include <raylib.h>

#include "Types.h"
#include "WeekGenerator.h"

class Game
{
public:
    void Init();
    void Update(float dt);
    void Draw() const;
    bool ShouldExit() const;

private:
    void StartNewRun();
    void StartWeek(int weekIndex);
    void AdvanceWeekOrEndRun();
    void EnterFail(const std::string& reason);

    void UpdateMenuInput();
    void UpdateWeekIntroInput();
    void UpdateWeekClearInput();
    void UpdateEndScreenInput();

    void UpdateInWeek(float dt);
    void UpdateMovement(float dt);
    void UpdateTeacher(float dt);
    void UpdateSuspicion(float dt);
    void UpdateInteractions(float dt);
    void UpdateAbilityCooldowns(float dt);
    void UpdateMirror(float dt);

    void HandleMovementInput();
    void HandleAbilityModeInput();
    void TryMovePlayer(int dx, int dy);
    void CastSelectedAbility();
    bool TriggerWhisper();
    bool TriggerFakeCough();
    bool TriggerMirror();

    bool IsAbilityUnlocked(AbilityType ability) const;
    int AbilityIndex(AbilityType ability) const;

    int FindPeekTarget(bool requireAdjacentOnly) const;
    int FindAdjacentStudent() const;
    bool CanMirrorPeekStudent(const Student& student) const;
    bool IsCheatingActionActive() const;

    void AddSuspicion(float amount);
    void AddProgress(int amount);
    void StepTeacherToward(GridPos target, float moveIntervalSec);
    void SetTeacherFacing(GridPos from, GridPos to);
    float EffectiveTeacherMoveSec() const;
    float EffectiveTeacherScanInterval() const;
    float EffectiveTeacherFovDeg() const;
    bool TeacherSeesPlayerNow() const;

    int ComputeScore() const;
    void PlayPeekSfx();
    void PlayAbilitySfx();
    void PlayCaughtSfx();

private:
    struct RuntimeSound
    {
        Sound sound{};
        bool loaded = false;
    };

    GameState state_ = GameState::MainMenu;
    bool shouldExit_ = false;

    WeekGenerator generator_{};
    WeekData week_{};
    WeekTuning tuning_{};
    int weekIndex_ = 1;
    uint32_t runSeed_ = 0;

    Player player_{};
    Teacher teacher_{};
    GridPos investigateTarget_{};
    float investigateRemainingSec_ = 0.0f;

    float suspicion_ = 0.0f;
    float peakSuspicion_ = 0.0f;
    float suspicionRate_ = 0.0f;
    float cheatingSeenTimer_ = 0.0f;
    bool wasSeeingCheatingLastFrame_ = false;
    GridPos lastSuspiciousPos_{};

    float quizProgress_ = 0.0f;
    float totalProgress_ = 0.0f;
    int weeksCleared_ = 0;
    int timesSeenCheating_ = 0;

    bool abilityMode_ = false;
    AbilityType selectedAbility_ = AbilityType::Whisper;
    std::array<float, 3> abilityCooldowns_{{0.0f, 0.0f, 0.0f}};

    bool playerMoving_ = false;
    GridPos playerMoveFrom_{};
    GridPos playerMoveTo_{};
    float playerMoveT_ = 1.0f;

    GridPos teacherMoveFrom_{};
    GridPos teacherMoveTo_{};
    float teacherMoveT_ = 1.0f;
    float teacherStepTimer_ = 0.0f;

    float peekHoldSec_ = 0.0f;
    float submitHoldSec_ = 0.0f;
    bool spaceActionTriggered_ = false;
    bool peeking_ = false;
    float cheatingActionTimer_ = 0.0f;

    float weekMessageTimer_ = 0.0f;
    std::string failReason_{};
    bool showDebug_ = false;

    bool teacherSeesPlayer_ = false;

    bool audioReady_ = false;
    RuntimeSound sfxPeek_{};
    RuntimeSound sfxAbility_{};
    RuntimeSound sfxCaught_{};
};
