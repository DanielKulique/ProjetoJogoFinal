// Possui a arena e coordena regras de movimento, combate, rounds e portal.
#pragma once

#include "TileMap.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "WaveManager.hpp"
#include "Portal.hpp"
#include "GameConfig.hpp"

#include <optional>
#include <random>

#include <vector>
#include <memory>

struct GameInput {
    Vector2D movement;
    Vector2D aimTarget;
    bool fire{false};
};

enum class GameState { Playing, RoundCompleted, GameOver };

struct Explosion {
    Vector2D position;
    float radius;
    float remainingLife;
};

class Game {
public:
    explicit Game(TileMap map, WaveConfig waves = {});
    Game(TileMap map, GameConfig config);
    [[nodiscard]] const TileMap& map() const noexcept;
    [[nodiscard]] Vector2D initialCenter() const noexcept;

    Player& player() noexcept;
    const Player& player() const noexcept;
    void update(float dt, const GameInput& input);
    [[nodiscard]] const std::vector<Projectile>& projectiles() const noexcept { return m_projectiles; }
    [[nodiscard]] const std::vector<std::unique_ptr<Enemy>>& enemies() const noexcept { return m_enemies; }
    [[nodiscard]] const WaveManager& waves() const noexcept { return m_waves; }
    [[nodiscard]] std::size_t enemiesRemaining() const noexcept { return m_waves.remaining(m_enemies.size()); }
    [[nodiscard]] const Portal& portal() const noexcept { return m_portal; }
    [[nodiscard]] GameState state() const noexcept { return m_state; }
    void restart();
    [[nodiscard]] const GameConfig& config() const noexcept { return m_config; }
    [[nodiscard]] const std::vector<Asteroid>& asteroids() const noexcept { return m_asteroids; }
    [[nodiscard]] const std::vector<Pickup>& pickups() const noexcept { return m_pickups; }
    [[nodiscard]] const std::optional<GravityField>& gravityField() const noexcept { return m_gravityField; }
    [[nodiscard]] const std::vector<Explosion>& explosions() const noexcept { return m_explosions; }
    [[nodiscard]] double gameplayTime() const noexcept { return m_gameplayTime; }

private:
    friend struct GameTestAccess;
    GameConfig m_config;
    std::mt19937 m_random;
    std::vector<Asteroid> m_asteroids;
    std::vector<Pickup> m_pickups;
    std::optional<GravityField> m_gravityField;
    std::vector<Explosion> m_explosions;
    std::vector<Vector2D> m_freePositions;
    double m_gameplayTime{0};
    double m_asteroidTimer{0};
    double m_gravityTimer{0};
    TileMap m_map;
    Vector2D m_initialCenter;
    Player m_player;
    std::vector<Projectile> m_projectiles;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    WaveManager m_waves;
    Portal m_portal;
    GameState m_state{GameState::Playing};
    std::vector<Vector2D> m_spawnCandidates;
    std::size_t m_spawnCursor{0};
    bool m_hasWalls{false};
    std::optional<EnemyKind> m_pendingEnemyKind;
    void prepareArena();
    void spawnEnemy();
    void constrainMovement(Entity& entity, Vector2D previous) const;
    void resolveProjectiles(float dt);
    void spawnEnvironment(float dt);
    void updateEnvironment(float dt);
    void resolveContacts();
    void explode(Enemy& enemy);
    void fragmentAsteroids();
    [[nodiscard]] bool positionAvailable(const AABB& bounds) const;
    [[nodiscard]] std::optional<Vector2D> findFreePosition(const Collision2D& shape);
    [[nodiscard]] std::optional<PickupKind> rollDrop();
    void tryDrop();
};
