// Regras da partida, sem acesso a janela, entrada SFML ou renderizacao.
#include "Game.hpp"

#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cmath>
#include <queue>
#include <limits>
#include "Random.hpp"

namespace {
GameConfig withWaves(WaveConfig waves) {
    GameConfig config;
    config.waves = waves;
    return config;
}
bool circleIntersects(Vector2D center, float radius, const AABB& bounds) {
    const Vector2D nearest{std::clamp(center.x, bounds.min.x, bounds.max.x),
                           std::clamp(center.y, bounds.min.y, bounds.max.y)};
    return (nearest - center).length_squared() <= radius * radius;
}
}

void Game::update(float dt, const GameInput& input) {
    if (!std::isfinite(dt) || dt <= 0 || m_state == GameState::GameOver) return;
    m_gameplayTime += dt;
    spawnEnvironment(dt);
    m_player.body.acceleration = m_gravityField
        ? m_gravityField->accelerationAt(m_player.body.position) : Vector2D{};
    m_player.setMovementDirection(input.movement);
    const auto previous = m_player.body.position;
    m_player.update(dt);
    constrainMovement(m_player, previous);
    m_player.aimAt(input.aimTarget);
    if (m_state == GameState::Playing) {
        m_waves.update(dt);
        spawnEnemy();
        if (input.fire) {
            auto volley = m_player.weapon().fireVolley(m_player.body.position, m_player.aimDirection());
            for (auto& shot : volley) {
                if (m_projectiles.size() >= m_config.maxProjectiles) break;
                if (m_map.isAreaFree(shot.bounds())) m_projectiles.push_back(std::move(shot));
            }
        }
        for (auto& enemy : m_enemies) {
            if (!enemy->active) continue;
            enemy->body.acceleration = m_gravityField
                ? m_gravityField->accelerationAt(enemy->body.position) : Vector2D{};
            const auto oldPosition = enemy->body.position;
            enemy->update(dt, m_player.body.position, m_hasWalls ? &m_map : nullptr);
            constrainMovement(*enemy, oldPosition);
        }
    }
    updateEnvironment(dt);
    resolveProjectiles(dt);
    resolveContacts();
    if (m_gravityField) {
        m_gravityField->update(dt);
        if (!m_gravityField->active) m_gravityField.reset();
    }
    m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(),
        [](const auto& enemy) { return !enemy->active; }), m_enemies.end());
    fragmentAsteroids();
    m_pickups.erase(std::remove_if(m_pickups.begin(), m_pickups.end(),
        [](const Pickup& item) { return !item.active; }), m_pickups.end());
    if (!m_player.active) {
        m_state = GameState::GameOver;
        m_player.body.velocity = {};
        m_projectiles.clear();
        m_portal.deactivate();
    } else if (m_state == GameState::Playing && m_waves.completed(m_enemies.size())) {
        m_state = GameState::RoundCompleted;
        m_projectiles.clear();
        m_portal.activate(m_player);
    } else if (m_state == GameState::RoundCompleted && m_portal.tryEnter(m_player)) {
        m_waves.startNextRound();
        m_portal.deactivate();
        m_state = GameState::Playing;
    }
}

void Game::restart() {
    m_enemies.clear();
    m_projectiles.clear();
    m_asteroids.clear();
    m_pickups.clear();
    m_explosions.clear();
    m_gravityField.reset();
    m_player = Player(m_config.player, m_config.weapon);
    m_player.body.position = m_initialCenter;
    m_waves.reset();
    m_portal.deactivate();
    m_spawnCursor = 0;
    m_pendingEnemyKind.reset();
    m_gameplayTime = m_asteroidTimer = m_gravityTimer = 0;
    m_random.seed(m_config.seed);
    m_state = GameState::Playing;
}

Game::Game(TileMap map, WaveConfig waves) : Game(std::move(map), withWaves(waves)) {}

Game::Game(TileMap map, GameConfig config)
    : m_config(std::move(config)), m_random(m_config.seed), m_map(std::move(map)),
      m_player(m_config.player, m_config.weapon), m_waves(m_config.waves) {
    m_config.validate();
    const int col = m_map.width() / 2;
    const int row = m_map.height() / 2;
    if (m_map.isFree(col, row)) {
        m_initialCenter = m_map.cellCenter(col, row);
        m_player.body.position = m_initialCenter;
        prepareArena();
        return;
    }
    for (int y = 0; y < m_map.height(); ++y) {
        for (int x = 0; x < m_map.width(); ++x) {
            if (m_map.isFree(x, y)) {
                m_initialCenter = m_map.cellCenter(x, y);
                m_player.body.position = m_initialCenter;
                prepareArena();
                return;
            }
        }
    }
    throw std::invalid_argument("O cenario deve conter pelo menos uma celula livre");
}

const TileMap& Game::map() const noexcept { return m_map; }

Vector2D Game::initialCenter() const noexcept { return m_initialCenter; }

Player& Game::player() noexcept { return m_player; }
const Player& Game::player() const noexcept { return m_player; }

void Game::constrainMovement(Entity& entity, Vector2D previous) const {
    const auto destination = entity.body.position;
    entity.body.position = previous;
    const auto delta = destination - previous;
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::abs(delta.x), std::abs(delta.y)) / 8.0f)));
    const auto step = delta / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        auto next = entity.body.position;
        next.x = std::clamp(next.x + step.x, entity.collision.halfExtents.x,
                            m_map.width() * TILE_SIZE - entity.collision.halfExtents.x);
        if (m_map.isAreaFree(entity.collision.bounds(next))) entity.body.position = next;
        next = entity.body.position;
        next.y = std::clamp(next.y + step.y, entity.collision.halfExtents.y,
                            m_map.height() * TILE_SIZE - entity.collision.halfExtents.y);
        if (m_map.isAreaFree(entity.collision.bounds(next))) entity.body.position = next;
    }
}

void Game::resolveProjectiles(float dt) {
    for (auto& shot : m_projectiles) {
        if (!shot.active) continue;
        shot.body.acceleration = m_gravityField
            ? m_gravityField->accelerationAt(shot.body.position) : Vector2D{};
        const bool inField = m_gravityField &&
            (shot.body.position - m_gravityField->body.position).length_squared() <
                m_gravityField->radius() * m_gravityField->radius();
        if (inField) shot.body.acceleration -= shot.body.velocity * m_config.gravity.damping;
        shot.update(dt);
        if (inField) {
            const float limit = m_player.weapon().config().projectileSpeed + m_config.gravity.maxDrift;
            if (shot.body.velocity.length_squared() > limit * limit)
                shot.body.velocity = shot.body.velocity.normalized() * limit;
        }
        const auto from = shot.previousPosition();
        const auto to = shot.body.position;
        float firstHit = 2.0f;
        Enemy* victim = nullptr;
        Asteroid* rockVictim = nullptr;
        // Varre apenas as celulas cobertas pelo percurso, incluindo o contorno externo.
        const auto extent = shot.collision.halfExtents;
        const int left = std::clamp(static_cast<int>(std::floor((std::min(from.x, to.x) - extent.x) / TILE_SIZE)), -1, m_map.width());
        const int right = std::clamp(static_cast<int>(std::floor((std::max(from.x, to.x) + extent.x) / TILE_SIZE)), -1, m_map.width());
        const int top = std::clamp(static_cast<int>(std::floor((std::min(from.y, to.y) - extent.y) / TILE_SIZE)), -1, m_map.height());
        const int bottom = std::clamp(static_cast<int>(std::floor((std::max(from.y, to.y) + extent.y) / TILE_SIZE)), -1, m_map.height());
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                if (!m_map.isWall(x, y)) continue;
                const AABB tile{{x * TILE_SIZE, y * TILE_SIZE}, {(x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE}};
                const auto hit = shot.collision.firstIntersection(from, to, tile);
                if (hit && *hit < firstHit) firstHit = *hit;
            }
        }
        for (auto& enemy : m_enemies) {
            if (!enemy->active) continue;
            const auto hit = shot.collision.firstIntersection(from, to, enemy->bounds());
            if (hit && *hit < firstHit) {
                firstHit = *hit;
                victim = enemy.get();
            }
        }
        for (auto& rock : m_asteroids) {
            if (!rock.active || !rock.collisionEnabled()) continue;
            const auto hit = shot.collision.firstIntersection(from, to, rock.bounds());
            if (hit && *hit < firstHit) {
                firstHit = *hit;
                victim = nullptr;
                rockVictim = &rock;
            }
        }
        if (firstHit <= 1) {
            if (victim) {
                victim->takeDamage(shot.damage());
                if (!victim->active && victim->kind() == EnemyKind::Advanced) tryDrop();
            }
            if (rockVictim) rockVictim->takeDamage(shot.damage());
            shot.active = false; // Consumo unico, mesmo com inimigos sobrepostos.
        }
    }
    m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(),
        [](const Projectile& shot) { return !shot.active; }), m_projectiles.end());
}

void Game::prepareArena() {
    m_hasWalls = std::any_of(m_map.rows().begin(), m_map.rows().end(), [](const std::string& row) {
        return row.find(WALL_TILE) != std::string::npos;
    });
    // BFS restringe spawns ao componente de piso alcancavel pelo jogador.
    const int width = m_map.width(), height = m_map.height();
    std::vector<bool> seen(static_cast<std::size_t>(width) * height, false);
    std::queue<std::pair<int, int>> queue;
    const int cx = static_cast<int>(m_initialCenter.x / TILE_SIZE);
    const int cy = static_cast<int>(m_initialCenter.y / TILE_SIZE);
    queue.emplace(cx, cy);
    seen[static_cast<std::size_t>(cy) * width + cx] = true;
    std::vector<std::pair<Vector2D, int>> candidates;
    int nearestEdge = std::numeric_limits<int>::max();
    constexpr int offsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    Collision2D shape;
    shape.halfExtents = {18, 18};
    while (!queue.empty()) {
        const auto [x, y] = queue.front();
        queue.pop();
        const auto position = m_map.cellCenter(x, y);
        if (m_map.isAreaFree(shape.bounds(position))) {
            const int edge = std::min({x, y, width - 1 - x, height - 1 - y});
            m_freePositions.push_back(position);
            nearestEdge = std::min(nearestEdge, edge);
            candidates.emplace_back(position, edge);
        }
        for (const auto& offset : offsets) {
            const int nx = x + offset[0], ny = y + offset[1];
            if (!m_map.isFree(nx, ny)) continue;
            const auto index = static_cast<std::size_t>(ny) * width + nx;
            if (seen[index]) continue;
            seen[index] = true;
            queue.emplace(nx, ny);
        }
    }
    const Vector2D mapCenter{width * TILE_SIZE * 0.5f, height * TILE_SIZE * 0.5f};
    m_portal.body.position = m_initialCenter;
    float closest = std::numeric_limits<float>::max();
    // Em CSV com centro bloqueado, usa o piso alcancavel mais proximo do centro.
    for (const auto& candidate : candidates) {
        const auto position = candidate.first;
        const float distance = (position - mapCenter).length_squared();
        if (distance < closest && m_map.isAreaFree(m_portal.collision.bounds(position))) {
            closest = distance;
            m_portal.body.position = position;
        }
    }
    if (m_map.isAreaFree(m_portal.collision.bounds(mapCenter)) &&
        seen[static_cast<std::size_t>(height / 2) * width + width / 2]) {
        m_portal.body.position = mapCenter;
    }
    for (const auto& candidate : candidates) {
        if (candidate.second <= nearestEdge + 1) m_spawnCandidates.push_back(candidate.first);
    }
}

void Game::spawnEnemy() {
    if (!m_waves.readyToSpawn() || m_spawnCandidates.empty() ||
        m_enemies.size() >= m_config.waves.maxActive) return;
    // Sorteia uma vez por inimigo previsto, preservando o tipo enquanto falta espaco.
    if (!m_pendingEnemyKind) m_pendingEnemyKind = m_waves.round() >= 2 &&
        randomChance(m_random, m_config.waves.advancedChance) ? EnemyKind::Advanced : EnemyKind::Normal;
    const auto kind = *m_pendingEnemyKind;
    Collision2D shape;
    const float extent = kind == EnemyKind::Advanced ? m_config.waves.enemy.advancedHalfExtent
                                                    : m_config.waves.enemy.normalHalfExtent;
    shape.halfExtents = {extent, extent};
    for (std::size_t i = 0; i < m_spawnCandidates.size(); ++i) {
        const auto index = (m_spawnCursor + i) % m_spawnCandidates.size();
        const auto position = m_spawnCandidates[index];
        if (!positionAvailable(shape.bounds(position))) continue;
        m_enemies.push_back(std::make_unique<Enemy>(position, m_waves.enemyConfig(kind), kind, m_random()));
        m_spawnCursor = (index + m_spawnCandidates.size() / 2 + 1) % m_spawnCandidates.size();
        m_waves.confirmSpawn();
        m_pendingEnemyKind.reset();
        return;
    }
}

bool Game::positionAvailable(const AABB& bounds) const {
    if (!m_map.isAreaFree(bounds) || bounds.intersects(m_player.bounds()) ||
        (m_portal.active && bounds.intersects(m_portal.bounds()))) return false;
    for (const auto& enemy : m_enemies) if (enemy->active && bounds.intersects(enemy->bounds())) return false;
    for (const auto& rock : m_asteroids) if (rock.active && bounds.intersects(rock.bounds())) return false;
    for (const auto& item : m_pickups) if (item.active && bounds.intersects(item.bounds())) return false;
    return true;
}

std::optional<Vector2D> Game::findFreePosition(const Collision2D& shape) {
    if (m_freePositions.empty()) return std::nullopt;
    const auto start = static_cast<std::size_t>(m_random()) % m_freePositions.size();
    for (std::size_t i = 0; i < m_freePositions.size(); ++i) {
        const auto position = m_freePositions[(start + i) % m_freePositions.size()];
        if (positionAvailable(shape.bounds(position))) return position;
    }
    return std::nullopt;
}

std::optional<PickupKind> Game::rollDrop() {
    if (!randomChance(m_random, m_config.drops.chance)) return std::nullopt;
    return randomChance(m_random, m_config.drops.upgradeChance)
        ? PickupKind::WeaponUpgrade : PickupKind::Engine;
}

void Game::tryDrop() {
    const auto kind = rollDrop(); // Cada eliminacao elegivel faz uma unica decisao, mesmo sem espaco.
    if (!kind || m_pickups.size() >= m_config.pickup.maxActive) return;
    Collision2D shape;
    shape.halfExtents = {m_config.pickup.halfExtent, m_config.pickup.halfExtent};
    const auto position = findFreePosition(shape);
    if (position) m_pickups.emplace_back(*position, *kind, m_config.pickup);
}

void Game::spawnEnvironment(float dt) {
    m_asteroidTimer += dt;
    m_gravityTimer += dt;
    while (m_gravityTimer + EPSILON >= m_config.gravity.spawnInterval) {
        m_gravityTimer -= m_config.gravity.spawnInterval;
        if (m_gravityField || !randomChance(m_random, m_config.gravity.spawnChance)) continue;
        Collision2D shape;
        shape.halfExtents = {16, 16};
        if (const auto position = findFreePosition(shape)) m_gravityField.emplace(*position, m_config.gravity);
    }
    while (m_asteroidTimer + EPSILON >= m_config.asteroid.spawnInterval) {
        m_asteroidTimer -= m_config.asteroid.spawnInterval;
        if (m_asteroids.size() >= m_config.asteroid.maxActive ||
            !randomChance(m_random, m_config.asteroid.spawnChance)) continue;
        Collision2D shape;
        shape.halfExtents = {m_config.asteroid.radius, m_config.asteroid.radius};
        if (const auto position = findFreePosition(shape)) {
            const float angle = static_cast<float>(randomUnit(m_random) * 6.283185307179586);
            m_asteroids.emplace_back(*position, Vector2D{std::cos(angle), std::sin(angle)}, m_config.asteroid);
        }
    }
}

void Game::updateEnvironment(float dt) {
    for (auto& visual : m_explosions) visual.remainingLife -= dt;
    m_explosions.erase(std::remove_if(m_explosions.begin(), m_explosions.end(),
        [](const Explosion& e) { return e.remainingLife <= 0; }), m_explosions.end());
    for (auto& item : m_pickups) item.update(dt);
    for (auto& rock : m_asteroids) {
        if (!rock.active) continue;
        rock.body.acceleration = m_gravityField ? m_gravityField->accelerationAt(rock.body.position) : Vector2D{};
        const auto previous = rock.body.position;
        rock.update(dt);
        const auto destination = rock.body.position;
        constrainMovement(rock, previous);
        if (std::abs(rock.body.position.x - destination.x) > EPSILON) rock.body.velocity.x *= -1;
        if (std::abs(rock.body.position.y - destination.y) > EPSILON) rock.body.velocity.y *= -1;
    }
}

void Game::explode(Enemy& enemy) {
    if (!enemy.active || enemy.kind() != EnemyKind::Advanced) return;
    const auto center = enemy.body.position;
    const float radius = enemy.config().explosionRadius;
    enemy.takeDamage(enemy.health()); // Torna a detonacao idempotente; nao e morte por tiro.
    if (m_explosions.size() < m_config.maxExplosions)
        m_explosions.push_back({center, radius, m_config.explosionLifetime});
    if (circleIntersects(center, radius, m_player.bounds())) m_player.takeDamage(enemy.config().damage);
    for (auto& rock : m_asteroids) {
        if (rock.active && rock.collisionEnabled() && circleIntersects(center, radius, rock.bounds()))
            rock.takeDamage(rock.health());
    }
    // Aliens nao recebem dano de explosao, evitando reacoes em cadeia.
}

void Game::resolveContacts() {
    for (auto& enemy : m_enemies) {
        if (!enemy->active) continue;
        if (m_player.active && enemy->kind() == EnemyKind::Advanced &&
            enemy->bounds().intersects(m_player.bounds())) explode(*enemy);
        else m_player.takeDamage(enemy->tryAttack(m_player));
    }
    for (auto& rock : m_asteroids) {
        if (!rock.active || !rock.collisionEnabled()) continue;
        if (m_player.active && rock.bounds().intersects(m_player.bounds())) {
            m_player.takeDamage(rock.config().damage);
            rock.takeDamage(rock.health());
            continue;
        }
        for (auto& enemy : m_enemies) {
            if (!rock.active) break;
            if (!enemy->active || !rock.bounds().intersects(enemy->bounds())) continue;
            if (enemy->kind() == EnemyKind::Advanced) explode(*enemy);
            else enemy->takeDamage(enemy->health());
            rock.takeDamage(rock.health());
        }
    }
    for (auto& item : m_pickups) {
        if (!item.active || !m_player.active || !item.bounds().intersects(m_player.bounds())) continue;
        if (item.kind() == PickupKind::Engine) m_player.restoreEngine();
        else m_player.weapon().upgrade();
        item.active = false;
    }
}

void Game::fragmentAsteroids() {
    std::vector<Asteroid> fragments;
    for (auto& rock : m_asteroids) {
        if (rock.active) continue;
        auto children = rock.fragment();
        for (auto& child : children) {
            const auto aliveCount = std::count_if(m_asteroids.begin(), m_asteroids.end(),
                [](const Asteroid& asteroid) { return asteroid.active; });
            if (static_cast<std::size_t>(aliveCount) + fragments.size() >= m_config.asteroid.maxActive) break;
            bool placed = false;
            // Distribui localmente: fragmentos nao reaparecem em outra regiao da arena.
            for (int attempt = 0; attempt < 16 && !placed; ++attempt) {
                const float angle = static_cast<float>(attempt) * 6.283185307179586f / 8.0f;
                const float ring = rock.radius() + child.radius() + 4.0f + (attempt / 8) * child.radius() * 2;
                auto position = attempt == 0 ? child.body.position
                    : rock.body.position + Vector2D{std::cos(angle), std::sin(angle)} * ring;
                if (!positionAvailable(child.collision.bounds(position))) continue;
                const auto bounds = child.collision.bounds(position);
                if (std::any_of(fragments.begin(), fragments.end(), [&](const Asteroid& other) {
                    return bounds.intersects(other.bounds());
                })) continue;
                child.body.position = position;
                placed = true;
            }
            if (placed) fragments.push_back(std::move(child));
        }
    }
    m_asteroids.erase(std::remove_if(m_asteroids.begin(), m_asteroids.end(),
        [](const Asteroid& rock) { return !rock.active; }), m_asteroids.end());
    for (auto& child : fragments) m_asteroids.push_back(std::move(child));
}
