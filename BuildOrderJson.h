#pragma once

#include "BuildOrderTypes.h"
#include <string>

bool BuildOrderFromJsonString(const std::string& sJson, BuildOrder& outOrder, std::string& outError);
bool BuildOrderToJsonString(const BuildOrder& order, std::string& outJson, std::string& outError);
