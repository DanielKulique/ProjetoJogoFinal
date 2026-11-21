#include "Projectile.hpp"

#include <stdexcept>
#include <algorithm>

Projectile::Projectile(Vector2D position, Vector2D direction, float speed, int damage, float lifetime)
    : m_damage(damage), m_remainingLife(lifetime), m_previousPosition(position) {
    if (!std::isfinite(speed) || !std::isfinite(lifetime) ||
        direction.length_squared() <= EPSILON * EPSILON || speed <= 0 || damage <= 0 || lifetime <= 0) {
        throw std::invalid_argument("Parametros de projetil invalidos");
    }
    body.position = position;
    body.velocity = direction.normalized() * speed;
    collision.halfExtents = {4, 4};
}

void Projectile::update(float dt) noexcept {
    if (!active || dt <= 0) return;
    m_previousPosition = body.position;
    body.integrate(std::min(dt, m_remainingLife));
    m_remainingLife -= dt;
    if (m_remainingLife <= EPSILON) active = false;
}
