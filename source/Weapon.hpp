#pragma once

#include "Projectile.hpp"

#include <optional>
#include <vector>

struct WeaponConfig {
    float cooldown{0.18f};
    float projectileSpeed{700.0f};
    int damage{25};
    float projectileLifetime{2.5f};
    float upgradedCooldown{0.12f};
    float spreadDegrees{12.0f};
    int maxLevel{3};
};

class Weapon {
public:
    explicit Weapon(WeaponConfig config = {});
    void update(float dt) noexcept;
    [[nodiscard]] std::optional<Projectile> tryFire(Vector2D muzzle, Vector2D direction);
    [[nodiscard]] std::vector<Projectile> fireVolley(Vector2D muzzle, Vector2D direction);
    bool upgrade() noexcept;
    [[nodiscard]] int level() const noexcept { return m_level; }
    [[nodiscard]] float cooldown() const noexcept;
    [[nodiscard]] const WeaponConfig& config() const noexcept { return m_config; }

private:
    WeaponConfig m_config;
    int m_level{1};
    float m_remainingCooldown{0};
};
