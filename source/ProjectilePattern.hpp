#pragma once

#include "Vector2D.hpp"

#include <cstddef>
#include <vector>

enum class ProjectilePattern { Standard, Burst, Heavy, Circle, Triangle, Square, Spread, Parallel };
inline constexpr std::size_t projectilePatternCount = 8;

struct ProjectileRay {
    Vector2D position;
    Vector2D direction;
};

// Geometria pura em coordenadas de mundo, independente da arma e da apresentacao.
[[nodiscard]] std::vector<ProjectileRay> projectilePatternRays(ProjectilePattern pattern,
    Vector2D muzzle, Vector2D direction, int count, float spreadDegrees, float formationRadius);
[[nodiscard]] const char* patternName(ProjectilePattern pattern) noexcept;
