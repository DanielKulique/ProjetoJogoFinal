#pragma once

#include <random>

// Uma amostra em [0,1); evita depender de uniform_real_distribution entre bibliotecas.
inline double randomUnit(std::mt19937& random) {
    return static_cast<double>(random()) / (static_cast<double>(std::mt19937::max()) + 1.0);
}
inline bool randomChance(std::mt19937& random, float probability) {
    return randomUnit(random) < probability;
}
