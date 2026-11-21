// Armazena e valida a matriz comum aos mapas CSV e procedurais, sem SFML.
#pragma once

#include "Vector2D.hpp"
#include "Collision2D.hpp"

#include <string>
#include <vector>

class Dungeon2D;

inline constexpr char EMPTY_TILE = '.';
inline constexpr char WALL_TILE = '#';

class TileMap {
public:
    using Matrix = std::vector<std::string>;

    TileMap() = default;
    explicit TileMap(Matrix rows);

    // Falhas preservam o mapa anterior e preenchem error, quando fornecido.
    bool loadCSV(const std::string& path, std::string* error = nullptr);
    void loadFromDungeon(const Dungeon2D& dungeon);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] bool contains(int col, int row) const noexcept;
    [[nodiscard]] char at(int col, int row) const; // Lanca out_of_range fora da matriz.
    [[nodiscard]] bool isWall(int col, int row) const noexcept;
    [[nodiscard]] bool isFree(int col, int row) const noexcept;
    [[nodiscard]] const Matrix& rows() const noexcept;

    // Origem no canto superior esquerdo; x = coluna, y = linha, em pixels.
    [[nodiscard]] Vector2D cellCenter(int col, int row) const;
    [[nodiscard]] bool isFreeAtWorld(const Vector2D& position) const noexcept;
    [[nodiscard]] bool isAreaFree(const AABB& area) const noexcept;

private:
    Matrix m_rows;
};
