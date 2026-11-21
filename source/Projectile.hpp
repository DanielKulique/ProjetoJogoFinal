#pragma once

#include "Entity.hpp"

class Projectile : public Entity {
public:
    Projectile(Vector2D position, Vector2D direction, float speed, int damage, float lifetime);
    void update(float dt) noexcept;
    [[nodiscard]] int damage() const noexcept { return m_damage; }
    [[nodiscard]] float remainingLife() const noexcept { return m_remainingLife; }
    [[nodiscard]] Vector2D previousPosition() const noexcept { return m_previousPosition; }

private:
    int m_damage;
    float m_remainingLife;
    Vector2D m_previousPosition;
};
