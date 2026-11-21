#include "Player.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

Player::Player(PlayerConfig config, WeaponConfig weapon) : m_config(config), m_weapon(weapon) {
    if (!std::isfinite(config.damageInvulnerability) || config.damageInvulnerability <= 0 ||
        !std::isfinite(config.driftDamping) || config.driftDamping < 0 ||
        !std::isfinite(config.maxDrift) || config.maxDrift < 0 || config.engineSpeeds[0] != 0) {
        throw std::invalid_argument("Configuracao de jogador invalida");
    }
    for (std::size_t i = 1; i < config.engineSpeeds.size(); ++i) {
        if (!std::isfinite(config.engineSpeeds[i]) || config.engineSpeeds[i] <= 0 ||
            config.engineSpeeds[i] < config.engineSpeeds[i - 1]) {
            throw std::invalid_argument("Velocidades de motores invalidas");
        }
    }
    collision.halfExtents = {16, 16};
}

bool Player::takeDamage(int amount) noexcept {
    if (!active || amount <= 0 || invulnerable()) return false;
    m_engines = std::max(0, m_engines - amount);
    m_damageCooldown = m_config.damageInvulnerability;
    active = m_engines > 0;
    body.velocity = active ? m_movementDirection * moveSpeed() + m_drift : Vector2D{};
    return true;
}

bool Player::restoreEngine() noexcept {
    if (!active || m_engines >= maxHealth()) return false;
    ++m_engines;
    body.velocity = m_movementDirection * moveSpeed() + m_drift;
    return true;
}

void Player::aimAt(const Vector2D& target) noexcept {
    const auto direction = target - body.position;
    if (direction.length_squared() > EPSILON * EPSILON) {
        m_angleDegrees = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265358979323846f;
    }
}

float Player::angleDegrees() const noexcept { return m_angleDegrees; }

Vector2D Player::aimDirection() const noexcept {
    const float radians = m_angleDegrees * 3.14159265358979323846f / 180.0f;
    return {std::cos(radians), std::sin(radians)};
}

void Player::update(float dt) noexcept {
    if (!active || !std::isfinite(dt) || dt <= 0) return;
    // A aceleracao externa forma uma deriva persistente, independente do comando WASD.
    m_drift += body.acceleration * dt;
    m_drift *= std::exp(-m_config.driftDamping * dt);
    if (m_drift.length_squared() > m_config.maxDrift * m_config.maxDrift) {
        m_drift *= m_config.maxDrift / m_drift.length();
    }
    body.acceleration = {};
    body.velocity = m_movementDirection * moveSpeed() + m_drift;
    body.integrate(dt);
    m_weapon.update(dt);
    m_damageCooldown = std::max(0.0f, m_damageCooldown - dt);
}

void Player::setMovementDirection(const Vector2D& direction) {
    m_movementDirection = direction.length_squared() > EPSILON * EPSILON ? direction.normalized() : Vector2D{};
    body.velocity = active ? m_movementDirection * moveSpeed() + m_drift : Vector2D{};
}
