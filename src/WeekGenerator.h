#pragma once

#include <cstdint>

#include "Types.h"

class WeekGenerator
{
public:
    WeekData Generate(int weekIndex, uint32_t seed);

private:
    int lastTemplateIndex_ = -1;
};
