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
    bool avoidFullEngineDrops{true};
};

struct ScoreConfig {
    std::uint64_t normal{100}, advanced{250}, asteroid{50}, fragment{10}, round{500};
};

// Ponto unico de ajuste do balanceamento; regras nao dependem da apresentacao.
struct GameConfig {
    ScoreConfig scoring;
    PlayerConfig player;
    WeaponConfig weapon;
    WaveConfig waves;
    AsteroidConfig asteroid;
    GravityFieldConfig gravity;
    PickupConfig pickup;
    DropConfig drops;
    std::uint32_t seed{1234};
    bool reseedOnRestart{false};
    std::size_t maxProjectiles{256};
    std::size_t maxExplosions{32};
    float explosionLifetime{0.35f};
    void validate() const;
    [[nodiscard]] static GameConfig recommended();
};
