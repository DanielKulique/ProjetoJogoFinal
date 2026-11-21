#pragma once

#include "Enemy.hpp"

#include <cstddef>

struct WaveConfig {
    int initialEnemies{5};
    int additionalEnemies{2};
    float spawnInterval{2.0f};
    EnemyConfig enemy;
    int healthPerRound{10};
    float speedPerRound{5};
    int maxEnemies{25};
    std::size_t maxActive{8};
    float minSpawnInterval{0.5f};
    float spawnIntervalDecrease{0.1f};
    float advancedChance{0.25f};
    float maxSpeed{220};
};

class WaveManager {
public:
    explicit WaveManager(WaveConfig config = {});
    void update(float dt) noexcept;
    [[nodiscard]] bool readyToSpawn() const noexcept;
    void confirmSpawn() noexcept;
    void startNextRound();
    void reset() noexcept;
    [[nodiscard]] int round() const noexcept { return m_round; }
    [[nodiscard]] int pending() const noexcept { return m_pending; }
    [[nodiscard]] std::size_t remaining(std::size_t alive) const noexcept;
    [[nodiscard]] bool completed(std::size_t alive) const noexcept;
    [[nodiscard]] EnemyConfig enemyConfig(EnemyKind kind = EnemyKind::Normal) const noexcept;
    [[nodiscard]] float spawnInterval() const noexcept;
    [[nodiscard]] const WaveConfig& config() const noexcept { return m_config; }

private:
    friend struct StateCodec;
    WaveConfig m_config;
    int m_round{1};
    int m_pending{0};
    float m_spawnTimer{0};
};
