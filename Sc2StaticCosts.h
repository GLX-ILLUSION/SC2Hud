#pragma once

#include "BuildOrderTypes.h"

#include <vector>

// Custos mineral/gas do multiplayer, alinhados ao balanceamento do ladder (mesma fonte de dados
// que a SC2API oficial le via catalogo / UnitTypeData — nao existe API HTTP publica da Blizzard).
void ApplySc2AutoCostsToBuildSteps(BuildRace eRace, std::vector<BuildStep>& vSteps);

// Ex.: "Gateway x2", "Hellion x8", "Zealot×4" -> 2, 8, 4; sem quantidade valida -> 1.
int ParseNoteTrailingBatchQuantity(const std::string& sNote);

// Remove zero ou mais sufixos " (texto...)" no fim (ex.: "Adept (Chrono Boost)" -> "Adept").
void StripTrailingParentheticalClauses(std::string& s);
