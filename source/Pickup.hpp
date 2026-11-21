#pragma once

#include "Entity.hpp"
#include "WeaponUpgrade.hpp"
#include <optional>

#include <cstddef>

enum class PickupKind { WeaponUpgrade, Engine };

struct PickupConfig {
    float lifetime{20};
    float halfExtent{12};
    std::size_t maxActive{12};
};

class Pickup : public Entity {
public:
    Pickup(Vector2D position, PickupKind kind, PickupConfig config = {},
           std::optional<WeaponUpgrade> upgrade = std::nullopt);
    void update(float dt) noexcept;
    [[nodiscard]] const std::optional<WeaponUpgrade>& upgrade() const noexcept { return m_upgrade; }
    [[nodiscard]] PickupKind kind() const noexcept { return m_kind; }
    [[nodiscard]] float remainingLife() const noexcept { return m_remainingLife; }
    [[nodiscard]] const PickupConfig& config() const noexcept { return m_config; }

private:
    friend struct StateCodec;
    PickupKind m_kind;
    std::optional<WeaponUpgrade> m_upgrade;
    PickupConfig m_config;
    float m_remainingLife;
};
