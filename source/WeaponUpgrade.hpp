#pragma once

#include <cstddef>
#include <string>

enum class UpgradeKind {
    Standard, Burst, Heavy, Circle, Triangle, Square, Spread, Parallel,
    FireRate, ProjectileSpeed, Damage, Lifetime, Count
};

inline constexpr std::size_t upgradeKindCount = static_cast<std::size_t>(UpgradeKind::Count);

struct WeaponUpgrade {
    UpgradeKind kind{UpgradeKind::Standard};
    int tier{1};
    float strength{1};
};

[[nodiscard]] bool validUpgrade(const WeaponUpgrade& upgrade) noexcept;
[[nodiscard]] const char* upgradeName(UpgradeKind kind) noexcept;
[[nodiscard]] std::string upgradeLabel(const WeaponUpgrade& upgrade);
