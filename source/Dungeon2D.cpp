// Escava caminhos a partir de uma sala central; cada novo piso toca um piso existente.
#include "Dungeon2D.hpp"

#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

Dungeon2D::Dungeon2D(int width, int height) : m_width(width), m_height(height) {
    if (width < 7 || height < 7) {
        throw std::invalid_argument("Dungeon2D exige dimensoes de pelo menos 7 x 7");
    }
}

void Dungeon2D::generate(std::uint32_t seed) {
    TileMap::Matrix rows(m_height, std::string(m_width, WALL_TILE));
    std::mt19937 random(seed);
    const auto interior = static_cast<std::size_t>(m_width - 2) * (m_height - 2);
    const auto target = interior / 2 + interior / 10;
    std::vector<std::pair<int, int>> frontier;
    frontier.reserve(interior);

    const auto carve = [&](int x, int y) {
        rows[y][x] = EMPTY_TILE;
        if (x > 1 && rows[y][x - 1] == WALL_TILE) frontier.emplace_back(x - 1, y);
        if (x < m_width - 2 && rows[y][x + 1] == WALL_TILE) frontier.emplace_back(x + 1, y);
        if (y > 1 && rows[y - 1][x] == WALL_TILE) frontier.emplace_back(x, y - 1);
        if (y < m_height - 2 && rows[y + 1][x] == WALL_TILE) frontier.emplace_back(x, y + 1);
    };

    const int cx = m_width / 2;
    const int cy = m_height / 2;
    std::size_t floors = 0;
    for (int y = cy - centerRadius; y <= cy + centerRadius; ++y) {
        for (int x = cx - centerRadius; x <= cx + centerRadius; ++x) {
            carve(x, y);
            ++floors;
        }
    }

    while (floors < target && !frontier.empty()) {
        const auto index = std::uniform_int_distribution<std::size_t>(0, frontier.size() - 1)(random);
        const auto [x, y] = frontier[index];
        frontier[index] = frontier.back();
        frontier.pop_back();
        if (rows[y][x] == EMPTY_TILE) continue;
        carve(x, y);
        ++floors;
    }

    m_rows = std::move(rows);
}

const TileMap::Matrix& Dungeon2D::rows() const noexcept { return m_rows; }
