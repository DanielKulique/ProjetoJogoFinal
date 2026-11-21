#pragma once

#include "Game.hpp"

#include <cstdint>
#include <iosfwd>
#include <random>
#include <string>
#include <vector>

enum class SkillProfile { Beginner, Intermediate, Advanced };
[[nodiscard]] const char* profileName(SkillProfile profile) noexcept;
[[nodiscard]] SkillProfile parseProfile(const std::string& text);

// Todas as diferenças de habilidade ficam explícitas e ajustáveis.
struct AgentConfig {
    SkillProfile profile{SkillProfile::Intermediate};
    float reactionSeconds{0.22f};
    float aimErrorDegrees{7};
    float movementErrorDegrees{8};
    float movementEfficiency{0.85f};
    float avoidanceWeight{1.2f};
    float avoidanceRadius{140};
    float collectRange{450};
    float preferredDistance{190};
    float leadFraction{0.25f};
    float fireProbability{0.95f};
    float strafeWeight{0.45f};
    [[nodiscard]] static AgentConfig forProfile(SkillProfile profile);
    void validate() const;
};

// Percepcao discreta: somente posicoes/tipos visiveis, HUD e geometria conhecida.
// O comando fica retido entre instantes de reacao; nao observa RNG, IA ou futuros spawns.
class SimulatedAgent {
public:
    explicit SimulatedAgent(AgentConfig config, std::uint32_t seed);
    [[nodiscard]] GameInput input(const Game& game, float dt);
    [[nodiscard]] const AgentConfig& config() const noexcept { return m_config; }
    [[nodiscard]] std::uint64_t observations() const noexcept { return m_observations; }

private:
    struct SeenEnemy { Vector2D position; EnemyKind kind; };
    AgentConfig m_config;
    std::mt19937 m_random;
    float m_reactionRemaining{0};
    GameInput m_held;
    std::vector<SeenEnemy> m_previousEnemies;
    Vector2D m_previousShip;
    int m_strafeSign{1};
    int m_decisions{0};
    std::uint64_t m_observations{0};
    [[nodiscard]] GameInput observeAndDecide(const Game& game);
};

struct SimulationOptions {
    std::uint32_t seed{1};
    SkillProfile profile{SkillProfile::Intermediate};
    double maxSeconds{180};
    int maxRounds{12};
};

struct SimulationResult {
    std::string preset;
    SkillProfile profile{SkillProfile::Intermediate};
    std::uint32_t seed{0};
    double maxSeconds{0};
    int maxRounds{0};
    int roundReached{0};
    double survivalSeconds{0};
    double meanRoundSeconds{0};
    double accuracy{0};
    std::size_t weaponDiversity{0};
    double meanProjectiles{0};
    double wallSeconds{0};
    bool censored{false};
    GameState finalState{GameState::Playing};
    GameStats stats;
};

struct ExperimentPreset {
    std::string name;
    GameConfig config;
};

[[nodiscard]] std::vector<ExperimentPreset> experimentPresets();
[[nodiscard]] SimulationResult runSimulation(GameConfig config, const SimulationOptions& options,
                                              std::string preset = "custom");
void writeSimulationCSVHeader(std::ostream& output);
void writeSimulationCSVRow(std::ostream& output, const SimulationResult& result);
void writeConfigurationCSVHeader(std::ostream& output);
void writeConfigurationCSVRow(std::ostream& output, const ExperimentPreset& preset, SkillProfile profile);
