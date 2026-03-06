#pragma once

#include <vector>

#include "Types.h"

namespace stealth
{
int ToIndex(const WeekData& week, GridPos pos);
bool InBounds(const WeekData& week, GridPos pos);
TileType GetTile(const WeekData& week, GridPos pos);
bool IsWalkable(TileType tile);
bool IsOpaque(TileType tile);
int ManhattanDistance(GridPos a, GridPos b);
bool HasLineOfSight(const WeekData& week, GridPos from, GridPos to, int allowedOpaqueTiles = 0);
bool IsInTeacherFov(const WeekData& week, const Teacher& teacher, GridPos target, float fovDeg, int rangeTiles);
std::vector<GridPos> BuildPath(const WeekData& week, GridPos start, GridPos goal);
bool IsAdjacentReachable(const WeekData& week, GridPos start, GridPos targetDesk);
float DirectionDeg(GridPos from, GridPos to);
} // namespace stealth
