#include "Collision2D.hpp"

#include <algorithm>
#include <cmath>

std::optional<float> Collision2D::firstIntersection(Vector2D from, Vector2D to,
                                                  const AABB& target) const noexcept {
    const auto low = target.min - halfExtents;
    const auto high = target.max + halfExtents;
    const auto delta = to - from;
    float entry = 0, exit = 1;
    const auto axis = [&](float start, float movement, float minimum, float maximum) {
        if (std::abs(movement) <= EPSILON) return start >= minimum && start <= maximum;
        float first = (minimum - start) / movement;
        float last = (maximum - start) / movement;
        if (first > last) std::swap(first, last);
        entry = std::max(entry, first);
        exit = std::min(exit, last);
        return entry <= exit;
    };
    if (!axis(from.x, delta.x, low.x, high.x) || !axis(from.y, delta.y, low.y, high.y)) {
        return std::nullopt;
    }
    return entry;
}

bool AABB::intersects(const AABB& other) const noexcept {
    return min.x <= other.max.x && max.x >= other.min.x &&
           min.y <= other.max.y && max.y >= other.min.y;
}

AABB Collision2D::bounds(const Vector2D& position) const noexcept {
    return {position - halfExtents, position + halfExtents};
}
