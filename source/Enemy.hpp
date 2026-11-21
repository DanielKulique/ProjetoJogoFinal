#pragma once

#include "NPC.hpp"
#include "TileMap.hpp"

#include <cstdint>
#include <random>
#include <vector>

enum class EnemyKind { Normal, Advanced };
enum class EnemyState { Wandering, Chasing };

struct EnemyConfig {
    int health{50};
    float speed{90};
    int damage{1};
    float attackInterval{0.8f};
    float detectionRadius{220};
    float lossRadius{280};
    float wanderInterval{2};
    float wanderRadius{300};
    float wanderSpeedFactor{0.65f};
    float normalHalfExtent{18};
    float advancedHalfExtent{24};
    float explosionRadius{110};
    float advancedHealthMultiplier{1.5f};
    float advancedSpeedMultiplier{1.1f};
    float driftDamping{2};
    float maxDrift{100};
};

class Enemy : public NPC {
public:
    Enemy(Vector2D position, EnemyConfig config = {}, EnemyKind kind = EnemyKind::Normal,
          std::uint32_t seed = 0);
    void update(float dt, Vector2D target, const TileMap* map = nullptr);
    void takeDamage(int amount) noexcept;
    [[nodiscard]] int tryAttack(const Entity& target) noexcept;
    [[nodiscard]] int health() const noexcept { return m_health; }
    [[nodiscard]] int maxHealth() const noexcept { return m_config.health; }
    [[nodiscard]] EnemyKind kind() const noexcept { return m_kind; }
    [[nodiscard]] EnemyState state() const noexcept { return m_state; }
    [[nodiscard]] const EnemyConfig& config() const noexcept { return m_config; }

private:
    EnemyConfig m_config;
    EnemyKind m_kind;
    EnemyState m_state;
    int m_health;
    float m_attackCooldown{0};
    std::mt19937 m_random;
    float m_wanderTimer{0};
    Vector2D m_wanderTarget;
    Vector2D m_drift;
    std::vector<Vector2D> m_path;
    std::size_t m_nextWaypoint{0};
    Vector2D m_pathGoal{-1, -1};
    float m_pathCooldown{0};
    [[nodiscard]] Vector2D decideTarget(float dt, Vector2D player, const TileMap* map);
    [[nodiscard]] Vector2D steeringTarget(float dt, Vector2D target, const TileMap& map);
};
