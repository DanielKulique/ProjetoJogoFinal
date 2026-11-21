#pragma once

#include "Entity.hpp"

#include <cstddef>

enum class PickupKind { WeaponUpgrade, Engine };

struct PickupConfig {
    float lifetime{20};
    float halfExtent{12};
    std::size_t maxActive{12};
};

class Pickup : public Entity {
public:
    Pickup(Vector2D position, PickupKind kind, PickupConfig config = {});
    void update(float dt) noexcept;
    [[nodiscard]] PickupKind kind() const noexcept { return m_kind; }
    [[nodiscard]] float remainingLife() const noexcept { return m_remainingLife; }
    [[nodiscard]] const PickupConfig& config() const noexcept { return m_config; }

private:
    PickupKind m_kind;
    PickupConfig m_config;
    float m_remainingLife;
};
