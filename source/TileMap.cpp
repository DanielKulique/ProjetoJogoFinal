// Implementa leitura CSV transacional, consultas de ocupacao e conversao para pixels.
#include "TileMap.hpp"

#include "Dungeon2D.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

TileMap::TileMap(Matrix rows) {
    if (rows.empty() || rows.front().empty()) {
        throw std::invalid_argument("Mapa vazio");
    }
    const auto maxDimension = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (rows.size() > maxDimension || rows.front().size() > maxDimension) {
        throw std::invalid_argument("Dimensoes do mapa excedem o limite de indices");
    }
    for (const auto& row : rows) {
        if (row.size() != rows.front().size() ||
            row.find_first_not_of(".#") != std::string::npos) {
            throw std::invalid_argument("Mapa deve ser retangular e conter somente . e #");
        }
    }
    m_rows = std::move(rows);
}

bool TileMap::loadCSV(const std::string& path, std::string* error) {
    const auto fail = [&](const std::string& message) {
        if (error) *error = path + ": " + message;
        return false;
    };
    std::ifstream input(path);
    if (!input) return fail("nao foi possivel abrir o arquivo");

    Matrix rows;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream cells(line);
        std::string token;
        std::string row;
        while (std::getline(cells, token, ',')) {
            const auto first = token.find_first_not_of(" \t\r");
            const auto last = token.find_last_not_of(" \t\r");
            if (first == std::string::npos || first != last ||
                (token[first] != '0' && token[first] != '1')) {
                return fail("linha " + std::to_string(rows.size() + 1) +
                            ": use apenas 0 e 1 separados por virgulas");
            }
            row += token[first] == '0' ? EMPTY_TILE : WALL_TILE;
        }
        const auto last = line.find_last_not_of(" \t\r");
        if (row.empty() || last == std::string::npos || line[last] == ',') {
            return fail("linha " + std::to_string(rows.size() + 1) + ": celula ausente");
        }
        rows.push_back(std::move(row));
    }
    if (input.bad()) return fail("erro durante a leitura");
    try {
        TileMap candidate(std::move(rows));
        m_rows = std::move(candidate.m_rows);
    } catch (const std::invalid_argument& exception) {
        return fail(exception.what());
    }
    if (error) error->clear();
    return true;
}

void TileMap::loadFromDungeon(const Dungeon2D& dungeon) {
    TileMap candidate(dungeon.rows()); // Valida inclusive generate() ainda nao chamado.
    m_rows = std::move(candidate.m_rows);
}

int TileMap::width() const noexcept {
    return m_rows.empty() ? 0 : static_cast<int>(m_rows.front().size());
}

int TileMap::height() const noexcept { return static_cast<int>(m_rows.size()); }

bool TileMap::contains(int col, int row) const noexcept {
    return col >= 0 && row >= 0 && col < width() && row < height();
}

char TileMap::at(int col, int row) const {
    if (!contains(col, row)) throw std::out_of_range("Celula fora do mapa");
    return m_rows[row][col];
}

bool TileMap::isWall(int col, int row) const noexcept { return !isFree(col, row); }

bool TileMap::isFree(int col, int row) const noexcept {
    return contains(col, row) && m_rows[row][col] == EMPTY_TILE;
}

const TileMap::Matrix& TileMap::rows() const noexcept { return m_rows; }

Vector2D TileMap::cellCenter(int col, int row) const {
    if (!contains(col, row)) throw std::out_of_range("Celula fora do mapa");
    return {(static_cast<float>(col) + 0.5f) * TILE_SIZE,
            (static_cast<float>(row) + 0.5f) * TILE_SIZE};
}

bool TileMap::isFreeAtWorld(const Vector2D& position) const noexcept {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
        position.x < 0 || position.y < 0 ||
        static_cast<double>(position.x) >= static_cast<double>(width()) * TILE_SIZE ||
        static_cast<double>(position.y) >= static_cast<double>(height()) * TILE_SIZE) {
        return false;
    }
    return isFree(static_cast<int>(std::floor(position.x / TILE_SIZE)),
                  static_cast<int>(std::floor(position.y / TILE_SIZE)));
}

bool TileMap::isAreaFree(const AABB& area) const noexcept {
    if (!std::isfinite(area.min.x) || !std::isfinite(area.min.y) ||
        !std::isfinite(area.max.x) || !std::isfinite(area.max.y) ||
        area.min.x < 0 || area.min.y < 0 || area.min.x >= area.max.x || area.min.y >= area.max.y ||
        area.max.x > width() * TILE_SIZE || area.max.y > height() * TILE_SIZE) return false;
    // A area pode tocar uma parede sem penetrar nela.
    const int left = static_cast<int>(area.min.x / TILE_SIZE);
    const int top = static_cast<int>(area.min.y / TILE_SIZE);
    const int right = static_cast<int>(std::nextafter(area.max.x, 0.0f) / TILE_SIZE);
    const int bottom = static_cast<int>(std::nextafter(area.max.y, 0.0f) / TILE_SIZE);
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) if (!isFree(x, y)) return false;
    }
    return true;
}
