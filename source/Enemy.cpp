#include "Enemy.hpp"
#include "AStar.hpp"

#include <algorithm>
#include <stdexcept>
#include <cmath>

Enemy::Enemy(Vector2D position, EnemyConfig config, EnemyKind kind, std::uint32_t seed)
    : m_config(config), m_kind(kind),
      m_state(kind == EnemyKind::Advanced ? EnemyState::Chasing : EnemyState::Wandering),
      m_health(config.health), m_random(seed), m_wanderTarget(position) {
    const float positive[]{config.speed, config.attackInterval, config.detectionRadius, config.lossRadius,
        config.wanderInterval, config.wanderRadius, config.wanderSpeedFactor, config.normalHalfExtent,
        config.advancedHalfExtent, config.explosionRadius, config.advancedHealthMultiplier,
        config.advancedSpeedMultiplier};
    if (config.health <= 0 || config.damage <= 0 || config.lossRadius < config.detectionRadius ||
        config.advancedHalfExtent <= config.normalHalfExtent ||
        !std::isfinite(config.driftDamping) || config.driftDamping < 0 ||
        !std::isfinite(config.maxDrift) || config.maxDrift < 0) {
        throw std::invalid_argument("Configuracao de inimigo invalida");
    }
    for (float value : positive) {
        if (!std::isfinite(value) || value <= 0) throw std::invalid_argument("Configuracao de inimigo invalida");
    }
    body.position = position;
    const float extent = kind == EnemyKind::Advanced ? config.advancedHalfExtent : config.normalHalfExtent;
    collision.halfExtents = {extent, extent};
}

Vector2D Enemy::decideTarget(float dt, Vector2D player, const TileMap* map) {
    const auto previous = m_state;
    if (m_kind == EnemyKind::Advanced) {
        m_state = EnemyState::Chasing;
    } else {
        const float distanceSquared = (player - body.position).length_squared();
        if (m_state == EnemyState::Wandering && distanceSquared <= m_config.detectionRadius * m_config.detectionRadius) {
            m_state = EnemyState::Chasing;
        } else if (m_state == EnemyState::Chasing && distanceSquared > m_config.lossRadius * m_config.lossRadius) {
            m_state = EnemyState::Wandering;
            m_wanderTimer = 0;
        }
    }
    if (m_state != previous) m_path.clear();
    if (m_state == EnemyState::Chasing) return player;

    m_wanderTimer -= dt;
    if (m_wanderTimer <= EPSILON || (m_wanderTarget - body.position).length_squared() < 4) {
        // Apenas o RNG local e a arena participam desta decisao; nao usa o alvo distante.
        constexpr double range = static_cast<double>(std::mt19937::max()) + 1.0;
        const float angle = static_cast<float>(m_random() / range * 6.28318530717958647692);
        m_wanderTarget = body.position + Vector2D{std::cos(angle), std::sin(angle)} * m_config.wanderRadius;
        if (map) {
            m_wanderTarget = body.position;
            for (int attempt = 0; attempt < 64; ++attempt) {
                const int x = static_cast<int>(m_random() % static_cast<unsigned>(map->width()));
                const int y = static_cast<int>(m_random() % static_cast<unsigned>(map->height()));
                if (!map->isFree(x, y)) continue;
                const auto candidate = map->cellCenter(x, y);
                if ((candidate - body.position).length_squared() > m_config.wanderRadius * m_config.wanderRadius ||
                    !map->isAreaFree(collision.bounds(candidate))) continue;
                m_wanderTarget = candidate;
                break;
            }
        }
        m_wanderTimer = m_config.wanderInterval;
    }
    return m_wanderTarget;
}

void Enemy::update(float dt, Vector2D target, const TileMap* map) {
    if (!active || !std::isfinite(dt) || dt <= 0) return;
    if (map && (map->width() == 0 || map->height() == 0)) map = nullptr;
    m_attackCooldown = std::max(0.0f, m_attackCooldown - dt);
    target = decideTarget(dt, target, map);
    if (map) target = steeringTarget(dt, target, *map);
    const auto direction = target - body.position;
    const float speed = m_config.speed * (m_state == EnemyState::Wandering ? m_config.wanderSpeedFactor : 1.0f);
    const auto commanded = direction.length_squared() > EPSILON * EPSILON
        ? direction.normalized() * std::min(speed, direction.length() / dt) : Vector2D{};
    m_drift += body.acceleration * dt;
    m_drift *= std::exp(-m_config.driftDamping * dt);
    if (m_drift.length_squared() > m_config.maxDrift * m_config.maxDrift) {
        m_drift *= m_config.maxDrift / m_drift.length();
    }
    body.acceleration = {};
    body.velocity = commanded + m_drift;
    body.integrate(dt);
}

Vector2D Enemy::steeringTarget(float dt, Vector2D target, const TileMap& map) {
    const Vector2D start{std::floor(body.position.x / TILE_SIZE), std::floor(body.position.y / TILE_SIZE)};
    const Vector2D goal{std::floor(target.x / TILE_SIZE), std::floor(target.y / TILE_SIZE)};
    m_pathCooldown = std::max(0.0f, m_pathCooldown - dt);
    if (start.equals(goal)) {
        m_path.clear();
        return target;
    }
    if (m_path.empty() || (m_pathCooldown <= EPSILON && !goal.equals(m_pathGoal))) {
        const auto cells = findPathAStar(map.rows(), start, goal);
        m_path.clear();
        for (const auto& cell : cells) {
            m_path.push_back(map.cellCenter(static_cast<int>(cell.x), static_cast<int>(cell.y)));
        }
        m_nextWaypoint = 0;
        m_pathGoal = goal;
        m_pathCooldown = 0.4f;
    }
    if (m_path.empty()) return body.position;
    while (m_nextWaypoint + 1 < m_path.size() &&
           (m_path[m_nextWaypoint] - body.position).length() <= std::max(2.0f, m_config.speed * dt)) {
        ++m_nextWaypoint;
    }
    return m_path[m_nextWaypoint];
}

void Enemy::takeDamage(int amount) noexcept {
    if (amount <= 0 || !active) return;
    m_health = std::max(0, m_health - amount);
    active = m_health > 0;
}

int Enemy::tryAttack(const Entity& target) noexcept {
    if (!active || !target.active || m_attackCooldown > EPSILON || !bounds().intersects(target.bounds())) {
        return 0;
    }
    m_attackCooldown = m_config.attackInterval;
    return m_config.damage;
}
