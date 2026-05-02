#pragma once

#include "BuildOrderTypes.h"

#include <string>
#include <vector>

std::vector<std::string> ResolveStepIconIds(const BuildStep& step, BuildRace eRace);
