#include "StealthSystems.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace
{
constexpr GridPos kDirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

float NormalizeDeg(float deg)
{
    while (deg < 0.0f)
    {
        deg += 360.0f;
    }
    while (deg >= 360.0f)
    {
        deg -= 360.0f;
    }
    return deg;
}

float SmallestAngleDiff(float a, float b)
{
    float diff = std::fmod((a - b + 540.0f), 360.0f) - 180.0f;
    return std::fabs(diff);
}
} // namespace

namespace stealth
{
int ToIndex(const WeekData& week, GridPos pos)
{
    return pos.y * week.w + pos.x;
}

bool InBounds(const WeekData& week, GridPos pos)
{
    return pos.x >= 0 && pos.x < week.w && pos.y >= 0 && pos.y < week.h;
}

TileType GetTile(const WeekData& week, GridPos pos)
{
    if (!InBounds(week, pos))
    {
        return TileType::Wall;
    }
    return week.tiles[ToIndex(week, pos)];
}

bool IsWalkable(TileType tile)
{
    return tile == TileType::Floor || tile == TileType::Submit;
}

bool IsOpaque(TileType tile)
{
    return tile == TileType::Desk || tile == TileType::TeacherDesk || tile == TileType::Wall;
}

int ManhattanDistance(GridPos a, GridPos b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

float DirectionDeg(GridPos from, GridPos to)
{
    float dx = static_cast<float>(to.x - from.x);
    float dy = static_cast<float>(to.y - from.y);
    float angle = std::atan2f(dy, dx) * 180.0f / 3.14159265f;
    return NormalizeDeg(angle);
}

bool HasLineOfSight(const WeekData& week, GridPos from, GridPos to, int allowedOpaqueTiles)
{
    int x0 = from.x;
    int y0 = from.y;
    int x1 = to.x;
    int y1 = to.y;

    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int blockedTiles = 0;

    while (true)
    {
        GridPos p{x0, y0};
        if (p != from && p != to && IsOpaque(GetTile(week, p)))
        {
            blockedTiles++;
            if (blockedTiles > allowedOpaqueTiles)
            {
                return false;
            }
        }

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
    return true;
}

bool IsInTeacherFov(const WeekData& week, const Teacher& teacher, GridPos target, float fovDeg, int rangeTiles)
{
    if (ManhattanDistance(teacher.pos, target) > rangeTiles)
    {
        return false;
    }

    if (!HasLineOfSight(week, teacher.pos, target, 0))
    {
        return false;
    }

    float targetDeg = DirectionDeg(teacher.pos, target);
    float diff = SmallestAngleDiff(targetDeg, teacher.facingDeg);
    return diff <= (fovDeg * 0.5f);
}

std::vector<GridPos> BuildPath(const WeekData& week, GridPos start, GridPos goal)
{
    std::vector<GridPos> empty{};
    if (start == goal)
    {
        return {start};
    }
    if (!InBounds(week, start) || !InBounds(week, goal))
    {
        return empty;
    }
    if (!IsWalkable(GetTile(week, goal)))
    {
        return empty;
    }

    const int total = week.w * week.h;
    std::vector<int> parent(total, -1);
    std::vector<bool> visited(total, false);
    std::queue<GridPos> q;

    q.push(start);
    visited[ToIndex(week, start)] = true;

    bool found = false;
    while (!q.empty())
    {
        GridPos cur = q.front();
        q.pop();

        for (const GridPos& d : kDirs)
        {
            GridPos nxt{cur.x + d.x, cur.y + d.y};
            if (!InBounds(week, nxt))
            {
                continue;
            }
            if (!IsWalkable(GetTile(week, nxt)))
            {
                continue;
            }
            const int idx = ToIndex(week, nxt);
            if (visited[idx])
            {
                continue;
            }
            visited[idx] = true;
            parent[idx] = ToIndex(week, cur);
            if (nxt == goal)
            {
                found = true;
                break;
            }
            q.push(nxt);
        }
        if (found)
        {
            break;
        }
    }

    if (!found)
    {
        return empty;
    }

    std::vector<GridPos> reversed{};
    GridPos cur = goal;
    reversed.push_back(cur);
    while (cur != start)
    {
        int idx = parent[ToIndex(week, cur)];
        if (idx < 0)
        {
            return empty;
        }
        cur = GridPos{idx % week.w, idx / week.w};
        reversed.push_back(cur);
    }

    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

bool IsAdjacentReachable(const WeekData& week, GridPos start, GridPos targetDesk)
{
    for (const GridPos& d : kDirs)
    {
        GridPos adj{targetDesk.x + d.x, targetDesk.y + d.y};
        if (!InBounds(week, adj))
        {
            continue;
        }
        if (!IsWalkable(GetTile(week, adj)))
        {
            continue;
        }
        auto path = BuildPath(week, start, adj);
        if (!path.empty())
        {
            return true;
        }
    }
    return false;
}
} // namespace stealth
