#include "Pickup.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

Pickup::Pickup(Vector2D position, PickupKind kind, PickupConfig config)
    : m_kind(kind), m_config(config), m_remainingLife(config.lifetime) {
    if (!std::isfinite(config.lifetime) || config.lifetime <= 0 ||
        !std::isfinite(config.halfExtent) || config.halfExtent <= 0 || config.maxActive == 0 ||
        !std::isfinite(position.x) || !std::isfinite(position.y) ||
        (kind != PickupKind::WeaponUpgrade && kind != PickupKind::Engine)) {
        throw std::invalid_argument("Configuracao de item invalida");
    }
    body.position = position;
    collision.halfExtents = {config.halfExtent, config.halfExtent};
}

void Pickup::update(float dt) noexcept {
    if (!active || !std::isfinite(dt) || dt <= 0) return;
    m_remainingLife = std::max(0.0f, m_remainingLife - dt);
    if (m_remainingLife <= EPSILON) active = false;
}
