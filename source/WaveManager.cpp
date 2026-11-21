#include "WaveManager.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cmath>

WaveManager::WaveManager(WaveConfig config) : m_config(config) {
    if (!std::isfinite(config.spawnInterval) || !std::isfinite(config.speedPerRound) ||
        !std::isfinite(config.enemy.speed) || !std::isfinite(config.enemy.attackInterval) ||
        !std::isfinite(config.minSpawnInterval) || !std::isfinite(config.spawnIntervalDecrease) ||
        !std::isfinite(config.advancedChance) || !std::isfinite(config.maxSpeed) ||
        config.initialEnemies <= 0 || config.additionalEnemies < 0 || config.spawnInterval <= 0 ||
        config.maxEnemies < config.initialEnemies || config.maxActive == 0 ||
        config.minSpawnInterval <= 0 || config.spawnIntervalDecrease < 0 ||
        config.advancedChance < 0 || config.advancedChance > 1 || config.maxSpeed <= 0 ||
        config.healthPerRound < 0 || config.speedPerRound < 0 || config.enemy.health <= 0 ||
        config.enemy.speed <= 0 || config.enemy.damage <= 0 || config.enemy.attackInterval <= 0) {
        throw std::invalid_argument("Configuracao de round invalida");
    }
    // Valida tambem os atributos de IA e colisao, sem criar recursos graficos.
    const Enemy validated({}, config.enemy);
    reset();
}

void WaveManager::reset() noexcept {
    m_round = 1;
    m_pending = m_config.initialEnemies;
    m_spawnTimer = 0;
}

void WaveManager::update(float dt) noexcept {
    if (!std::isfinite(dt) || dt <= 0) return;
    m_spawnTimer = std::max(0.0f, m_spawnTimer - dt);
}

bool WaveManager::readyToSpawn() const noexcept { return m_pending > 0 && m_spawnTimer <= EPSILON; }

float WaveManager::spawnInterval() const noexcept {
    const float floor = std::min(m_config.spawnInterval, m_config.minSpawnInterval);
    return std::max(floor, m_config.spawnInterval - (m_round - 1) * m_config.spawnIntervalDecrease);
}

void WaveManager::confirmSpawn() noexcept {
    if (!readyToSpawn()) return;
    --m_pending;
    m_spawnTimer = spawnInterval();
}

void WaveManager::startNextRound() {
    const auto nextCount = static_cast<long long>(m_config.initialEnemies) +
                           static_cast<long long>(m_round) * m_config.additionalEnemies;
    const auto nextHealth = static_cast<long long>(m_config.enemy.health) +
                            static_cast<long long>(m_round) * m_config.healthPerRound;
    if (m_round == std::numeric_limits<int>::max() || nextHealth > std::numeric_limits<int>::max()) {
        throw std::overflow_error("Limite de rounds atingido");
    }
    ++m_round;
    m_pending = static_cast<int>(std::min(static_cast<long long>(m_config.maxEnemies), nextCount));
    m_spawnTimer = 0;
}

std::size_t WaveManager::remaining(std::size_t alive) const noexcept {
    return alive + static_cast<std::size_t>(m_pending);
}

bool WaveManager::completed(std::size_t alive) const noexcept { return remaining(alive) == 0; }

EnemyConfig WaveManager::enemyConfig(EnemyKind kind) const noexcept {
    auto config = m_config.enemy;
    const double multiplier = kind == EnemyKind::Advanced ? config.advancedHealthMultiplier : 1.0;
    const double health = (static_cast<double>(config.health) + (m_round - 1) * static_cast<double>(m_config.healthPerRound)) * multiplier;
    config.health = static_cast<int>(std::clamp(health, 1.0, static_cast<double>(std::numeric_limits<int>::max())));
    const float speedMultiplier = kind == EnemyKind::Advanced ? config.advancedSpeedMultiplier : 1.0f;
    config.speed = std::min(m_config.maxSpeed, (config.speed + (m_round - 1) * m_config.speedPerRound) * speedMultiplier);
    return config;
}
