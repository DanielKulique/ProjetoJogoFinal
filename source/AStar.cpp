#include "AStar.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <utility>

namespace {
struct OpenNode {
    std::size_t x;
    std::size_t y;
    std::size_t g; // Custo conhecido desde o inicio.
    std::size_t h; // Estimativa ate o destino.
};

struct LowerPriority {
    bool operator()(const OpenNode& a, const OpenNode& b) const noexcept {
        const std::size_t fa = a.g + a.h;
        const std::size_t fb = b.g + b.h;
        return fa != fb ? fa > fb : a.h > b.h;
    }
};
} // namespace

std::vector<Vector2D> findPathAStar(const std::vector<std::string>& maze,
                                  const Vector2D& start, const Vector2D& goal) {
    if (maze.empty() || maze.front().empty()) {
        return {};
    }
    const std::size_t height = maze.size();
    const std::size_t width = maze.front().size();
    for (const auto& row : maze) {
        if (row.size() != width || row.find_first_not_of(".#") != std::string::npos) {
            return {};
        }
    }

    const auto validPosition = [&](const Vector2D& position) {
        return std::isfinite(position.x) && std::isfinite(position.y) &&
               position.x >= 0.0f && position.y >= 0.0f &&
               std::floor(position.x) == position.x && std::floor(position.y) == position.y &&
               static_cast<double>(position.x) < static_cast<double>(width) &&
               static_cast<double>(position.y) < static_cast<double>(height);
    };
    if (!validPosition(start) || !validPosition(goal)) {
        return {};
    }

    const auto startX = static_cast<std::size_t>(start.x);
    const auto startY = static_cast<std::size_t>(start.y);
    const auto goalX = static_cast<std::size_t>(goal.x);
    const auto goalY = static_cast<std::size_t>(goal.y);
    if (maze[startY][startX] == '#' || maze[goalY][goalX] == '#') {
        return {};
    }

    const auto heuristic = [&](std::size_t x, std::size_t y) {
        const std::size_t dx = x > goalX ? x - goalX : goalX - x;
        const std::size_t dy = y > goalY ? y - goalY : goalY - y;
        return dx + dy;
    };

    const auto infinity = std::numeric_limits<std::size_t>::max();
    std::vector<std::vector<std::size_t>> costs(
        height, std::vector<std::size_t>(width, infinity));
    using Cell = std::pair<std::size_t, std::size_t>;
    std::vector<std::vector<Cell>> parents(height, std::vector<Cell>(width));
    std::priority_queue<OpenNode, std::vector<OpenNode>, LowerPriority> open;

    costs[startY][startX] = 0;
    open.push({startX, startY, 0, heuristic(startX, startY)});
    while (!open.empty()) {
        const OpenNode current = open.top();
        open.pop();
        // Uma rota melhor pode ter substituido esta entrada na fila.
        if (current.g != costs[current.y][current.x]) {
            continue;
        }
        if (current.x == goalX && current.y == goalY) {
            std::vector<Vector2D> path;
            Cell cell{goalX, goalY};
            while (true) {
                path.emplace_back(static_cast<float>(cell.first), static_cast<float>(cell.second));
                if (cell == Cell{startX, startY}) {
                    break;
                }
                cell = parents[cell.second][cell.first];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        const auto visit = [&](std::size_t x, std::size_t y) {
            const std::size_t candidateG = current.g + 1;
            if (maze[y][x] == '#' || candidateG >= costs[y][x]) {
                return;
            }
            costs[y][x] = candidateG;
            parents[y][x] = {current.x, current.y};
            open.push({x, y, candidateG, heuristic(x, y)});
        };
        if (current.x + 1 < width) {
            visit(current.x + 1, current.y);
        }
        if (current.y + 1 < height) {
            visit(current.x, current.y + 1);
        }
        if (current.x > 0) {
            visit(current.x - 1, current.y);
        }
        if (current.y > 0) {
            visit(current.x, current.y - 1);
        }
    }
    return {}; // A fronteira foi esgotada sem alcancar o destino.
}
