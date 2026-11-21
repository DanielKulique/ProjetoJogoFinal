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
