#include "WeekGenerator.h"

#include <algorithm>
#include <array>
#include <random>
#include <utility>

#include "Config.h"
#include "StealthSystems.h"

namespace
{
constexpr int kRoomTemplateCount = 5;
constexpr int kClassicTemplateCount = 4;

enum class LayoutStyle
{
    Hybrid = 0,
    Classic = 1
};

enum class DeskPattern
{
    BrokenRows,
    BrokenColumns,
    Pods,
    Islands,
    Zigzag
};

struct DeskRect
{
    int x;
    int y;
    int w;
    int h;
};

struct RoomTemplateData
{
    GridPos playerSpawn;
    GridPos submitPos;
    GridPos teacherDesk;
    std::array<GridPos, 4> patrolAnchors;
    std::array<DeskRect, 4> deskZones;
    int zoneCount;
};

struct ClassicTemplateData
{
    std::array<GridPos, 4> corners{};
    bool horizontalStripes = true;
};

int Right(const DeskRect& area)
{
    return area.x + area.w - 1;
}

int Bottom(const DeskRect& area)
{
    return area.y + area.h - 1;
}

RoomTemplateData GetRoomTemplate(int index)
{
    switch (index)
    {
        case 0:
            return RoomTemplateData{
                GridPos{2, 13},
                GridPos{13, 2},
                GridPos{8, 1},
                std::array<GridPos, 4>{GridPos{2, 2}, GridPos{13, 2}, GridPos{13, 13}, GridPos{2, 13}},
                std::array<DeskRect, 4>{DeskRect{2, 2, 12, 2}, DeskRect{2, 5, 12, 2}, DeskRect{2, 8, 12, 2}, DeskRect{2, 11, 12, 2}},
                4};
        case 1:
            return RoomTemplateData{
                GridPos{2, 13},
                GridPos{13, 2},
                GridPos{8, 1},
                std::array<GridPos, 4>{GridPos{3, 3}, GridPos{12, 3}, GridPos{12, 12}, GridPos{3, 12}},
                std::array<DeskRect, 4>{DeskRect{2, 2, 4, 4}, DeskRect{9, 2, 4, 4}, DeskRect{2, 9, 4, 4}, DeskRect{9, 9, 4, 4}},
                4};
        case 2:
            return RoomTemplateData{
                GridPos{7, 13},
                GridPos{8, 2},
                GridPos{8, 1},
                std::array<GridPos, 4>{GridPos{3, 4}, GridPos{12, 4}, GridPos{12, 11}, GridPos{3, 11}},
                std::array<DeskRect, 4>{DeskRect{2, 2, 4, 5}, DeskRect{10, 2, 4, 5}, DeskRect{2, 9, 4, 4}, DeskRect{10, 9, 4, 4}},
                4};
        case 3:
            return RoomTemplateData{
                GridPos{3, 13},
                GridPos{12, 2},
                GridPos{8, 1},
                std::array<GridPos, 4>{GridPos{2, 3}, GridPos{13, 3}, GridPos{12, 13}, GridPos{3, 12}},
                std::array<DeskRect, 4>{DeskRect{2, 2, 10, 2}, DeskRect{4, 5, 10, 2}, DeskRect{2, 8, 10, 2}, DeskRect{4, 11, 10, 2}},
                4};
        default:
            return RoomTemplateData{
                GridPos{2, 12},
                GridPos{13, 3},
                GridPos{8, 1},
                std::array<GridPos, 4>{GridPos{2, 4}, GridPos{13, 2}, GridPos{12, 12}, GridPos{4, 13}},
                std::array<DeskRect, 4>{DeskRect{2, 3, 5, 4}, DeskRect{9, 3, 4, 4}, DeskRect{4, 9, 4, 4}, DeskRect{10, 10, 3, 3}},
                4};
    }
}

ClassicTemplateData GetClassicTemplate(int index)
{
    switch (index)
    {
        case 0: return ClassicTemplateData{{GridPos{3, 3}, GridPos{12, 3}, GridPos{12, 12}, GridPos{3, 12}}, true};
        case 1: return ClassicTemplateData{{GridPos{2, 2}, GridPos{13, 2}, GridPos{13, 13}, GridPos{2, 13}}, false};
        case 2: return ClassicTemplateData{{GridPos{4, 2}, GridPos{11, 4}, GridPos{12, 12}, GridPos{2, 11}}, true};
        case 3: return ClassicTemplateData{{GridPos{2, 4}, GridPos{11, 2}, GridPos{13, 11}, GridPos{4, 13}}, false};
        default: return ClassicTemplateData{{GridPos{3, 3}, GridPos{12, 3}, GridPos{12, 12}, GridPos{3, 12}}, true};
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

void SetDesk(WeekData& week, const DeskRect& area, int x, int y)
{
    if (x < area.x || y < area.y || x > Right(area) || y > Bottom(area))
    {
        return;
    }
    if (x <= 0 || y <= 0 || x >= week.w - 1 || y >= week.h - 1)
    {
        return;
    }
    if (week.tiles[y * week.w + x] == TileType::Floor)
    {
        week.tiles[y * week.w + x] = TileType::Desk;
    }
}

void CarveFloor(WeekData& week, const DeskRect& area, int x, int y)
{
    if (x < area.x || y < area.y || x > Right(area) || y > Bottom(area))
    {
        return;
    }
    if (x <= 0 || y <= 0 || x >= week.w - 1 || y >= week.h - 1)
    {
        return;
    }
    week.tiles[y * week.w + x] = TileType::Floor;
}

void PlaceBrokenRows(WeekData& week, const DeskRect& area, std::mt19937& rng)
{
    if (area.w < 2 || area.h < 1)
    {
        return;
    }

    std::uniform_int_distribution<int> gapDist(1, 2);
    std::uniform_int_distribution<int> runDist(2, std::max(2, std::min(4, area.w)));
    std::uniform_int_distribution<int> rowStepDist(1, std::max(1, std::min(2, area.h)));
    std::bernoulli_distribution skipRow(0.18);

    for (int y = area.y; y <= Bottom(area); y += rowStepDist(rng))
    {
        if (skipRow(rng))
        {
            continue;
        }

        int x = area.x + gapDist(rng) - 1;
        while (x <= Right(area))
        {
            x += gapDist(rng);
            const int run = runDist(rng);
            for (int i = 0; i < run && x + i <= Right(area); ++i)
            {
                SetDesk(week, area, x + i, y);
            }
            x += run;
        }
    }
}

void PlaceBrokenColumns(WeekData& week, const DeskRect& area, std::mt19937& rng)
{
    if (area.w < 1 || area.h < 2)
    {
        return;
    }

    std::uniform_int_distribution<int> gapDist(1, 2);
    std::uniform_int_distribution<int> runDist(2, std::max(2, std::min(4, area.h)));
    std::uniform_int_distribution<int> colStepDist(1, std::max(1, std::min(2, area.w)));
    std::bernoulli_distribution skipCol(0.18);

    for (int x = area.x; x <= Right(area); x += colStepDist(rng))
    {
        if (skipCol(rng))
        {
            continue;
        }

        int y = area.y + gapDist(rng) - 1;
        while (y <= Bottom(area))
        {
            y += gapDist(rng);
            const int run = runDist(rng);
            for (int i = 0; i < run && y + i <= Bottom(area); ++i)
            {
                SetDesk(week, area, x, y + i);
            }
            y += run;
        }
    }
}

void PlacePods(WeekData& week, const DeskRect& area, std::mt19937& rng)
{
    if (area.w < 2 || area.h < 2)
    {
        return;
    }

    std::uniform_int_distribution<int> sizeDist(0, 1);
    const int maxWidth = std::max(2, std::min(3, area.w));
    const int maxHeight = std::max(2, std::min(3, area.h));
    const int attempts = std::max(4, area.w + area.h);

    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        const int width = std::min(maxWidth, 2 + sizeDist(rng));
        const int height = std::min(maxHeight, 2 + sizeDist(rng));
        std::uniform_int_distribution<int> xDist(area.x, Right(area) - width + 1);
        std::uniform_int_distribution<int> yDist(area.y, Bottom(area) - height + 1);
        const int startX = xDist(rng);
        const int startY = yDist(rng);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if ((x == width - 1 && y == height - 1) && ((attempt + x + y) % 3 == 0))
                {
                    continue;
                }
                SetDesk(week, area, startX + x, startY + y);
            }
        }
    }
}

void PlaceIslands(WeekData& week, const DeskRect& area, std::mt19937& rng)
{
    if (area.w < 2 || area.h < 2)
    {
        return;
    }

    std::bernoulli_distribution fill(0.72);
    for (int y = area.y; y <= Bottom(area) - 1; y += 3)
    {
        for (int x = area.x; x <= Right(area) - 1; x += 3)
        {
            if (!fill(rng))
            {
                continue;
            }

            SetDesk(week, area, x, y);
            SetDesk(week, area, x + 1, y);
            if ((x + y) % 2 == 0)
            {
                SetDesk(week, area, x, y + 1);
            }
            else
            {
                SetDesk(week, area, x + 1, y + 1);
            }
        }
    }
}

void PlaceZigzag(WeekData& week, const DeskRect& area, std::mt19937& rng)
{
    if (area.w < 2 || area.h < 1)
    {
        return;
    }

    std::uniform_int_distribution<int> phaseDist(0, 1);
    const int phase = phaseDist(rng);
    for (int y = area.y; y <= Bottom(area); y += 2)
    {
        const int offset = (((y - area.y) / 2) + phase) % 2;
        for (int x = area.x + offset; x <= Right(area) - 1; x += 4)
        {
            SetDesk(week, area, x, y);
            SetDesk(week, area, x + 1, y);
            if (y + 1 <= Bottom(area) && (x + y + phase) % 3 == 0)
            {
                SetDesk(week, area, x + 1, y + 1);
            }
        }
    }
}

void CarveAisles(WeekData& week, const DeskRect& area, std::mt19937& rng)
{
    if (area.w < 2 || area.h < 2)
    {
        return;
    }

    std::uniform_int_distribution<int> xDist(area.x, Right(area));
    std::uniform_int_distribution<int> yDist(area.y, Bottom(area));

    if (area.w >= 5)
    {
        const int x = xDist(rng);
        for (int y = area.y; y <= Bottom(area); ++y)
        {
            CarveFloor(week, area, x, y);
            if (area.w >= 7 && x + 1 <= Right(area) && ((x - area.x) % 2 == 0))
            {
                CarveFloor(week, area, x + 1, y);
            }
        }
    }

    if (area.h >= 4)
    {
        const int y = yDist(rng);
        for (int x = area.x; x <= Right(area); ++x)
        {
            CarveFloor(week, area, x, y);
        }
    }

    if (area.w >= 3 && area.h >= 3)
    {
        const int pockets = (area.w >= 5 && area.h >= 5) ? 2 : 1;
        for (int i = 0; i < pockets; ++i)
        {
            const int pocketX = xDist(rng);
            const int pocketY = yDist(rng);
            for (int dy = 0; dy < 2; ++dy)
            {
                for (int dx = 0; dx < 2; ++dx)
                {
                    CarveFloor(week, area, pocketX + dx, pocketY + dy);
                }
            }
        }
    }
}

void ApplyPattern(WeekData& week, const DeskRect& area, DeskPattern pattern, std::mt19937& rng)
{
    switch (pattern)
    {
        case DeskPattern::BrokenRows: PlaceBrokenRows(week, area, rng); break;
        case DeskPattern::BrokenColumns: PlaceBrokenColumns(week, area, rng); break;
        case DeskPattern::Pods: PlacePods(week, area, rng); break;
        case DeskPattern::Islands: PlaceIslands(week, area, rng); break;
        case DeskPattern::Zigzag: PlaceZigzag(week, area, rng); break;
        default: PlaceBrokenRows(week, area, rng); break;
    }
}

void PlaceDesks(WeekData& week, const RoomTemplateData& roomTemplate, std::mt19937& rng)
{
    std::uniform_int_distribution<int> patternDist(0, 4);
    std::bernoulli_distribution extraPattern(0.45);

    for (int zoneIndex = 0; zoneIndex < roomTemplate.zoneCount; ++zoneIndex)
    {
        const DeskRect& area = roomTemplate.deskZones[zoneIndex];
        const DeskPattern primaryPattern = static_cast<DeskPattern>(patternDist(rng));
        ApplyPattern(week, area, primaryPattern, rng);

        if (extraPattern(rng))
        {
            const DeskPattern secondaryPattern =
                (primaryPattern == DeskPattern::Pods) ? DeskPattern::Islands : DeskPattern::Pods;
            ApplyPattern(week, area, secondaryPattern, rng);
        }

        CarveAisles(week, area, rng);
    }
}

void PlaceFixedTiles(WeekData& week, const RoomTemplateData& roomTemplate)
{
    week.playerSpawn = roomTemplate.playerSpawn;
    week.submitPos = roomTemplate.submitPos;
    week.tiles[week.playerSpawn.y * week.w + week.playerSpawn.x] = TileType::SpawnDesk;
    week.tiles[week.submitPos.y * week.w + week.submitPos.x] = TileType::Submit;
    week.tiles[roomTemplate.teacherDesk.y * week.w + roomTemplate.teacherDesk.x] = TileType::TeacherDesk;

    const std::array<GridPos, 3> critical{week.playerSpawn, week.submitPos, roomTemplate.teacherDesk};
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

void PlaceClassicDesks(WeekData& week, const ClassicTemplateData& roomTemplate)
{
    const int minX = 2;
    const int maxX = week.w - 3;
    const int minY = 2;
    const int maxY = week.h - 3;

    if (roomTemplate.horizontalStripes)
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

    for (int y = 1; y < week.h - 1; ++y)
    {
        week.tiles[y * week.w + (week.w / 2)] = TileType::Floor;
    }
}

void PlaceClassicFixedTiles(WeekData& week)
{
    week.playerSpawn = GridPos{2, week.h - 3};
    week.submitPos = GridPos{week.w - 3, 2};
    week.tiles[week.playerSpawn.y * week.w + week.playerSpawn.x] = TileType::SpawnDesk;
    week.tiles[week.submitPos.y * week.w + week.submitPos.x] = TileType::Submit;

    GridPos teacherDesk{week.w / 2, 1};
    week.tiles[teacherDesk.y * week.w + teacherDesk.x] = TileType::TeacherDesk;

    const std::array<GridPos, 2> critical{week.playerSpawn, week.submitPos};
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
            const GridPos seat{x, y};
            if (week.tiles[y * week.w + x] != TileType::Desk)
            {
                continue;
            }
            if (!stealth::IsAdjacentReachable(week, week.playerSpawn, seat))
            {
                continue;
            }
            seats.push_back(seat);
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

GridPos FindNearbyWalkable(const WeekData& week, GridPos target)
{
    if (stealth::IsWalkable(week.tiles[target.y * week.w + target.x]))
    {
        return target;
    }

    for (int radius = 1; radius <= 4; ++radius)
    {
        for (int dy = -radius; dy <= radius; ++dy)
        {
            for (int dx = -radius; dx <= radius; ++dx)
            {
                GridPos candidate{target.x + dx, target.y + dy};
                if (candidate.x <= 0 || candidate.y <= 0 || candidate.x >= week.w - 1 || candidate.y >= week.h - 1)
                {
                    continue;
                }
                if (stealth::IsWalkable(week.tiles[candidate.y * week.w + candidate.x]))
                {
                    return candidate;
                }
            }
        }
    }

    return GridPos{week.w / 2, week.h / 2};
}

void BuildPatrolLoop(WeekData& week, const RoomTemplateData& roomTemplate, std::mt19937& rng)
{
    std::uniform_int_distribution<int> jitterDist(-1, 1);

    week.patrol.clear();
    for (const GridPos& anchor : roomTemplate.patrolAnchors)
    {
        GridPos point{
            std::max(1, std::min(week.w - 2, anchor.x + jitterDist(rng))),
            std::max(1, std::min(week.h - 2, anchor.y + jitterDist(rng)))};

        const GridPos patrolPoint = FindNearbyWalkable(week, point);
        if (week.patrol.empty() || week.patrol.back() != patrolPoint)
        {
            week.patrol.push_back(patrolPoint);
        }
    }

    if (week.patrol.size() < 3)
    {
        week.patrol = {GridPos{3, 3}, GridPos{week.w - 4, 3}, GridPos{week.w - 4, week.h - 4}, GridPos{3, week.h - 4}};
    }
    week.patrol.push_back(week.patrol.front());
}

void BuildClassicPatrolLoop(WeekData& week, const ClassicTemplateData& roomTemplate)
{
    week.patrol.clear();
    for (const GridPos& corner : roomTemplate.corners)
    {
        week.patrol.push_back(corner);
    }
    week.patrol.push_back(roomTemplate.corners[0]);

    for (GridPos& point : week.patrol)
    {
        point.x = std::max(1, std::min(week.w - 2, point.x));
        point.y = std::max(1, std::min(week.h - 2, point.y));
        if (!stealth::IsWalkable(week.tiles[point.y * week.w + point.x]))
        {
            week.tiles[point.y * week.w + point.x] = TileType::Floor;
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
        std::uniform_int_distribution<int> styleDist(0, 1);
        LayoutStyle layoutStyle = static_cast<LayoutStyle>(styleDist(rng));
        if (lastLayoutStyle_ >= 0)
        {
            while (static_cast<int>(layoutStyle) == lastLayoutStyle_)
            {
                layoutStyle = static_cast<LayoutStyle>(styleDist(rng));
            }
        }

        WeekData week{};
        week.w = config::kGridW;
        week.h = config::kGridH;
        week.seed = seed + static_cast<uint32_t>(attempt * 17);

        FillWalls(week);

        if (layoutStyle == LayoutStyle::Hybrid)
        {
            std::uniform_int_distribution<int> templateDist(0, kRoomTemplateCount - 1);
            int templateIndex = templateDist(rng);
            if (kRoomTemplateCount > 1)
            {
                while (templateIndex == lastHybridTemplateIndex_)
                {
                    templateIndex = templateDist(rng);
                }
            }

            const RoomTemplateData roomTemplate = GetRoomTemplate(templateIndex);
            PlaceDesks(week, roomTemplate, rng);
            PlaceFixedTiles(week, roomTemplate);
            BuildPatrolLoop(week, roomTemplate, rng);

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
                lastLayoutStyle_ = static_cast<int>(layoutStyle);
                lastHybridTemplateIndex_ = templateIndex;
                return week;
            }
            continue;
        }

        std::uniform_int_distribution<int> classicTemplateDist(0, kClassicTemplateCount - 1);
        int classicTemplateIndex = classicTemplateDist(rng);
        if (kClassicTemplateCount > 1)
        {
            while (classicTemplateIndex == lastClassicTemplateIndex_)
            {
                classicTemplateIndex = classicTemplateDist(rng);
            }
        }

        const ClassicTemplateData classicTemplate = GetClassicTemplate(classicTemplateIndex);
        PlaceClassicDesks(week, classicTemplate);
        PlaceClassicFixedTiles(week);
        BuildClassicPatrolLoop(week, classicTemplate);

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
            lastLayoutStyle_ = static_cast<int>(layoutStyle);
            lastClassicTemplateIndex_ = classicTemplateIndex;
            return week;
        }
    }

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
    fallback.tiles[fallback.playerSpawn.y * fallback.w + fallback.playerSpawn.x] = TileType::SpawnDesk;
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
