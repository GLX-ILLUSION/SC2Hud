#pragma once

#include "BuildOrderTypes.h"

#include <string>

// Faz HTTPS GET em lotv.spawningtool.com/build/..., extrai titulo (h1) e linhas da tabela HTML,
// e preenche outOrder via o mesmo parser que a colagem manual.
bool ImportBuildOrderFromSpawningToolUrl(const std::string& sUrlUtf8, BuildOrder& outOrder, std::string& outError);
