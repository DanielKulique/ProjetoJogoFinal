#pragma once

#include "Projectile.hpp"
#include "ProjectilePattern.hpp"
#include "WeaponUpgrade.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

struct WeaponConfig {
    float cooldown{0.18f};
    float projectileSpeed{700.0f};
    int damage{25};
    float projectileLifetime{2.5f};
    float upgradedCooldown{0.12f};
    float spreadDegrees{12.0f};
    int maxLevel{3}; // Apenas reproducao do baseline anterior.
    bool legacyProgression{false};
    float progressionStrength{1};
    int maxProjectilesPerEmission{16};
    int maxBurstShots{5};
    float minimumCooldown{0.06f};
    float maximumProjectileSpeed{1600};
    float maximumLifetime{8};
};

struct WeaponStats {
    float damage{25};
    float cooldown{0.18f};
    float projectileSpeed{700};
    float lifetime{2.5f};
    int projectiles{1};
    float spreadDegrees{12};
    int burstCount{1};
    float burstInterval{0.07f};
    float formationRadius{12};
};

class Weapon {
public:
    explicit Weapon(WeaponConfig config = {});
    void update(float dt) noexcept;
    [[nodiscard]] std::optional<Projectile> tryFire(Vector2D muzzle, Vector2D direction);
    [[nodiscard]] std::vector<Projectile> fireVolley(Vector2D muzzle, Vector2D direction);
    // Chamar a cada passo mesmo sem gatilho, depois de update(dt), para continuar rajadas.
    [[nodiscard]] std::vector<Projectile> pollProjectiles(Vector2D muzzle, Vector2D direction);
    bool upgrade() noexcept;
    bool applyUpgrade(const WeaponUpgrade& upgrade) noexcept;
    [[nodiscard]] bool canApply(const WeaponUpgrade& upgrade) const noexcept;
    [[nodiscard]] int acquiredCount() const noexcept { return m_acquiredCount; }
    [[nodiscard]] int acquiredTier(UpgradeKind kind) const noexcept;
    [[nodiscard]] ProjectilePattern pattern() const noexcept { return m_pattern; }
    [[nodiscard]] WeaponStats effectiveStats() const noexcept;
    [[nodiscard]] std::string summary() const;
    [[nodiscard]] int level() const noexcept { return m_config.legacyProgression ? m_level : m_acquiredCount + 1; }
    [[nodiscard]] float cooldown() const noexcept;
    [[nodiscard]] const WeaponConfig& config() const noexcept { return m_config; }

private:
    friend struct StateCodec;
    WeaponConfig m_config;
    int m_level{1};
    int m_acquiredCount{0};
    std::array<int, upgradeKindCount> m_tiers{};
    std::array<float, upgradeKindCount> m_strengths{};
    ProjectilePattern m_pattern{ProjectilePattern::Standard};
    float m_remainingCooldown{0};
    float m_damageCarry{0};
    int m_burstRemaining{0};
    float m_burstTimer{0};
    WeaponStats m_burstStats;
    [[nodiscard]] std::vector<Projectile> emit(ProjectilePattern pattern, const WeaponStats& stats,
                                               Vector2D muzzle, Vector2D direction);
    [[nodiscard]] int quantizedDamage(float nominal) noexcept;
};
