#pragma once

#include "Entity.hpp"
#include <cstdint>

class Projectile : public Entity {
public:
    std::uint8_t patternId{0}; // Origem para telemetria; nao altera o impacto.
    Projectile(Vector2D position, Vector2D direction, float speed, int damage, float lifetime);
    void update(float dt) noexcept;
    [[nodiscard]] int damage() const noexcept { return m_damage; }
    [[nodiscard]] float remainingLife() const noexcept { return m_remainingLife; }
    [[nodiscard]] Vector2D previousPosition() const noexcept { return m_previousPosition; }

private:
    friend struct StateCodec;
    int m_damage;
    float m_remainingLife;
    Vector2D m_previousPosition;
};
