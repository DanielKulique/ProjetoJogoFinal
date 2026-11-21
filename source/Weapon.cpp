#include "Weapon.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
bool finitePositive(float value) { return std::isfinite(value) && value > 0; }
constexpr float pi = 3.14159265358979323846f;
}

Weapon::Weapon(WeaponConfig config) : m_config(config) {
    if (!finitePositive(config.cooldown) || !finitePositive(config.projectileSpeed) ||
        !finitePositive(config.projectileLifetime) || !finitePositive(config.upgradedCooldown) ||
        !std::isfinite(config.spreadDegrees) || config.spreadDegrees < 0 || config.spreadDegrees > 90 ||
        config.maxLevel < 1 || config.maxLevel > 3 || config.damage <= 0 ||
        !finitePositive(config.progressionStrength) || config.progressionStrength > 8 ||
        config.maxProjectilesPerEmission < 4 || config.maxProjectilesPerEmission > 64 ||
        config.maxBurstShots < 2 || config.maxBurstShots > 16 ||
        !finitePositive(config.minimumCooldown) || !finitePositive(config.maximumProjectileSpeed) ||
        !finitePositive(config.maximumLifetime)) throw std::invalid_argument("Configuracao de arma invalida");
}

void Weapon::update(float dt) noexcept {
    if (!std::isfinite(dt) || dt <= 0) return;
    m_remainingCooldown = std::max(0.0f, m_remainingCooldown - dt);
    m_burstTimer -= dt;
}

int Weapon::acquiredTier(UpgradeKind kind) const noexcept {
    const auto index = static_cast<std::size_t>(kind);
    return index < upgradeKindCount ? m_tiers[index] : 0;
}

WeaponStats Weapon::effectiveStats() const noexcept {
    WeaponStats stats;
    stats.damage = static_cast<float>(m_config.damage);
    stats.cooldown = m_config.cooldown;
    stats.projectileSpeed = m_config.projectileSpeed;
    stats.lifetime = m_config.projectileLifetime;
    stats.spreadDegrees = m_config.spreadDegrees;
    if (m_config.legacyProgression) {
        stats.cooldown = m_level >= 2 ? std::min(m_config.cooldown, m_config.upgradedCooldown) : m_config.cooldown;
        stats.projectiles = m_level >= 3 ? 3 : 1;
        stats.spreadDegrees = m_config.spreadDegrees * 2;
        return stats;
    }
    const auto power = [&](UpgradeKind kind) {
        const auto index = static_cast<std::size_t>(kind);
        return std::sqrt(static_cast<float>(m_tiers[index])) * m_strengths[index] * m_config.progressionStrength;
    };
    const float damage = power(UpgradeKind::Damage);
    const float rate = power(UpgradeKind::FireRate);
    const float speed = power(UpgradeKind::ProjectileSpeed);
    const float duration = power(UpgradeKind::Lifetime);
    // Cada especializacao cobra um custo: dano/frequencia, cadencia/potencia,
    // alcance/velocidade ou velocidade/potencia. Nao ha multiplicacao exponencial por coleta.
    stats.damage *= (1 + 0.18f * damage) / (1 + 0.025f * rate) / (1 + 0.015f * speed);
    stats.cooldown *= (1 + 0.035f * damage) / (1 + 0.18f * rate);
    stats.projectileSpeed *= (1 + 0.16f * speed) / (1 + 0.01f * duration);
    stats.lifetime *= 1 + 0.12f * duration;
    const auto activeKind = static_cast<UpgradeKind>(static_cast<std::size_t>(m_pattern));
    const float patternPower = power(activeKind);
    const int patternTier = acquiredTier(activeKind);
    stats.formationRadius = std::min(30.0f, 12 + 2 * patternPower);
    // Variantes de qualquer padrao refinam potencia com custo pequeno de cadencia.
    stats.damage *= 1 + 0.045f * patternPower;
    stats.cooldown *= 1 + 0.015f * patternPower;
    switch (m_pattern) {
    case ProjectilePattern::Standard: break;
    case ProjectilePattern::Heavy:
        stats.damage *= 2.4f;
        stats.cooldown *= 2.8f;
        stats.projectileSpeed *= 0.8f;
        break;
    case ProjectilePattern::Burst:
        stats.burstCount = std::min(m_config.maxBurstShots, 3 + std::max(0, patternTier - 1) / 3);
        stats.damage *= 0.85f;
        stats.cooldown *= stats.burstCount * 0.9f;
        stats.burstInterval = std::max(0.035f, m_config.cooldown * 0.38f / (1 + 0.03f * patternPower));
        stats.cooldown = std::max(stats.cooldown, stats.burstInterval * (stats.burstCount - 1) + m_config.minimumCooldown);
        break;
    case ProjectilePattern::Circle:
        stats.projectiles = std::min(m_config.maxProjectilesPerEmission, 8 + std::max(0, patternTier - 1) / 2);
        stats.damage *= 0.55f;
        stats.cooldown *= 2.4f;
        break;
    case ProjectilePattern::Triangle:
        stats.projectiles = 3;
        stats.damage *= 0.55f;
        stats.cooldown *= 1.3f;
        break;
    case ProjectilePattern::Square:
        stats.projectiles = 4;
        stats.damage *= 0.45f;
        stats.cooldown *= 1.4f;
        break;
    case ProjectilePattern::Spread:
        stats.projectiles = std::min(m_config.maxProjectilesPerEmission, 3 + 2 * (std::max(0, patternTier - 1) / 3));
        stats.spreadDegrees = std::min(110.0f, 2 * m_config.spreadDegrees + 3 * patternPower);
        stats.damage *= 1.6f / stats.projectiles;
        stats.cooldown *= 1.2f;
        break;
    case ProjectilePattern::Parallel:
        stats.projectiles = std::min(m_config.maxProjectilesPerEmission, 3 + 2 * (std::max(0, patternTier - 1) / 4));
        stats.damage *= 1.6f / stats.projectiles;
        stats.cooldown *= 1.2f;
        break;
    }
    stats.damage = std::max(1.0f, stats.damage);
    stats.cooldown = std::max(std::min(m_config.minimumCooldown, m_config.cooldown), stats.cooldown);
    stats.projectileSpeed = std::clamp(stats.projectileSpeed, std::min(100.0f, m_config.projectileSpeed),
                                      std::max(std::min(100.0f, m_config.projectileSpeed), m_config.maximumProjectileSpeed));
    stats.lifetime = std::min(stats.lifetime, m_config.maximumLifetime);
    return stats;
}

float Weapon::cooldown() const noexcept { return effectiveStats().cooldown; }

bool Weapon::canApply(const WeaponUpgrade& upgrade) const noexcept {
    if (!validUpgrade(upgrade)) return false;
    if (m_config.legacyProgression) return m_level < m_config.maxLevel;
    const auto index = static_cast<std::size_t>(upgrade.kind);
    if (upgrade.tier <= m_tiers[index]) return false;
    // Especializacoes ja no teto nao viram itens sem efeito. Padroes e dano/cadencia
    // continuam possuindo variantes, sem teto global no numero de aquisicoes.
    const auto stats = effectiveStats();
    if (upgrade.kind == UpgradeKind::ProjectileSpeed && stats.projectileSpeed >= m_config.maximumProjectileSpeed) return false;
    if (upgrade.kind == UpgradeKind::Lifetime && stats.lifetime >= m_config.maximumLifetime) return false;
    if (upgrade.kind == UpgradeKind::FireRate && stats.cooldown <= std::min(m_config.minimumCooldown, m_config.cooldown) + EPSILON) return false;
    return true;
}

bool Weapon::applyUpgrade(const WeaponUpgrade& upgrade) noexcept {
    if (!canApply(upgrade)) return false;
    if (m_config.legacyProgression) return this->upgrade();
    const auto index = static_cast<std::size_t>(upgrade.kind);
    m_tiers[index] = upgrade.tier;
    m_strengths[index] = upgrade.strength;
    if (index < projectilePatternCount) m_pattern = static_cast<ProjectilePattern>(index);
    if (m_acquiredCount < std::numeric_limits<int>::max() - 1) ++m_acquiredCount;
    m_remainingCooldown = std::min(m_remainingCooldown, cooldown());
    return true;
}

bool Weapon::upgrade() noexcept {
    if (m_config.legacyProgression) {
        if (m_level >= m_config.maxLevel) return false;
        ++m_level;
        m_pattern = m_level >= 3 ? ProjectilePattern::Spread : ProjectilePattern::Standard;
        m_remainingCooldown = std::min(m_remainingCooldown, cooldown());
        return true;
    }
    // API de compatibilidade; o jogo usa a definicao desbloqueada no item.
    constexpr UpgradeKind sequence[]{UpgradeKind::FireRate, UpgradeKind::Spread, UpgradeKind::Damage,
        UpgradeKind::ProjectileSpeed, UpgradeKind::Lifetime};
    const auto kind = sequence[m_acquiredCount % 5];
    return applyUpgrade({kind, acquiredTier(kind) + 1, 1});
}

int Weapon::quantizedDamage(float nominal) noexcept {
    const float total = nominal + m_damageCarry;
    const float bounded = std::min(total, static_cast<float>(std::numeric_limits<int>::max() / 2));
    const int damage = std::max(1, static_cast<int>(std::floor(bounded)));
    m_damageCarry = std::max(0.0f, bounded - damage);
    return damage;
}

std::optional<Projectile> Weapon::tryFire(Vector2D muzzle, Vector2D direction) {
    if (m_remainingCooldown > EPSILON || m_burstRemaining > 0 || direction.length_squared() <= EPSILON * EPSILON) return std::nullopt;
    const auto stats = effectiveStats();
    m_remainingCooldown = stats.cooldown;
    Projectile result(muzzle, direction, stats.projectileSpeed, quantizedDamage(stats.damage), stats.lifetime);
    result.patternId = static_cast<std::uint8_t>(m_pattern);
    return result;
}

std::vector<Projectile> Weapon::emit(ProjectilePattern pattern, const WeaponStats& stats,
                                    Vector2D muzzle, Vector2D direction) {
    const auto rays = projectilePatternRays(pattern, muzzle, direction, stats.projectiles,
                                           stats.spreadDegrees, stats.formationRadius);
    std::vector<Projectile> result;
    result.reserve(rays.size());
    for (const auto& ray : rays) {
        result.emplace_back(ray.position, ray.direction, stats.projectileSpeed, quantizedDamage(stats.damage), stats.lifetime);
        result.back().patternId = static_cast<std::uint8_t>(pattern);
    }
    return result;
}

std::vector<Projectile> Weapon::fireVolley(Vector2D muzzle, Vector2D direction) {
    if (m_remainingCooldown > EPSILON || m_burstRemaining > 0 || direction.length_squared() <= EPSILON * EPSILON) return {};
    const auto stats = effectiveStats();
    m_remainingCooldown = stats.cooldown;
    const auto pattern = m_config.legacyProgression && m_level >= 3 ? ProjectilePattern::Spread : m_pattern;
    auto result = emit(pattern, stats, muzzle, direction);
    if (!m_config.legacyProgression && m_pattern == ProjectilePattern::Burst) {
        m_burstRemaining = stats.burstCount - 1;
        m_burstTimer = stats.burstInterval;
        m_burstStats = stats;
    }
    return result;
}

std::vector<Projectile> Weapon::pollProjectiles(Vector2D muzzle, Vector2D direction) {
    std::vector<Projectile> result;
    if (direction.length_squared() <= EPSILON * EPSILON) return result;
    while (m_burstRemaining > 0 && m_burstTimer <= EPSILON) {
        auto next = emit(ProjectilePattern::Burst, m_burstStats, muzzle, direction);
        for (auto& projectile : next) result.push_back(std::move(projectile));
        --m_burstRemaining;
        m_burstTimer += m_burstStats.burstInterval;
    }
    return result;
}

std::string Weapon::summary() const {
    const auto stats = effectiveStats();
    std::ostringstream text;
    text << patternName(m_pattern) << " | " << (m_config.legacyProgression ? m_level - 1 : m_acquiredCount)
         << " melhorias | D " << static_cast<int>(std::round(stats.damage))
         << " | " << stats.projectiles << (stats.burstCount > 1 ? "x" + std::to_string(stats.burstCount) : "")
         << " | " << static_cast<int>(std::round(1 / stats.cooldown)) << "/s"
         << " | V " << static_cast<int>(std::round(stats.projectileSpeed))
         << " | T " << std::fixed << std::setprecision(1) << stats.lifetime;
    return text.str();
}
