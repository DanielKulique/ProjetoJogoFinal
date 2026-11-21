#include "Weapon.hpp"

#include <algorithm>
#include <stdexcept>
#include <cmath>

Weapon::Weapon(WeaponConfig config) : m_config(config) {
    if (!std::isfinite(config.cooldown) || !std::isfinite(config.projectileSpeed) ||
        !std::isfinite(config.projectileLifetime) || !std::isfinite(config.upgradedCooldown) ||
        !std::isfinite(config.spreadDegrees) || config.upgradedCooldown <= 0 ||
        config.spreadDegrees < 0 || config.spreadDegrees > 90 ||
        config.maxLevel < 1 || config.maxLevel > 3 ||
        config.cooldown <= 0 || config.projectileSpeed <= 0 || config.damage <= 0 ||
        config.projectileLifetime <= 0) throw std::invalid_argument("Configuracao de arma invalida");
}

void Weapon::update(float dt) noexcept {
    if (!std::isfinite(dt) || dt <= 0) return;
    m_remainingCooldown = std::max(0.0f, m_remainingCooldown - dt);
}

float Weapon::cooldown() const noexcept { return m_level >= 2 ? std::min(m_config.cooldown, m_config.upgradedCooldown) : m_config.cooldown; }

bool Weapon::upgrade() noexcept {
    if (m_level >= m_config.maxLevel) return false;
    ++m_level;
    m_remainingCooldown = std::min(m_remainingCooldown, cooldown());
    return true;
}

std::optional<Projectile> Weapon::tryFire(Vector2D muzzle, Vector2D direction) {
    if (m_remainingCooldown > EPSILON || direction.length_squared() <= EPSILON * EPSILON) {
        return std::nullopt;
    }
    m_remainingCooldown = cooldown();
    return Projectile(muzzle, direction, m_config.projectileSpeed, m_config.damage,
                      m_config.projectileLifetime);
}

std::vector<Projectile> Weapon::fireVolley(Vector2D muzzle, Vector2D direction) {
    auto center = tryFire(muzzle, direction);
    if (!center) return {};
    std::vector<Projectile> result;
    result.reserve(m_level >= 3 ? 3 : 1);
    result.push_back(std::move(*center));
    if (m_level >= 3) {
        const float angle = m_config.spreadDegrees * 3.14159265358979323846f / 180.0f;
        const float cosine = std::cos(angle), sine = std::sin(angle);
        const Vector2D left{direction.x * cosine - direction.y * sine,
                            direction.x * sine + direction.y * cosine};
        const Vector2D right{direction.x * cosine + direction.y * sine,
                             -direction.x * sine + direction.y * cosine};
        result.emplace_back(muzzle, left, m_config.projectileSpeed, m_config.damage, m_config.projectileLifetime);
        result.emplace_back(muzzle, right, m_config.projectileSpeed, m_config.damage, m_config.projectileLifetime);
    }
    return result;
}
