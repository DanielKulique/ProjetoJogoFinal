#include "GravityField.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

GravityField::GravityField(Vector2D position, GravityFieldConfig config)
    : m_config(config), m_remainingLife(config.duration) {
    const auto positive = [](float value) { return std::isfinite(value) && value > 0; };
    if (!positive(config.spawnInterval) || !std::isfinite(config.spawnChance) || config.spawnChance < 0 ||
        config.spawnChance > 1 || !positive(config.radius) || !positive(config.duration) ||
        !positive(config.accelerationMeters) || !positive(config.pixelsPerMeter) ||
        !positive(config.softening) || !positive(config.damping) || !positive(config.maxDrift) ||
        !std::isfinite(config.accelerationMeters * config.pixelsPerMeter) ||
        !std::isfinite(position.x) || !std::isfinite(position.y)) {
        throw std::invalid_argument("Configuracao de gravidade invalida");
    }
    body.position = position;
    collision.halfExtents = {config.radius, config.radius};
}

void GravityField::update(float dt) noexcept {
    if (!active || !std::isfinite(dt) || dt <= 0) return;
    m_remainingLife = std::max(0.0f, m_remainingLife - dt);
    if (m_remainingLife <= EPSILON) active = false;
}

Vector2D GravityField::accelerationAt(Vector2D position) const noexcept {
    if (!active || !std::isfinite(position.x) || !std::isfinite(position.y)) return {};
    const auto direction = body.position - position;
    const float distance = direction.length();
    if (distance <= EPSILON || distance >= m_config.radius) return {};
    // Hipotese: 10 m/s² e 32 px/m => 320 px/s² antes das atenuacoes.
    const float edgeFalloff = 1.0f - distance / m_config.radius;
    const float centerFalloff = std::min(1.0f, distance / m_config.softening);
    const float strength = m_config.accelerationMeters * m_config.pixelsPerMeter * edgeFalloff * centerFalloff;
    return direction * (strength / distance);
}

void GravityField::apply(RigidBody2D& target) const noexcept {
    target.acceleration = accelerationAt(target.position);
}
