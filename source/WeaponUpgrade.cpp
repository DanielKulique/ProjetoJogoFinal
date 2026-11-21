#include "WeaponUpgrade.hpp"

#include <cmath>

bool validUpgrade(const WeaponUpgrade& upgrade) noexcept {
    return static_cast<std::size_t>(upgrade.kind) < upgradeKindCount && upgrade.tier > 0 &&
           std::isfinite(upgrade.strength) && upgrade.strength > 0 && upgrade.strength <= 8;
}

const char* upgradeName(UpgradeKind kind) noexcept {
    constexpr const char* names[]{"Padrao", "Rajada", "Pesado", "Circular", "Triangular", "Quadrado",
        "Espalhado", "Paralelo", "Cadencia", "Velocidade", "Dano", "Duracao"};
    const auto index = static_cast<std::size_t>(kind);
    return index < upgradeKindCount ? names[index] : "Desconhecido";
}

std::string upgradeLabel(const WeaponUpgrade& upgrade) {
    return std::string(upgradeName(upgrade.kind)) + " " + std::to_string(upgrade.tier);
}
