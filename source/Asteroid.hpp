#pragma once

#include "Entity.hpp"

#include <cstddef>
#include <vector>

struct AsteroidConfig {
    float radius{28};
    float speed{65};
    int health{50};
    int damage{1};
    int fragmentCount{4};
    float fragmentScale{0.45f};
    float fragmentSpeedFactor{1.25f};
    int maxFragmentDepth{1};
    float fragmentGrace{0.3f};
    std::size_t maxActive{20};
    float spawnInterval{5};
    float spawnChance{0.1f};
    float maxSpeed{240};
    float linearDamping{0.05f};
    float lifetime{45};
};

// Corpo fisico dinamico. A arena resolve limites, impactos e insercao dos fragmentos.
class Asteroid : public Entity {
public:
    Asteroid(Vector2D position, Vector2D direction, AsteroidConfig config = {}, int depth = 0);
    void update(float dt) noexcept;
    void takeDamage(int amount) noexcept;
    [[nodiscard]] std::vector<Asteroid> fragment();
    [[nodiscard]] int health() const noexcept { return m_health; }
    [[nodiscard]] float radius() const noexcept { return m_config.radius; }
    [[nodiscard]] int depth() const noexcept { return m_depth; }
    [[nodiscard]] bool collisionEnabled() const noexcept { return active && m_collisionGrace <= EPSILON; }
    [[nodiscard]] float remainingLife() const noexcept { return m_remainingLife; }
    [[nodiscard]] const AsteroidConfig& config() const noexcept { return m_config; }

private:
    friend struct StateCodec;
    AsteroidConfig m_config;
    int m_health;
    int m_depth;
    float m_remainingLife;
    float m_collisionGrace{0};
    bool m_fragmented{false};
};
