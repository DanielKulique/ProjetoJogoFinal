#include "Simulation.hpp"
#include "Random.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace {
constexpr float pi = 3.14159265358979323846f;
constexpr float simulationDt = 1.0f / 60.0f;
constexpr std::array<const char*, 8> patternNames{{"standard", "burst", "heavy", "circle", "triangle", "square", "spread", "parallel"}};
Vector2D safeUnit(Vector2D vector) {
    return vector.length_squared() > EPSILON * EPSILON ? vector.normalized() : Vector2D{};
}
Vector2D rotated(Vector2D vector, float angleDegrees) {
    const float angle = angleDegrees * pi / 180;
    return {vector.x * std::cos(angle) - vector.y * std::sin(angle),
            vector.x * std::sin(angle) + vector.y * std::cos(angle)};
}
float symmetric(std::mt19937& random, float extent) {
    return static_cast<float>((2 * randomUnit(random) - 1) * extent);
}
bool inUnit(float value) { return std::isfinite(value) && value >= 0 && value <= 1; }
}

const char* profileName(SkillProfile profile) noexcept {
    switch (profile) {
    case SkillProfile::Beginner: return "beginner";
    case SkillProfile::Intermediate: return "intermediate";
    case SkillProfile::Advanced: return "advanced";
    }
    return "unknown";
}

SkillProfile parseProfile(const std::string& text) {
    if (text == "beginner") return SkillProfile::Beginner;
    if (text == "intermediate") return SkillProfile::Intermediate;
    if (text == "advanced") return SkillProfile::Advanced;
    throw std::invalid_argument("Perfil invalido: " + text);
}

AgentConfig AgentConfig::forProfile(SkillProfile profile) {
    AgentConfig config;
    config.profile = profile;
    if (profile == SkillProfile::Beginner) {
        config.reactionSeconds = 0.50f;
        config.aimErrorDegrees = 23;
        config.movementErrorDegrees = 25;
        config.movementEfficiency = 0.58f;
        config.avoidanceWeight = 0.3f;
        config.avoidanceRadius = 65;
        config.collectRange = 150;
        config.preferredDistance = 80;
        config.leadFraction = 0;
        config.fireProbability = 0.85f;
        config.strafeWeight = 0.1f;
    } else if (profile == SkillProfile::Advanced) {
        config.reactionSeconds = 0.09f;
        config.aimErrorDegrees = 1.5f;
        config.movementErrorDegrees = 1.5f;
        config.movementEfficiency = 0.99f;
        config.avoidanceWeight = 2.2f;
        config.avoidanceRadius = 210;
        config.collectRange = 1800;
        config.preferredDistance = 270;
        config.leadFraction = 0.8f;
        config.fireProbability = 1;
        config.strafeWeight = 0.7f;
    }
    config.validate();
    return config;
}

void AgentConfig::validate() const {
    if (!std::isfinite(reactionSeconds) || reactionSeconds <= 0 ||
        !std::isfinite(aimErrorDegrees) || aimErrorDegrees < 0 || aimErrorDegrees > 180 ||
        !std::isfinite(movementErrorDegrees) || movementErrorDegrees < 0 || movementErrorDegrees > 180 ||
        !inUnit(movementEfficiency) || !inUnit(leadFraction) || !inUnit(fireProbability) ||
        !std::isfinite(avoidanceWeight) || avoidanceWeight < 0 ||
        !std::isfinite(avoidanceRadius) || avoidanceRadius <= 0 ||
        !std::isfinite(collectRange) || collectRange < 0 ||
        !std::isfinite(preferredDistance) || preferredDistance <= 0 ||
        !std::isfinite(strafeWeight) || strafeWeight < 0 ||
        (profile != SkillProfile::Beginner && profile != SkillProfile::Intermediate && profile != SkillProfile::Advanced)) {
        throw std::invalid_argument("Configuracao de agente invalida");
    }
}

SimulatedAgent::SimulatedAgent(AgentConfig config, std::uint32_t seed)
    : m_config(config), m_random(seed) {
    m_config.validate();
    m_strafeSign = randomChance(m_random, 0.5f) ? 1 : -1;
}

GameInput SimulatedAgent::input(const Game& game, float dt) {
    if (!std::isfinite(dt) || dt <= 0) throw std::invalid_argument("Passo do agente invalido");
    m_reactionRemaining -= dt;
    if (m_reactionRemaining <= EPSILON) {
        m_held = observeAndDecide(game);
        m_reactionRemaining += m_config.reactionSeconds;
        if (m_reactionRemaining <= 0) m_reactionRemaining = m_config.reactionSeconds;
    }
    return m_held;
}

GameInput SimulatedAgent::observeAndDecide(const Game& game) {
    ++m_observations;
    ++m_decisions;
    const auto ship = game.player().body.position;
    GameInput command;
    command.aimTarget = ship + game.player().aimDirection();
    if (game.state() == GameState::GameOver) return command;

    std::vector<SeenEnemy> seen;
    seen.reserve(game.enemies().size());
    const Enemy* target = nullptr;
    float targetScore = std::numeric_limits<float>::max();
    for (const auto& enemy : game.enemies()) {
        if (!enemy->active) continue;
        seen.push_back({enemy->body.position, enemy->kind()});
        float score = (enemy->body.position - ship).length();
        if (m_config.profile == SkillProfile::Advanced && enemy->kind() == EnemyKind::Advanced) score *= 0.65f;
        if (score < targetScore) { targetScore = score; target = enemy.get(); }
    }
    if (m_decisions % 30 == 0 && randomChance(m_random, 0.25f)) m_strafeSign = -m_strafeSign;

    Vector2D desired;
    if (game.state() == GameState::RoundCompleted && game.portal().active) {
        const auto portal = game.portal().body.position;
        if (game.portal().waitingForExit()) {
            desired = ship - portal;
            if (desired.length_squared() < 1) desired = {static_cast<float>(m_strafeSign), 0};
        } else desired = portal - ship;
    } else {
        if (target) {
            const auto relative = target->body.position - ship;
            const float distance = relative.length();
            const auto direction = safeUnit(relative);
            float preferred = m_config.preferredDistance;
            // Usa apenas a configuracao atual exibida ao jogador, nunca os desbloqueios futuros.
            if (m_config.profile == SkillProfile::Advanced && game.player().weapon().pattern() == ProjectilePattern::Circle)
                preferred *= 0.7f;
            const float approach = std::clamp((distance - preferred) / std::max(40.0f, preferred), -1.0f, 1.0f);
            desired = direction * approach + Vector2D{-direction.y, direction.x} *
                (m_config.strafeWeight * m_strafeSign);

            Vector2D estimatedVelocity;
            float matchDistance = 180;
            for (const auto& previous : m_previousEnemies) {
                if (previous.kind != target->kind()) continue;
                const float difference = (target->body.position - previous.position).length();
                if (difference < matchDistance) {
                    matchDistance = difference;
                    estimatedVelocity = (target->body.position - previous.position) / m_config.reactionSeconds;
                }
            }
            const float estimatedFlight = distance / game.player().weapon().effectiveStats().projectileSpeed;
            auto aim = relative + estimatedVelocity * (estimatedFlight * m_config.leadFraction);
            if (aim.length_squared() < 1) aim = {1, 0};
            aim = rotated(aim, symmetric(m_random, m_config.aimErrorDegrees));
            command.aimTarget = ship + aim;
            command.fire = randomChance(m_random, m_config.fireProbability);
        } else {
            // Enquanto espera novos aliens, evita ficar parado no local do ultimo impacto.
            const auto center = game.initialCenter();
            desired = safeUnit(center - ship) * 0.5f;
        }

        const Pickup* goal = nullptr;
        float bestPickup = std::numeric_limits<float>::max();
        for (const auto& pickup : game.pickups()) {
            if (!pickup.active || (pickup.kind() == PickupKind::Engine && game.player().engines() == 4)) continue;
            const float distance = (pickup.body.position - ship).length();
            if (distance > m_config.collectRange) continue;
            float score = distance;
            if (pickup.kind() == PickupKind::Engine && game.player().engines() < 3) score *= 0.25f;
            if (score < bestPickup) { bestPickup = score; goal = &pickup; }
        }
        if (goal) desired = safeUnit(goal->body.position - ship) * 1.25f;
    }

    // Repulsao usa unicamente corpos vistos no instante de reacao.
    for (const auto& enemy : seen) {
        const auto away = ship - enemy.position;
        const float distance = away.length();
        const float radius = m_config.avoidanceRadius *
            (enemy.kind == EnemyKind::Advanced && m_config.profile == SkillProfile::Advanced ? 1.2f : 1.0f);
        if (distance < radius) desired += safeUnit(away) *
            (m_config.avoidanceWeight * (1.0f - distance / radius));
    }
    for (const auto& rock : game.asteroids()) {
        if (!rock.active) continue;
        const auto away = ship - rock.body.position;
        const float distance = away.length();
        const float radius = m_config.avoidanceRadius + rock.radius();
        if (distance < radius) desired += safeUnit(away) *
            (m_config.avoidanceWeight * 1.2f * (1.0f - distance / radius));
        if (!target && distance < 450) {
            const auto aim = rotated(rock.body.position - ship, symmetric(m_random, m_config.aimErrorDegrees));
            command.aimTarget = ship + aim;
            command.fire = randomChance(m_random, m_config.fireProbability);
        }
    }
    if (game.gravityField() && m_config.profile != SkillProfile::Beginner) {
        const auto& field = *game.gravityField();
        const auto away = ship - field.body.position;
        const float distance = away.length();
        if (distance < field.radius() * 0.6f)
            desired += safeUnit(away) * (m_config.avoidanceWeight * 0.45f);
    }

    const float width = game.map().width() * TILE_SIZE, height = game.map().height() * TILE_SIZE;
    constexpr float margin = 60;
    if (ship.x < margin) desired.x += (margin - ship.x) / margin * 2;
    if (ship.x > width - margin) desired.x -= (ship.x - width + margin) / margin * 2;
    if (ship.y < margin) desired.y += (margin - ship.y) / margin * 2;
    if (ship.y > height - margin) desired.y -= (ship.y - height + margin) / margin * 2;
    if (m_decisions > 1 && (ship - m_previousShip).length_squared() < 2 && desired.length_squared() > 0.1f)
        desired = rotated(desired, static_cast<float>(m_strafeSign) * 70);

    desired = rotated(desired, symmetric(m_random, m_config.movementErrorDegrees));
    if (desired.length_squared() > EPSILON * EPSILON) {
        // Mapa/corpo sao visiveis: rejeita apenas comandos que entram na parede proxima.
        const auto next = ship + safeUnit(desired) * 32;
        if (!game.map().isAreaFree(game.player().collision.bounds(next))) {
            for (const float angle : {90.0f, -90.0f, 180.0f}) {
                const auto alternative = rotated(desired, angle);
                const auto position = ship + safeUnit(alternative) * 32;
                if (game.map().isAreaFree(game.player().collision.bounds(position))) {
                    desired = alternative;
                    break;
                }
            }
        }
    }
    command.movement = randomChance(m_random, m_config.movementEfficiency) ? safeUnit(desired) : Vector2D{};
    m_previousShip = ship;
    m_previousEnemies = std::move(seen);
    return command;
}

std::vector<ExperimentPreset> experimentPresets() {
    GameConfig baseline;
    baseline.weapon.legacyProgression = true;
    baseline.drops.avoidFullEngineDrops = false;
    std::vector<ExperimentPreset> presets{{"baseline", baseline}};
    auto procedural = baseline;
    procedural.weapon.legacyProgression = false;
    procedural.drops.avoidFullEngineDrops = true;
    presets.push_back({"procedural_baseline", procedural});
    auto repair = procedural;
    repair.drops.chance = 0.85f;
    repair.drops.upgradeChance = 0.60f;
    repair.waves.advancedChance = 0.30f;
    repair.waves.healthPerRound = 8;
    presets.push_back({"repair", repair});
    auto balanced = repair;
    balanced.drops.upgradeChance = 0.65f;
    balanced.waves.healthPerRound = 6;
    balanced.waves.spawnInterval = 1.9f;
    balanced.weapon.progressionStrength = 1.15f;
    presets.push_back({"balanced", balanced});
    auto gentle = balanced;
    gentle.drops.chance = 0.90f;
    gentle.drops.upgradeChance = 0.60f;
    gentle.waves.advancedChance = 0.35f;
    gentle.waves.healthPerRound = 4;
    gentle.waves.spawnInterval = 2.2f;
    gentle.asteroid.spawnChance = 0.07f;
    gentle.gravity.accelerationMeters = 8;
    presets.push_back({"gentle", gentle});
    auto pressure = procedural;
    pressure.drops.chance = 0.80f;
    pressure.drops.upgradeChance = 0.70f;
    pressure.waves.spawnInterval = 1.6f;
    pressure.weapon.progressionStrength = 1.2f;
    pressure.asteroid.spawnChance = 0.15f;
    pressure.gravity.accelerationMeters = 12;
    presets.push_back({"pressure", pressure});
    auto variety = repair;
    variety.drops.chance = 0.80f;
    variety.drops.upgradeChance = 0.72f;
    variety.waves.advancedChance = 0.40f;
    variety.waves.spawnInterval = 1.8f;
    variety.weapon.progressionStrength = 1.1f;
    presets.push_back({"variety", variety});
    auto environment = balanced;
    environment.asteroid.spawnChance = 0.20f;
    environment.gravity.accelerationMeters = 15;
    presets.push_back({"environment", environment});
    for (const auto& preset : presets) preset.config.validate();
    return presets;
}

SimulationResult runSimulation(GameConfig config, const SimulationOptions& options, std::string preset) {
    if (!std::isfinite(options.maxSeconds) || options.maxSeconds <= 0 || options.maxSeconds > 86400 || options.maxRounds <= 0)
        throw std::invalid_argument("Horizonte de simulacao invalido");
    config.seed = options.seed;
    config.validate();
    const auto start = std::chrono::steady_clock::now();
    Game game(TileMap(TileMap::Matrix(23, std::string(31, EMPTY_TILE))), config);
    const auto agentConfig = AgentConfig::forProfile(options.profile);
    SimulatedAgent agent(agentConfig, options.seed ^ (0x9e3779b9u + static_cast<std::uint32_t>(options.profile) * 101u));
    const auto maxSteps = static_cast<std::uint64_t>(std::ceil(options.maxSeconds * 60));
    double projectileSamples = 0;
    std::uint64_t steps = 0;
    for (; steps < maxSteps && game.state() != GameState::GameOver &&
           game.stats().roundsCompleted < static_cast<std::uint64_t>(options.maxRounds); ++steps) {
        game.update(simulationDt, agent.input(game, simulationDt));
        projectileSamples += game.projectiles().size();
    }
    SimulationResult result;
    result.preset = std::move(preset);
    result.profile = options.profile;
    result.seed = options.seed;
    result.maxSeconds = options.maxSeconds;
    result.maxRounds = options.maxRounds;
    result.roundReached = game.waves().round();
    result.survivalSeconds = game.gameplayTime();
    result.stats = game.stats();
    if (!result.stats.roundDurations.empty()) result.meanRoundSeconds =
        std::accumulate(result.stats.roundDurations.begin(), result.stats.roundDurations.end(), 0.0) /
        result.stats.roundDurations.size();
    result.accuracy = result.stats.shotsFired ? static_cast<double>(result.stats.projectileHits) / result.stats.shotsFired : 0;
    for (std::size_t i = 0; i < 8; ++i) if (result.stats.perPatternShots[i]) ++result.weaponDiversity;
    result.meanProjectiles = steps ? projectileSamples / steps : 0;
    result.wallSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    result.censored = game.state() != GameState::GameOver;
    result.finalState = game.state();
    return result;
}

void writeSimulationCSVHeader(std::ostream& output) {
    output << "preset,profile,seed,max_seconds,max_rounds,round_reached,rounds_completed,survival_seconds,mean_round_seconds,enemy_kills,player_kills,shots_fired,projectile_hits,accuracy,damage_dealt,asteroid_damage_dealt,damage_received,engines_lost,engines_recovered,upgrades_collected,upgrade_rate_per_minute,weapon_diversity,pattern_mask,peak_projectiles,mean_projectiles,wall_seconds,censored,round_durations";
    for (const auto* pattern : patternNames)
        output << ',' << pattern << "_shots," << pattern << "_hits," << pattern << "_damage," << pattern << "_accuracy," << pattern << "_damage_per_shot";
    output << '\n';
}

void writeSimulationCSVRow(std::ostream& output, const SimulationResult& result) {
    const auto& s = result.stats;
    output << std::setprecision(10) << result.preset << ',' << profileName(result.profile) << ',' << result.seed << ','
        << result.maxSeconds << ',' << result.maxRounds << ',' << result.roundReached << ',' << s.roundsCompleted << ','
        << result.survivalSeconds << ',' << result.meanRoundSeconds << ',' << s.enemyKills << ',' << s.playerKills << ','
        << s.shotsFired << ',' << s.projectileHits << ',' << result.accuracy << ',' << s.damageDealt << ',' << s.asteroidDamageDealt << ','
        << s.damageReceived << ',' << s.enginesLost << ',' << s.enginesRecovered << ',' << s.upgradesCollected << ','
        << (result.survivalSeconds > 0 ? s.upgradesCollected * 60.0 / result.survivalSeconds : 0) << ','
        << result.weaponDiversity << ',' << s.patternMask << ',' << s.peakProjectiles << ',' << result.meanProjectiles << ','
        << result.wallSeconds << ',' << (result.censored ? 1 : 0) << ",\"";
    for (std::size_t i = 0; i < s.roundDurations.size(); ++i) {
        if (i) output << ';';
        output << s.roundDurations[i];
    }
    output << '"';
    for (std::size_t i = 0; i < 8; ++i) {
        const auto shots = s.perPatternShots[i];
        output << ',' << shots << ',' << s.perPatternHits[i] << ',' << s.perPatternDamage[i] << ','
            << (shots ? static_cast<double>(s.perPatternHits[i]) / shots : 0) << ','
            << (shots ? static_cast<double>(s.perPatternDamage[i]) / shots : 0);
    }
    output << '\n';
}

void writeConfigurationCSVHeader(std::ostream& output) {
    output << "preset,profile,legacy_progression,drop_chance,engine_conditional_chance,upgrade_conditional_chance,avoid_full_engine_drops,advanced_chance,health_per_round,spawn_interval,min_spawn_interval,spawn_interval_decrease,max_active_enemies,max_enemies,progression_strength,asteroid_chance,asteroid_interval,gravity_chance,gravity_interval,gravity_acceleration_meters,gravity_duration,pickup_lifetime,max_projectiles,reaction_seconds,aim_error_degrees,movement_error_degrees,movement_efficiency,avoidance_weight,avoidance_radius,collect_range,preferred_distance,lead_fraction,fire_probability,strafe_weight\n";
}

void writeConfigurationCSVRow(std::ostream& output, const ExperimentPreset& preset, SkillProfile profile) {
    const auto& c = preset.config;
    const auto agent = AgentConfig::forProfile(profile);
    output << std::setprecision(10) << preset.name << ',' << profileName(profile) << ',' << c.weapon.legacyProgression << ','
        << c.drops.chance << ',' << 1.0f - c.drops.upgradeChance << ',' << c.drops.upgradeChance << ',' << c.drops.avoidFullEngineDrops << ','
        << c.waves.advancedChance << ',' << c.waves.healthPerRound << ',' << c.waves.spawnInterval << ','
        << c.waves.minSpawnInterval << ',' << c.waves.spawnIntervalDecrease << ',' << c.waves.maxActive << ',' << c.waves.maxEnemies << ','
        << c.weapon.progressionStrength << ',' << c.asteroid.spawnChance << ',' << c.asteroid.spawnInterval << ','
        << c.gravity.spawnChance << ',' << c.gravity.spawnInterval << ',' << c.gravity.accelerationMeters << ',' << c.gravity.duration << ','
        << c.pickup.lifetime << ',' << c.maxProjectiles << ',' << agent.reactionSeconds << ',' << agent.aimErrorDegrees << ',' << agent.movementErrorDegrees << ','
        << agent.movementEfficiency << ',' << agent.avoidanceWeight << ',' << agent.avoidanceRadius << ',' << agent.collectRange << ','
        << agent.preferredDistance << ',' << agent.leadFraction << ',' << agent.fireProbability << ',' << agent.strafeWeight << '\n';
}
