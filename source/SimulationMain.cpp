#include "Simulation.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {
constexpr const char* usage =
    "Uso: space_arena_sim [--preset NOME[,NOME]|all] [--profile beginner|intermediate|advanced|all]\n"
    "  [--seed-start N] [--runs N] [--seconds S] [--max-rounds N]\n"
    "  [--output RESULTADOS.csv] [--configs-output CONFIGURACOES.csv] [--configs-only]\n"
    "  [--label IDENTIFICADOR] (exige exatamente um preset)\n"
    "Presets: baseline,procedural_baseline,repair,balanced,gentle,pressure,variety,environment.\n"
    "Padrao: todos os presets/perfis; 3 sementes desde1; 180s; 12rounds; CSV em stdout.\n"
    "Overrides: --drop P --engine-chance P --advanced-chance P --health-growth N\n"
    "  --spawn-interval S --progression-strength X --asteroid-chance P --gravity-strength X\n"
    "  --pickup-lifetime S\n"
    "O simulador usa Game real a1/60s, sem janela, com agentes de percepcao discreta.\n";

std::uint32_t readUnsigned(const std::string& value) {
    std::uint32_t result{};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || end != value.data() + value.size())
        throw std::invalid_argument("Inteiro sem sinal invalido: " + value);
    return result;
}
double readReal(const std::string& value) {
    std::size_t used = 0;
    const double result = std::stod(value, &used);
    if (used != value.size() || !std::isfinite(result)) throw std::invalid_argument("Numero real invalido: " + value);
    return result;
}
std::vector<std::string> split(const std::string& text) {
    if (text.empty() || text.front() == ',' || text.back() == ',')
        throw std::invalid_argument("Lista com item vazio");
    std::vector<std::string> result;
    std::istringstream stream(text);
    std::string item;
    while (std::getline(stream, item, ',')) {
        if (item.empty()) throw std::invalid_argument("Lista com item vazio");
        result.push_back(item);
    }
    if (result.empty()) throw std::invalid_argument("Lista vazia");
    return result;
}
void validateLabel(const std::string& label) {
    if (label.empty()) throw std::invalid_argument("Label vazio");
    for (const char character : label) {
        if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9') || character == '_' || character == '-'))
            throw std::invalid_argument("Label deve usar apenas letras ASCII, numeros, _ e -");
    }
}
void openOutput(std::ofstream& output, const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    output.open(path, std::ios::trunc);
    if (!output) throw std::runtime_error("Nao foi possivel escrever: " + path);
}
}

int main(int argc, char** argv) {
    try {
        std::string presetSelection = "all", profileSelection = "all", outputPath, configsPath, label;
        bool labelProvided = false;
        std::uint32_t firstSeed = 1, runs = 3;
        SimulationOptions options;
        bool configsOnly = false;
        std::vector<std::function<void(GameConfig&)>> overrides;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--help") { std::cout << usage; return 0; }
            if (argument == "--configs-only") { configsOnly = true; continue; }
            if (i + 1 >= argc) throw std::invalid_argument("Argumento sem valor: " + argument);
            const std::string value = argv[++i];
            if (argument == "--preset") presetSelection = value;
            else if (argument == "--label") {
                validateLabel(value);
                label = value;
                labelProvided = true;
            }
            else if (argument == "--profile") profileSelection = value;
            else if (argument == "--seed-start") firstSeed = readUnsigned(value);
            else if (argument == "--runs") runs = readUnsigned(value);
            else if (argument == "--seconds") options.maxSeconds = readReal(value);
            else if (argument == "--max-rounds") {
                const auto count = readUnsigned(value);
                if (count > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) throw std::invalid_argument("Limite de rounds excessivo");
                options.maxRounds = static_cast<int>(count);
            } else if (argument == "--output") outputPath = value;
            else if (argument == "--configs-output") configsPath = value;
            else if (argument == "--health-growth") {
                const auto amount = readUnsigned(value);
                if (amount > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) throw std::invalid_argument("Incremento de vida excessivo");
                overrides.push_back([amount](GameConfig& c) { c.waves.healthPerRound = static_cast<int>(amount); });
            } else {
                const auto amount = static_cast<float>(readReal(value));
                if (argument == "--drop") overrides.push_back([amount](GameConfig& c) { c.drops.chance = amount; });
                else if (argument == "--engine-chance") overrides.push_back([amount](GameConfig& c) { c.drops.upgradeChance = 1 - amount; });
                else if (argument == "--advanced-chance") overrides.push_back([amount](GameConfig& c) { c.waves.advancedChance = amount; });
                else if (argument == "--spawn-interval") overrides.push_back([amount](GameConfig& c) { c.waves.spawnInterval = amount; });
                else if (argument == "--progression-strength") overrides.push_back([amount](GameConfig& c) { c.weapon.progressionStrength = amount; });
                else if (argument == "--asteroid-chance") overrides.push_back([amount](GameConfig& c) { c.asteroid.spawnChance = amount; });
                else if (argument == "--gravity-strength") overrides.push_back([amount](GameConfig& c) { c.gravity.accelerationMeters = amount; });
                else if (argument == "--pickup-lifetime") overrides.push_back([amount](GameConfig& c) { c.pickup.lifetime = amount; });
                else throw std::invalid_argument("Argumento desconhecido: " + argument);
            }
        }
        if (runs == 0 || runs > 100000 || static_cast<std::uint64_t>(firstSeed) + runs - 1 > std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument("Faixa de sementes invalida");
        if (options.maxSeconds <= 0 || options.maxSeconds > 86400 || options.maxRounds <= 0)
            throw std::invalid_argument("Horizonte de simulacao invalido");
        if (!outputPath.empty() && !configsPath.empty()) {
            bool sameFile = std::filesystem::weakly_canonical(outputPath) ==
                            std::filesystem::weakly_canonical(configsPath);
            if (!sameFile && std::filesystem::exists(outputPath) && std::filesystem::exists(configsPath))
                sameFile = std::filesystem::equivalent(outputPath, configsPath);
            if (sameFile) throw std::invalid_argument("CSV de resultados e configuracoes devem ser diferentes");
        }
        if (configsOnly && configsPath.empty()) throw std::invalid_argument("--configs-only exige --configs-output");

        const auto allPresets = experimentPresets();
        std::vector<ExperimentPreset> presets;
        if (presetSelection == "all") presets = allPresets;
        else for (const auto& name : split(presetSelection)) {
            bool found = false;
            for (const auto& preset : allPresets) if (preset.name == name) {
                presets.push_back(preset);
                found = true;
                break;
            }
            if (!found) throw std::invalid_argument("Preset desconhecido: " + name);
        }
        if (labelProvided) {
            if (presets.size() != 1) throw std::invalid_argument("--label exige exatamente um preset selecionado");
            presets.front().name = label;
        }
        std::vector<SkillProfile> profiles;
        if (profileSelection == "all") profiles = {SkillProfile::Beginner, SkillProfile::Intermediate, SkillProfile::Advanced};
        else for (const auto& name : split(profileSelection)) profiles.push_back(parseProfile(name));
        for (auto& preset : presets) {
            for (const auto& override : overrides) override(preset.config);
            preset.config.validate();
        }
        std::ofstream configsFile;
        if (!configsPath.empty()) {
            openOutput(configsFile, configsPath);
            writeConfigurationCSVHeader(configsFile);
            for (const auto& preset : presets) for (const auto profile : profiles)
                writeConfigurationCSVRow(configsFile, preset, profile);
            configsFile.close();
            if (!configsFile) throw std::runtime_error("Falha ao escrever configuracoes");
        }
        if (configsOnly) return 0;
        std::ofstream resultsFile;
        if (!outputPath.empty()) openOutput(resultsFile, outputPath);
        std::ostream& output = outputPath.empty() ? std::cout : resultsFile;
        writeSimulationCSVHeader(output);
        const auto total = presets.size() * profiles.size() * runs;
        std::size_t completed = 0;
        for (const auto& preset : presets) for (const auto profile : profiles) {
            options.profile = profile;
            for (std::uint32_t index = 0; index < runs; ++index) {
                options.seed = firstSeed + index;
                writeSimulationCSVRow(output, runSimulation(preset.config, options, preset.name));
                if (!output) throw std::runtime_error("Falha ao escrever resultados");
                output.flush();
                ++completed;
                if (completed % 10 == 0 || completed == total)
                    std::cerr << "Simulacoes: " << completed << '/' << total << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "ERRO: " << error.what() << '\n' << usage;
        return 1;
    }
    return 0;
}
