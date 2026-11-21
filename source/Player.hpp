#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Entity.hpp"
#include "Weapon.hpp"

#include <array>

struct PlayerConfig {
    // Indexado pelo numero de motores restantes.
    std::array<float, 5> engineSpeeds{{0, 130, 170, 210, 250}};
    float damageInvulnerability{1.25f};
    float driftDamping{2.0f};
    float maxDrift{100.0f};
};

class Player : public Entity {
public:
    explicit Player(PlayerConfig config = {}, WeaponConfig weapon = {});
    ~Player() override = default;

    void update(float dt) noexcept;
    void setMovementDirection(const Vector2D& direction);
    void aimAt(const Vector2D& target) noexcept;
    [[nodiscard]] float angleDegrees() const noexcept;
    [[nodiscard]] Vector2D aimDirection() const noexcept;
    Weapon& weapon() noexcept { return m_weapon; }
    const Weapon& weapon() const noexcept { return m_weapon; }
    bool takeDamage(int amount) noexcept;
    bool restoreEngine() noexcept;
    [[nodiscard]] int engines() const noexcept { return m_engines; }
    // Aliases mantidos para consumidores da interface de vida anterior.
    [[nodiscard]] int health() const noexcept { return m_engines; }
    [[nodiscard]] int maxHealth() const noexcept { return 4; }
    [[nodiscard]] bool invulnerable() const noexcept { return m_damageCooldown > EPSILON; }
    [[nodiscard]] float moveSpeed() const noexcept { return m_config.engineSpeeds[m_engines]; }
    [[nodiscard]] const PlayerConfig& config() const noexcept { return m_config; }

private:
    PlayerConfig m_config;
    Weapon m_weapon;
    int m_engines{4};
    Vector2D m_movementDirection;
    Vector2D m_drift;
    float m_damageCooldown{0};
    float m_angleDegrees{-90.0f}; // Frente inicial para cima; Y cresce para baixo.
};

#endif
