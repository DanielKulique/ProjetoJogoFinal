// Gera uma arena conectada por semente, reservando o centro para o futuro portal.
#pragma once

#include "TileMap.hpp"

#include <cstdint>

class Dungeon2D {
public:
    // Dimensoes de pelo menos 7 x 7; o contorno permanece fechado.
    Dungeon2D(int width, int height);
    void generate(std::uint32_t seed);
    [[nodiscard]] const TileMap::Matrix& rows() const noexcept;

    static constexpr int centerRadius = 1; // Reserva 3 x 3 celulas.

private:
    int m_width;
    int m_height;
    TileMap::Matrix m_rows; // Vazia ate generate().
};
