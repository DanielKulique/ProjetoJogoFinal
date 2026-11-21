#pragma once

#include "WeaponUpgrade.hpp"

#include <array>
#include <optional>
#include <random>
#include <vector>

class Weapon;

// Libera definicoes; nao aplica beneficios e nao possui RNG oculto.
class UpgradeManager {
public:
    [[nodiscard]] std::optional<WeaponUpgrade> unlockForRound(int round, std::mt19937& random,
                                                            const Weapon* weapon = nullptr);
    [[nodiscard]] const std::vector<WeaponUpgrade>& unlocked() const noexcept { return m_unlocked; }
    [[nodiscard]] std::optional<WeaponUpgrade> chooseUpgrade(const Weapon& weapon, std::mt19937& random) const;
    void reset() noexcept;
    [[nodiscard]] int lastRound() const noexcept { return m_lastRound; }

private:
    friend struct StateCodec;
    int m_lastRound{0};
    std::array<int, upgradeKindCount> m_tiers{};
    std::vector<UpgradeKind> m_bag;
    std::vector<WeaponUpgrade> m_unlocked;
};
