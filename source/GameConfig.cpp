#include "GameConfig.hpp"

#include <cmath>
#include <stdexcept>

void GameConfig::validate() const {
    // Os componentes validam seus respectivos contratos sem criar contexto grafico.
    const Player validPlayer(player, weapon);
    const WaveManager validWaves(waves);
    const Asteroid validAsteroid({}, {1, 0}, asteroid);
    const GravityField validField({}, gravity);
    const Pickup validPickup({}, PickupKind::Engine, pickup);
    const auto probability = [](float p) { return std::isfinite(p) && p >= 0 && p <= 1; };
    if (!probability(drops.chance) || !probability(drops.upgradeChance) ||
        maxProjectiles == 0 || maxExplosions == 0 ||
        !std::isfinite(explosionLifetime) || explosionLifetime <= 0) {
        throw std::invalid_argument("Configuracao global invalida");
    }
}

// Recomendacao offline growth18: definida apenas com sementes 1001..1020.
// GameConfig{} conserva os escalares do baseline para reproduzir os experimentos.
GameConfig GameConfig::recommended() {
    GameConfig config;
    config.drops.chance = 0.90f;
    config.drops.upgradeChance = 1.0f - 0.60f; // Mesmo float do override experimental --engine-chance .60.
    config.waves.advancedChance = 0.30f;
    config.waves.healthPerRound = 18;
    config.waves.spawnInterval = 1.6f;
    config.weapon.progressionStrength = 1.2f;
    config.asteroid.spawnChance = 0.15f;
    config.gravity.accelerationMeters = 12;
    config.pickup.lifetime = 35;
    return config;
}
