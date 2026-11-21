#pragma once

#include "Player.hpp"
#include "WaveManager.hpp"
#include "Asteroid.hpp"
#include "GravityField.hpp"
#include "Pickup.hpp"

#include <cstddef>
#include <cstdint>

struct DropConfig {
    float chance{0.75f};
    float upgradeChance{0.8f};
};

// Ponto unico de ajuste do balanceamento; regras nao dependem da apresentacao.
struct GameConfig {
    PlayerConfig player;
    WeaponConfig weapon;
    WaveConfig waves;
    AsteroidConfig asteroid;
    GravityFieldConfig gravity;
    PickupConfig pickup;
    DropConfig drops;
    std::uint32_t seed{1234};
    std::size_t maxProjectiles{256};
    std::size_t maxExplosions{32};
    float explosionLifetime{0.35f};
    void validate() const;
};
