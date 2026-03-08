#pragma once

#include <cstdint>

#include "Types.h"

class WeekGenerator
{
public:
    WeekData Generate(int weekIndex, uint32_t seed);

private:
    int lastHybridTemplateIndex_ = -1;
    int lastClassicTemplateIndex_ = -1;
    int lastLayoutStyle_ = -1;
};
