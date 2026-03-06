#include "WeekGenerator.h"

#include <algorithm>
#include <array>
#include <random>
#include <utility>

#include "Config.h"
#include "StealthSystems.h"

namespace
{
constexpr int kTemplateCount = 4;

struct TemplateData
{
    std::array<GridPos, 4> corners{};
    bool horizontalStripes = true;
};

TemplateData GetTemplate(int idx)
{
    switch (idx)
    {
        case 0: return TemplateData{{GridPos{3, 3}, GridPos{12, 3}, GridPos{12, 12}, GridPos{3, 12}}, true};
        case 1: return TemplateData{{GridPos{2, 2}, GridPos{13, 2}, GridPos{13, 13}, GridPos{2, 13}}, false};
        case 2: return TemplateData{{GridPos{4, 2}, GridPos{11, 4}, GridPos{12, 12}, GridPos{2, 11}}, true};
        case 3: return TemplateData{{GridPos{2, 4}, GridPos{11, 2}, GridPos{13, 11}, GridPos{4, 13}}, false};
        default: return TemplateData{{GridPos{3, 3}, GridPos{12, 3}, GridPos{12, 12}, GridPos{3, 12}}, true};
    }
}

void FillWalls(WeekData& week)
{
    week.tiles.assign(week.w * week.h, TileType::Floor);
    for (int y = 0; y < week.h; ++y)
    {
        for (int x = 0; x < week.w; ++x)
        {
            if (x == 0 || y == 0 || x == week.w - 1 || y == week.h - 1)
            {
                week.tiles[y * week.w + x] = TileType::Wall;
            }
        }
    }
}

void PlaceDesks(WeekData& week, const TemplateData& tpl)
{
    const int minX = 2;
    const int maxX = week.w - 3;
    const int minY = 2;
    const int maxY = week.h - 3;

    if (tpl.horizontalStripes)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            if (y % 2 == 0)
            {
                continue;
            }
            for (int x = minX; x <= maxX; ++x)
            {
                if (x % 3 == 0)
                {
                    continue;
                }
                week.tiles[y * week.w + x] = TileType::Desk;
            }
        }
    }
    else
    {
        for (int x = minX; x <= maxX; ++x)
        {
            if (x % 2 == 0)
            {
                continue;
            }
            for (int y = minY; y <= maxY; ++y)
            {
                if (y % 3 == 0)
                {
                    continue;
                }
                week.tiles[y * week.w + x] = TileType::Desk;
            }
        }
    }

    // Keep a central lane open.
    for (int y = 1; y < week.h - 1; ++y)
    {
        week.tiles[y * week.w + (week.w / 2)] = TileType::Floor;
    }
}

void PlaceFixedTiles(WeekData& week)
{
    week.playerSpawn = GridPos{2, week.h - 3};
    week.submitPos = GridPos{week.w - 3, 2};
    week.tiles[week.submitPos.y * week.w + week.submitPos.x] = TileType::Submit;

    GridPos teacherDesk{week.w / 2, 1};
    week.tiles[teacherDesk.y * week.w + teacherDesk.x] = TileType::TeacherDesk;

    // Ensure walkable rings around critical points.
    const GridPos critical[2] = {week.playerSpawn, week.submitPos};
    for (const GridPos& c : critical)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                GridPos p{c.x + dx, c.y + dy};
                if (p.x <= 0 || p.y <= 0 || p.x >= week.w - 1 || p.y >= week.h - 1)
                {
                    continue;
                }
                if (week.tiles[p.y * week.w + p.x] == TileType::Desk)
                {
                    week.tiles[p.y * week.w + p.x] = TileType::Floor;
                }
            }
        }
    }
}

std::vector<GridPos> CollectDeskSeats(const WeekData& week)
{
    std::vector<GridPos> seats{};
    for (int y = 1; y < week.h - 1; ++y)
    {
        for (int x = 1; x < week.w - 1; ++x)
        {
            GridPos p{x, y};
            if (week.tiles[y * week.w + x] != TileType::Desk)
            {
                continue;
            }
            seats.push_back(p);
        }
    }
    return seats;
}

StudentTrait RollTrait(int weekIndex, std::mt19937& rng)
{
    auto weights = config::TraitWeights(weekIndex);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float r = dist(rng);

    if (r < weights[0])
    {
        return StudentTrait::Helpful;
    }
    if (r < (weights[0] + weights[1]))
    {
        return StudentTrait::Normal;
    }
    return StudentTrait::Snitchy;
}

void BuildPatrolLoop(WeekData& week, const TemplateData& tpl)
{
    week.patrol.clear();
    for (const GridPos& c : tpl.corners)
    {
        week.patrol.push_back(c);
    }
    week.patrol.push_back(tpl.corners[0]);

    // Snap patrol points to walkable floor tiles.
    for (GridPos& p : week.patrol)
    {
        p.x = std::max(1, std::min(week.w - 2, p.x));
        p.y = std::max(1, std::min(week.h - 2, p.y));
        if (!stealth::IsWalkable(week.tiles[p.y * week.w + p.x]))
        {
            week.tiles[p.y * week.w + p.x] = TileType::Floor;
        }
    }
}

bool ValidateWeek(const WeekData& week)
{
    auto path = stealth::BuildPath(week, week.playerSpawn, week.submitPos);
    if (path.empty())
    {
        return false;
    }

    int reachableStudents = 0;
    for (const Student& s : week.students)
    {
        if (stealth::IsAdjacentReachable(week, week.playerSpawn, s.pos))
        {
            reachableStudents++;
        }
    }

    if (week.students.empty())
    {
        return false;
    }
    const float ratio = static_cast<float>(reachableStudents) / static_cast<float>(week.students.size());
    return ratio >= 0.70f;
}
} // namespace

WeekData WeekGenerator::Generate(int weekIndex, uint32_t seed)
{
    const WeekTuning tuning = config::GetWeekTuning(weekIndex);

    for (int attempt = 0; attempt < 20; ++attempt)
    {
        std::mt19937 rng(seed + static_cast<uint32_t>(weekIndex * 131 + attempt * 911));
        std::uniform_int_distribution<int> tplDist(0, kTemplateCount - 1);

        int tplIdx = tplDist(rng);
        if (kTemplateCount > 1)
        {
            while (tplIdx == lastTemplateIndex_)
            {
                tplIdx = tplDist(rng);
            }
        }

        WeekData week{};
        week.w = config::kGridW;
        week.h = config::kGridH;
        week.seed = seed + static_cast<uint32_t>(attempt * 17);

        const TemplateData tpl = GetTemplate(tplIdx);
        FillWalls(week);
        PlaceDesks(week, tpl);
        PlaceFixedTiles(week);
        BuildPatrolLoop(week, tpl);

        std::vector<GridPos> seats = CollectDeskSeats(week);
        std::shuffle(seats.begin(), seats.end(), rng);

        const int seatCount = std::min(static_cast<int>(seats.size()), tuning.studentCount);
        week.students.clear();
        week.students.reserve(seatCount);
        for (int i = 0; i < seatCount; ++i)
        {
            Student s{};
            s.pos = seats[i];
            s.trait = RollTrait(weekIndex, rng);
            s.consumed = false;
            week.students.push_back(s);
        }

        if (ValidateWeek(week))
        {
            lastTemplateIndex_ = tplIdx;
            return week;
        }
    }

    // Guaranteed fallback map if random attempts fail.
    WeekData fallback{};
    fallback.w = config::kGridW;
    fallback.h = config::kGridH;
    fallback.seed = seed;
    fallback.tiles.assign(fallback.w * fallback.h, TileType::Floor);
    for (int y = 0; y < fallback.h; ++y)
    {
        for (int x = 0; x < fallback.w; ++x)
        {
            if (x == 0 || y == 0 || x == fallback.w - 1 || y == fallback.h - 1)
            {
                fallback.tiles[y * fallback.w + x] = TileType::Wall;
            }
        }
    }
    fallback.playerSpawn = GridPos{2, fallback.h - 3};
    fallback.submitPos = GridPos{fallback.w - 3, 2};
    fallback.tiles[fallback.submitPos.y * fallback.w + fallback.submitPos.x] = TileType::Submit;
    fallback.patrol = {GridPos{3, 3}, GridPos{12, 3}, GridPos{12, 12}, GridPos{3, 12}, GridPos{3, 3}};

    std::mt19937 fallbackRng(seed + 451);
    for (int i = 0; i < tuning.studentCount; ++i)
    {
        Student s{};
        s.pos = GridPos{2 + (i % 11), 4 + (i / 11)};
        s.trait = RollTrait(weekIndex, fallbackRng);
        if (s.pos.x >= fallback.w - 1)
        {
            s.pos.x = fallback.w - 2;
        }
        if (s.pos.y >= fallback.h - 1)
        {
            s.pos.y = fallback.h - 2;
        }
        fallback.students.push_back(s);
    }
    return fallback;
}
