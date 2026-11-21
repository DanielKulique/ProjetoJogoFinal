#include "ProjectilePattern.hpp"
#include "Transform2D.hpp"

#include <cmath>
#include <stdexcept>

namespace { constexpr float pi = 3.14159265358979323846f; }

const char* patternName(ProjectilePattern pattern) noexcept {
    constexpr const char* names[]{"Padrao", "Rajada", "Pesado", "Circular", "Triangular", "Quadrado", "Espalhado", "Paralelo"};
    const auto index = static_cast<std::size_t>(pattern);
    return index < projectilePatternCount ? names[index] : "Desconhecido";
}

std::vector<ProjectileRay> projectilePatternRays(ProjectilePattern pattern,
    Vector2D muzzle, Vector2D direction, int count, float spreadDegrees, float formationRadius) {
    if (static_cast<std::size_t>(pattern) >= projectilePatternCount || count <= 0 || count > 64 ||
        direction.length_squared() <= EPSILON * EPSILON || !std::isfinite(spreadDegrees) ||
        !std::isfinite(formationRadius) || formationRadius < 0) {
        throw std::invalid_argument("Geometria de disparo invalida");
    }
    direction = direction.normalized();
    const float aim = std::atan2(direction.y, direction.x);
    const auto frame = Transform2D::rotation(aim) * Transform2D::translation(muzzle.x, muzzle.y);
    const Vector2D side{-direction.y, direction.x};
    std::vector<ProjectileRay> rays;
    rays.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        Vector2D position = muzzle, heading = direction;
        if (pattern == ProjectilePattern::Circle) {
            const float angle = 2 * pi * i / count;
            const Vector2D radial{std::cos(angle), std::sin(angle)};
            position = frame.transform_point(radial * formationRadius);
            heading = frame.transform_vector(radial);
        } else if (pattern == ProjectilePattern::Triangle || pattern == ProjectilePattern::Square) {
            const float angle = -pi * 0.5f + 2 * pi * i / count;
            position = frame.transform_point(Vector2D{std::cos(angle), std::sin(angle)} * formationRadius);
        } else if (pattern == ProjectilePattern::Spread) {
            const float angle = count == 1 ? 0 : (static_cast<float>(i) / (count - 1) - 0.5f) * spreadDegrees * pi / 180;
            heading = Transform2D::rotation(angle).transform_vector(direction);
        } else if (pattern == ProjectilePattern::Parallel) {
            const float offset = count == 1 ? 0 : (static_cast<float>(i) / (count - 1) - 0.5f) * 2 * formationRadius;
            position += side * offset;
        }
        rays.push_back({position, heading});
    }
    return rays;
}
