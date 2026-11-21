#include "Simulation.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
constexpr float dt = 1.0f / 60;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool sameInput(const GameInput& a, const GameInput& b) {
    return a.movement.equals(b.movement) && a.aimTarget.equals(b.aimTarget) && a.fire == b.fire;
}
GameConfig quietConfig() {
    GameConfig config;
    config.asteroid.spawnChance = 0;
    config.gravity.spawnChance = 0;
    return config;
}
void testProfilesAndPerception() {
    const auto beginner = AgentConfig::forProfile(SkillProfile::Beginner);
    const auto intermediate = AgentConfig::forProfile(SkillProfile::Intermediate);
    const auto advanced = AgentConfig::forProfile(SkillProfile::Advanced);
    require(beginner.reactionSeconds > intermediate.reactionSeconds && intermediate.reactionSeconds > advanced.reactionSeconds,
            "Perfis diferem por tempo de reacao");
    require(beginner.aimErrorDegrees > intermediate.aimErrorDegrees && intermediate.aimErrorDegrees > advanced.aimErrorDegrees,
            "Perfis diferem por erro de mira");
    require(beginner.movementEfficiency < advanced.movementEfficiency && beginner.collectRange < advanced.collectRange,
            "Perfis parametrizam movimento e coleta");
    Game game(TileMap(TileMap::Matrix(23, std::string(31, '.'))), quietConfig());
    game.update(dt, {});
    require(!game.enemies().empty(), "Fixture real nasce pelo agendamento do Game");
    SimulatedAgent agent(beginner, 25);
    const auto initial = agent.input(game, dt);
    require(agent.observations() == 1, "Primeira observacao");
    // Entidade muda visivelmente, mas o bot so pode reagir no proximo instante de percepcao.
    game.enemies().front()->body.position += Vector2D{150, 150};
    game.player().body.position += Vector2D{50, 0};
    require(sameInput(initial, agent.input(game, dt)) && agent.observations() == 1,
            "Comando retido nao consulta estado novo entre reacoes");
    for (int i = 0; i < 30; ++i) (void)agent.input(game, dt);
    require(agent.observations() >= 2, "Agente volta a perceber apos intervalo configurado");
    SimulatedAgent first(advanced, 123), second(advanced, 123);
    for (int i = 0; i < 100; ++i)
        require(sameInput(first.input(game, dt), second.input(game, dt)), "Mesmas observacoes/semente reproduzem comandos");
    auto invalid = beginner;
    invalid.reactionSeconds = 0;
    bool rejected = false;
    try { SimulatedAgent agentInvalid(invalid, 1); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Perfil invalido rejeitado");
}

void testRealGameReproduction() {
    SimulationOptions options;
    options.seed = 47;
    options.maxSeconds = 12;
    options.profile = SkillProfile::Advanced;
    auto a = runSimulation(quietConfig(), options, "test");
    auto b = runSimulation(quietConfig(), options, "test");
    require(a.stats.shotsFired > 0 && a.stats.peakProjectiles > 0, "Agente controla tiros do Game real");
    require(a.survivalSeconds > 0 && a.survivalSeconds <= options.maxSeconds + dt, "Horizonte real respeitado");
    require(a.accuracy >= 0 && a.accuracy <= 1, "Precisao entre zero e um");
    require(a.stats.enginesLost == a.stats.damageReceived, "Metricas registram dano em motores");
    require(a.stats.damageDealt > 0, "Agente causa dano sem alterar vidas artificialmente");
    a.wallSeconds = b.wallSeconds = 0; // Tempo da CPU e deliberadamente nao deterministico.
    std::ostringstream first, second;
    writeSimulationCSVRow(first, a);
    writeSimulationCSVRow(second, b);
    require(first.str() == second.str(), "Experimento reproduz todos os resultados de jogo pela semente");
    std::ostringstream csv;
    writeSimulationCSVHeader(csv);
    writeSimulationCSVRow(csv, a);
    require(csv.str().find("wall_seconds") != std::string::npos && csv.str().find("circle_damage_per_shot") != std::string::npos,
            "CSV inclui custo computacional e eficiencia por padrao");
}

void testCompletedRoundHorizon() {
    SimulationOptions options;
    options.seed = 47;
    options.profile = SkillProfile::Advanced;
    options.maxSeconds = 180;
    options.maxRounds = 1;
    const auto result = runSimulation(quietConfig(), options, "round_cap");
    require(result.stats.roundsCompleted == 1 && result.roundReached == 1 &&
            result.finalState == GameState::RoundCompleted && result.censored,
            "Limite de rounds para na conclusao do ultimo round, antes do portal seguinte");
    require(result.stats.roundDurations.size() == 1 && result.survivalSeconds < options.maxSeconds,
            "Horizonte de rounds registra uma conclusao e interrompe antes do tempo maximo");
}

void testRecommendedConfigurationMatchesValidatedCandidate() {
    const auto recommended = GameConfig::recommended();
    require(recommended.drops.chance == 0.90f && recommended.drops.upgradeChance == 1.0f - 0.60f &&
            recommended.waves.advancedChance == 0.30f && recommended.waves.healthPerRound == 18 &&
            recommended.waves.spawnInterval == 1.6f && recommended.weapon.progressionStrength == 1.2f &&
            recommended.asteroid.spawnChance == 0.15f && recommended.gravity.accelerationMeters == 12 &&
            recommended.pickup.lifetime == 35 && !recommended.weapon.legacyProgression &&
            recommended.drops.avoidFullEngineDrops,
            "Factory recomendada conserva exatamente os parametros do candidato growth18");
    GameConfig expected;
    bool found = false;
    for (const auto& preset : experimentPresets()) if (preset.name == "pressure") {
        expected = preset.config;
        found = true;
    }
    require(found, "Preset de origem do candidato validado existe");
    expected.drops.chance = 0.90f;
    expected.drops.upgradeChance = 1.0f - 0.60f; // Mesma operacao do CLI --engine-chance .60.
    expected.waves.advancedChance = 0.30f;
    expected.waves.healthPerRound = 18;
    expected.pickup.lifetime = 35;
    std::ostringstream configA, configB;
    writeConfigurationCSVRow(configA, {"growth18", recommended}, SkillProfile::Advanced);
    writeConfigurationCSVRow(configB, {"growth18", expected}, SkillProfile::Advanced);
    require(configA.str() == configB.str(), "Factory e overrides experimentais exportam os mesmos parametros");
    SimulationOptions options;
    options.seed = 47;
    options.profile = SkillProfile::Advanced;
    options.maxSeconds = 12;
    auto a = runSimulation(recommended, options, "growth18");
    auto b = runSimulation(expected, options, "growth18");
    a.wallSeconds = b.wallSeconds = 0;
    std::ostringstream resultA, resultB;
    writeSimulationCSVRow(resultA, a);
    writeSimulationCSVRow(resultB, b);
    require(resultA.str() == resultB.str() && a.finalState == b.finalState,
            "Default real reproduz todos os resultados deterministicos do candidato validado");
}

void testPresetsAndBounds() {
    const auto presets = experimentPresets();
    require(presets.size() == 8 && presets.front().name == "baseline" && presets.front().config.weapon.legacyProgression,
            "Baseline conserva sistema antigo para comparacao");
    require(!presets[1].config.weapon.legacyProgression && presets[1].config.drops.avoidFullEngineDrops,
            "Baseline procedural distingue nova progressao e evita reparo inutil");
    require(presets.front().config.drops.chance == 0.75f && presets.front().config.drops.upgradeChance == 0.8f &&
            !presets.front().config.drops.avoidFullEngineDrops,
            "Baseline conserva probabilidades originais e reparos cheios");
    SimulationOptions options;
    options.maxSeconds = 1;
    for (const auto profile : {SkillProfile::Beginner, SkillProfile::Intermediate, SkillProfile::Advanced}) {
        options.profile = profile;
        const auto result = runSimulation(presets[1].config, options, presets[1].name);
        require(result.stats.peakProjectiles <= presets[1].config.maxProjectiles, "Limite de projeteis durante simulacao real");
    }
    options.maxSeconds = 0;
    bool rejected = false;
    try { (void)runSimulation(quietConfig(), options); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Simulacao exige horizonte positivo");
    std::ostringstream configuration;
    writeConfigurationCSVHeader(configuration);
    writeConfigurationCSVRow(configuration, presets.front(), SkillProfile::Beginner);
    require(configuration.str().find("reaction_seconds") != std::string::npos &&
            configuration.str().find("legacy_progression") != std::string::npos,
            "Configuracoes experimentais incluem agente e regras");
}
}
int main() {
    try {
        testProfilesAndPerception();
        testRealGameReproduction();
        testPresetsAndBounds();
        testCompletedRoundHorizon();
        testRecommendedConfigurationMatchesValidatedCandidate();
        std::cout << "Agentes discretos, simulacao real e reproducao experimental: OK\n";
    } catch (const std::exception& error) {
        std::cerr << "FALHA: " << error.what() << '\n';
        return 1;
    }
}
