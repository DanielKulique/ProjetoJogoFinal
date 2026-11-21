#include "Asteroid.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace {
bool positive(float value) { return std::isfinite(value) && value > 0; }
bool nonnegative(float value) { return std::isfinite(value) && value >= 0; }

void capVelocity(RigidBody2D& body, float maximum) noexcept {
    if (body.velocity.length_squared() > maximum * maximum) {
        body.velocity = body.velocity.normalized() * maximum;
    }
}
}

Asteroid::Asteroid(Vector2D position, Vector2D direction, AsteroidConfig config, int depth)
    : m_config(config), m_health(config.health), m_depth(depth), m_remainingLife(config.lifetime) {
    if (!positive(config.radius) || !positive(config.speed) || config.health <= 0 || config.damage <= 0 ||
        (config.fragmentCount != 3 && config.fragmentCount != 4) || !positive(config.fragmentScale) ||
        config.fragmentScale >= 1 || !positive(config.fragmentSpeedFactor) || config.maxFragmentDepth < 0 ||
        !nonnegative(config.fragmentGrace) || config.maxActive == 0 || !positive(config.spawnInterval) ||
        !nonnegative(config.spawnChance) || config.spawnChance > 1 || !positive(config.maxSpeed) ||
        !nonnegative(config.linearDamping) || !positive(config.lifetime) || depth < 0 ||
        !std::isfinite(position.x) || !std::isfinite(position.y) ||
        !std::isfinite(direction.x) || !std::isfinite(direction.y) ||
        direction.length_squared() <= EPSILON * EPSILON) {
        throw std::invalid_argument("Configuracao de asteroide invalida");
    }
    body.position = position;
    body.velocity = direction.normalized() * std::min(config.speed, config.maxSpeed);
    collision.halfExtents = {config.radius, config.radius};
    if (depth > 0) m_collisionGrace = config.fragmentGrace;
}

void Asteroid::update(float dt) noexcept {
    if (!active || !std::isfinite(dt) || dt <= 0) return;
    const float step = std::min(dt, m_remainingLife);
    // Resistência exponencial conserva a direcao sem gravidade e limita a energia acumulada.
    body.velocity *= std::exp(-m_config.linearDamping * step);
    body.integrate(step);
    capVelocity(body, m_config.maxSpeed);
    body.acceleration = {};
    m_remainingLife = std::max(0.0f, m_remainingLife - dt);
    m_collisionGrace = std::max(0.0f, m_collisionGrace - dt);
    if (m_remainingLife <= EPSILON) active = false;
}

void Asteroid::takeDamage(int amount) noexcept {
    if (!active || amount <= 0) return;
    m_health = std::max(0, m_health - amount);
    if (m_health == 0) active = false;
}

std::vector<Asteroid> Asteroid::fragment() {
    std::vector<Asteroid> children;
    if (active || m_health > 0 || m_fragmented || m_depth >= m_config.maxFragmentDepth) return children;
    m_fragmented = true;
    children.reserve(static_cast<std::size_t>(m_config.fragmentCount));
    auto childConfig = m_config;
    childConfig.radius *= m_config.fragmentScale;
    childConfig.health = std::max(1, m_config.health / 2);
    childConfig.speed = std::min(m_config.maxSpeed, m_config.speed * m_config.fragmentSpeedFactor);
    constexpr float twoPi = 6.28318530717958647692f;
    const float initialAngle = std::atan2(body.velocity.y, body.velocity.x);
    for (int i = 0; i < m_config.fragmentCount; ++i) {
        const float angle = initialAngle + twoPi * static_cast<float>(i) / m_config.fragmentCount;
        const Vector2D radial{std::cos(angle), std::sin(angle)};
        const auto position = body.position + radial * (m_config.radius + childConfig.radius);
        Asteroid child(position, radial, childConfig, m_depth + 1);
        child.body.velocity = body.velocity * 0.5f + radial * childConfig.speed;
        capVelocity(child.body, m_config.maxSpeed);
        children.push_back(std::move(child));
    }
    return children;
}
