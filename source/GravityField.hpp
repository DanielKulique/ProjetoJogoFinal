#pragma once

#include "Entity.hpp"

struct GravityFieldConfig {
    float spawnInterval{60};
    float spawnChance{0.5f};
    float radius{260};
    float duration{20};
    float accelerationMeters{10};
    float pixelsPerMeter{32};
    float softening{16};
    float damping{2};
    float maxDrift{100};
};

// Regiao de influencia; nao integra corpos nem representa um obstaculo solido.
class GravityField : public Entity {
public:
    GravityField(Vector2D position, GravityFieldConfig config = {});
    void update(float dt) noexcept;
    [[nodiscard]] Vector2D accelerationAt(Vector2D position) const noexcept;
    void apply(RigidBody2D& body) const noexcept;
    [[nodiscard]] float remainingLife() const noexcept { return m_remainingLife; }
    [[nodiscard]] float radius() const noexcept { return m_config.radius; }
    [[nodiscard]] const GravityFieldConfig& config() const noexcept { return m_config; }

private:
    GravityFieldConfig m_config;
    float m_remainingLife;
};
