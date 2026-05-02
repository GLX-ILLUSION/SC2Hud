#include "UiLanguage.h"

#include "PathUtils.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
struct Bilingual
{
    const char* en;
    const char* pt;
};

static UiLang g_lang = UiLang::English;

// Order must match `enum class Tr` in UiLanguage.h.
static const Bilingual kUi[] = {
    // Settings panel — title and shortcuts
    { "SC2 Build HUD — Settings", "Configuracao - SC2 Build HUD" },
    { "Close this window (X) or press INSERT to play with only the compact overlay in the corner.",
      "Feche esta janela (X) ou pressione INSERT para jogar so com o overlay compacto no canto." },
    { "Shortcuts", "Atalhos" },
    { "INSERT — open or close this menu", "INSERT — abre ou fecha este menu" },
    { "END — quit the application", "END — fechar o programa" },
    // Settings — language
    { "Language", "Idioma" },
    { "English", "English" },
    { "Portugues (Brasil)", "Portugues (Brasil)" },
    // Main settings tabs
    { "Match", "Partida" },
    { "Builds", "Builds" },
    { "HUD", "HUD" },
    { "Editor", "Editor" },
    // Match tab — game clock
    { "Match clock", "Relogio da partida" },
    { "Match the StarCraft II in-game timer: when the match starts at 0:00, press Start now.",
      "Alinhe com o timer do StarCraft II: quando a partida comeca no relogio do jogo (0:00), pressione Iniciar." },
    { "HUD time: %d:%02d", "Tempo no HUD: %d:%02d" },
    { "Start now", "Iniciar agora" },
    { "Pause", "Pausar" },
    { "Resume", "Retomar" },
    { "Reset", "Zerar" },
    { "Sync with in-game time", "Sincronizar com o tempo do jogo" },
    { "Quick adjust: %.0f s", "Ajuste rapido: %.0f s" },
    { "Apply this time", "Aplicar esse tempo" },
    { "(Optional) Bot / API builds can sync time using the game loop on Faster (~22.4 loops/s) if you wire that externally.",
      "(Opcional) Builds via bot/API podem sincronizar o tempo pelo game loop em Faster (~22.4/s) se integrar isso por fora." },
    { "If the in-game timer is already past 0:00, set the matching seconds and apply. The HUD keeps counting normally after that.",
      "Se o timer do jogo ja passou de 0:00, defina os segundos correspondentes e aplique. Depois o HUD continua contando normalmente." },
    // Builds tab — list and preview
    { "Choose build order", "Escolher build order" },
    { "List comes from the data/builds folder (presets and user/).", "Lista vem da pasta data/builds (presets e builds em user/)." },
    { "Refresh file list", "Atualizar lista de arquivos" },
    { "Saved build", "Build salva" },
    { "Load this build", "Carregar esta build" },
    { "No .json found in data/builds.", "Nenhum .json encontrado em data/builds." },
    { "Preview", "Pre-visualizacao" },
    { "Loaded: %s", "Carregada: %s" },
    { "Race: %s", "Raca: %s" },
    // Editor tab — import, meta, steps
    { "Edit steps in order (supply, time, description). Saves to data/builds/user.",
      "Monte passos na ordem (supply, tempo, descricao). Grava em data/builds/user." },
    { "Mineral/gas costs for ladder races are estimated from the description using a static table (SC2 catalog / SC2API). There is no public HTTP API.",
      "Custos mineral/gas para Terran, Zerg e Protoss vêm de tabela estatica no codigo (catalogo SC2 / SC2API). Nao ha API HTTP publica." },
    { "Import from website or clipboard", "Importar do site ou clipboard" },
    { "Optional title plus lines: supply, time (m:ss or h:mm:ss), action — tab or spaces. Race inferred from TvT, PvZ, etc.",
      "Titulo opcional e linhas: supply, tempo (m:ss ou h:mm:ss), acao — separados por tab ou espacos. Raca deduzida de TvT, PvZ, etc." },
    { "Open Spawning Tool (build orders)", "Abrir Spawning Tool (build orders)" },
    { "Opens https://lotv.spawningtool.com in your default browser.", "Abre https://lotv.spawningtool.com no navegador padrao." },
    { "Import to editor", "Converter para editor" },
    { "Clear paste", "Limpar colagem" },
    { "Copy JSON", "Copiar JSON" },
    { "Name, race and save", "Nome, raca e gravacao" },
    { "Build name", "Nome da build" },
    { "Race", "Raca" },
    { "File name in user/ (no .json)", "Ficheiro em user/ (sem .json)" },
    { "Save to build list", "Salvar na lista de builds" },
    { "Use on HUD now (do not save file)", "Usar no HUD agora (sem gravar ficheiro)" },
    { "After saving, use \"Refresh file list\" on the Builds tab if it does not appear immediately.",
      "Apos gravar, use \"Atualizar lista\" no separador Builds se nao aparecer de imediato." },
    { "Steps", "Passos" },
    { "+ Add step", "+ Adicionar passo" },
    { "Step %d — %s", "Passo %d — %s" },
    { "Pop. (supply)", "Pop. (supply)" },
    { "Seconds on clock", "Segundos no relogio" },
    { "Description", "Descricao" },
    { "Auto quantity: %d", "Quantidade (auto): %d" },
    { "Icon (id, optional)", "Icone (id, opcional)" },
    { "Notes (right column)", "Notas (coluna direita)" },
    { "Remove this step", "Remover este passo" },
    // Race names (shared)
    { "Unknown", "Desconhecida" },
    { "Terran", "Terran" },
    { "Protoss", "Protoss" },
    { "Zerg", "Zerg" },
    // Table / HUD columns
    { "Sup.", "Pop." },
    { "Time", "Tempo" },
    { "Action", "Acao" },
    { "Cost", "Custo" },
    { "Notes", "Notas" },
    { "Visible columns", "Colunas visiveis" },
    { "Affects the compact overlay (INSERT closed) and the preview table. At least one column stays on.",
      "Afeta o overlay compacto (INSERT fechado) e a tabela de preview. Pelo menos uma coluna fica ativa." },
    { "Same data as the overlay list; optional columns.", "Combina com o estilo da lista do overlay: mesmos dados, colunas opcionais." },
    // HUD tab — help copy
    { "In-game overlay", "Overlay no jogo" },
    { "With INSERT closed: follow-along style — large time, current step, and COMING UP list (icons, timing, and costs when applicable).",
      "Com INSERT fechado: estilo follow-along — tempo grande, passo atual e lista COMING UP (icones, tempo e custos quando aplicavel)." },
    { "COMING UP: minerals.png and gas.png in data/icons/common/ when cost is resolved.",
      "COMING UP: minerals.png e gas.png em data/icons/common/ quando o custo e resolvido." },
    { "Race icons: data/icons/<race>/ (e.g. terran/supply_depot.png).", "Icones de raca: data/icons/<raca>/ (ex.: terran/supply_depot.png)." },
    // In-game overlay
    { "ELAPSED TIME", "TEMPO DECORRIDO" },
    { "Decrease scale (-5%%)\nCurrent: %.0f%%", "Diminuir escala (-5%%)\nAtual: %.0f%%" },
    { "Increase scale (+5%%)\nCurrent: %.0f%%", "Aumentar escala (+5%%)\nAtual: %.0f%%" },
    { "Reset time (0:00)", "Reiniciar tempo (0:00)" },
    { "Pause", "Pausar" },
    { "Resume", "Retomar" },
    { "Pause", "Pause" },
    { "Resume", "Resume" },
    { "CURRENT STEP (%d / %d)", "PASSO ATUAL (%d / %d)" },
    { "SUPPLY", "SUPPLY" },
    { "Hide upcoming steps list", "Ocultar lista de proximos passos" },
    { "Show upcoming steps list", "Mostrar lista de proximos passos" },
    { "COMING UP", "PROXIMOS" },
    { "INSERT — settings", "INSERT — configurar" },
    { "No build loaded.", "Nenhuma build carregada." },
    { "%s\n\nClick: sync timer to %s.", "%s\n\nClique: sincronizar timer a %s." },
    { "Sync timer to %s (counting continues from here)", "Sincronizar timer com %s (continua a contar a partir daqui)" },
    // Status messages
    { "OK: %zu steps. Adjust name/race under \"Name, race and save\" and save.", "OK: %zu passos. Ajuste nome/raca em \"Nome, raca e gravacao\" e salve." },
    { "Saved as user/%s.json — you can select it under the Builds tab.", "Guardado como user/%s.json — ja pode escolher no separador Builds." },
    { "JSON copied (%zu bytes).", "JSON copiado (%zu bytes)." },
    { "(no description)", "(sem descricao)" },
};

static_assert(sizeof(kUi) / sizeof(kUi[0]) == static_cast<int>(Tr::Count), "kUi row count");

std::filesystem::path SettingsFilePath()
{
    wchar_t wbuf[512];
    const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", wbuf, 512);
    if (n == 0 || n >= 512)
        return GetExecutableDirectory() / "SC2Hud" / "settings.ini";
    return std::filesystem::path(wbuf) / L"SC2Hud" / L"settings.ini";
}

static void TrimInPlace(std::string& s)
{
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
    size_t iStart = 0;
    while (iStart < s.size() && (s[iStart] == ' ' || s[iStart] == '\t'))
        ++iStart;
    if (iStart > 0)
        s.erase(0, iStart);
}
} // namespace

const char* UiTr(Tr id)
{
    const int i = static_cast<int>(id);
    if (i < 0 || i >= static_cast<int>(Tr::Count))
        return "";
    const Bilingual& b = kUi[static_cast<size_t>(i)];
    return g_lang == UiLang::PortugueseBR ? b.pt : b.en;
}

UiLang UiGetLang()
{
    return g_lang;
}

void UiSetLang(UiLang lang)
{
    g_lang = lang;
}

void UiLanguageLoadFromDisk()
{
    g_lang = UiLang::English;
    const std::filesystem::path path = SettingsFilePath();
    std::ifstream f(path);
    if (!f)
        return;
    std::string line;
    while (std::getline(f, line))
    {
        TrimInPlace(line);
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        if (line.rfind("language=", 0) == 0)
        {
            const std::string v = line.substr(9);
            if (v == "pt_BR")
                g_lang = UiLang::PortugueseBR;
            else
                g_lang = UiLang::English;
        }
    }
}

void UiLanguageSaveToDisk()
{
    const std::filesystem::path path = SettingsFilePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream f(path);
    if (!f)
        return;
    f << "language=" << (g_lang == UiLang::PortugueseBR ? "pt_BR" : "en") << "\n";
}

