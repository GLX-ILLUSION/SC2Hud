#pragma once

#include "BuildOrderTypes.h"

#include <string>

// Interpreta texto colado (Spawning Tool e similares): colunas supply, m:ss ou h:mm:ss, acao.
// Linhas iniciais que nao casam com um passo viram o titulo; raca inferida (TvT, ZvZ, ...). Builds longas: minutos sem limite de 2h.
bool ParseWebsiteBuildOrderPaste(const std::string& sPaste, BuildOrder& outOrder, std::string& outError);
void BuildOrderPasteSuggestedFileBase(const std::string& sBuildName, char* cBuf, size_t nBuf);
