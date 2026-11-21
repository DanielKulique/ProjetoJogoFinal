#include "Game.hpp"
#include "Asteroid.hpp"
#include "GravityField.hpp"
#include "Pickup.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

// Arranjo de cenarios pelo contrato de teste; a API de producao segue observadora.
struct GameTestAccess {
    static void drainSchedule(Game& game) {
        while (game.m_waves.pending() > 0) {
            game.m_waves.update(100);
            game.m_waves.confirmSpawn();
        }
        game.m_enemies.clear();
    }
    static Enemy& addEnemy(Game& game, Vector2D position, EnemyKind kind, EnemyConfig config = {}) {
        game.m_enemies.push_back(std::make_unique<Enemy>(position, config, kind, 123));
        return *game.m_enemies.back();
    }
    static void asteroid(Game& game, Asteroid asteroid) { game.m_asteroids.push_back(std::move(asteroid)); }
    static void pickup(Game& game, Pickup pickup) { game.m_pickups.push_back(std::move(pickup)); }
    static void shot(Game& game, Projectile shot) { game.m_projectiles.push_back(std::move(shot)); }
    static void field(Game& game, GravityField field) { game.m_gravityField = std::move(field); }
    static void nextRound(Game& game) { game.m_waves.startNextRound(); }
    static EnemyKind spawnKind(Game& game) {
        if (game.m_waves.pending() == 0) game.m_waves.startNextRound();
        game.m_enemies.clear();
        game.m_waves.update(100);
        game.spawnEnemy();
        if (game.m_enemies.empty()) throw std::runtime_error("Fixture sem posicao valida de spawn");
        return game.m_enemies.front()->kind();
    }
    static auto rollDrop(Game& game) { return game.rollDrop(); }
    static void drop(Game& game) { game.tryDrop(); }
    static void peaceful(Game& game) {
        drainSchedule(game);
        game.m_state = GameState::RoundCompleted;
    }
};

namespace {
constexpr float dt = 1.0f / 60;
void require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance = 0.005f) { return std::abs(a - b) <= tolerance; }
TileMap arena() { return TileMap(TileMap::Matrix(15, std::string(19, '.'))); }
GameConfig quietConfig() {
    GameConfig config;
    config.weapon.legacyProgression = true;
    config.asteroid.spawnChance = 0;
    config.gravity.spawnChance = 0;
    return config;
}

void testEnginesAndUpgrade() {
    Player player;
    const auto config = player.config();
    require(player.engines() == 4 && !player.restoreEngine(), "Quatro motores e recuperacao limitada");
    for (int engines = 4; engines > 0; --engines) {
        player.setMovementDirection({1, 1});
        const auto previous = player.body.position;
        player.update(dt);
        require(near((player.body.position - previous).length() / dt, config.engineSpeeds[engines]),
                "Velocidade corresponde aos motores restantes");
        require(player.takeDamage(1) && !player.takeDamage(1), "Um contato nao retira varios motores");
        if (engines > 1) player.update(config.damageInvulnerability);
    }
    require(!player.active && player.engines() == 0 && near(player.moveSpeed(), 0), "Zero motores encerra nave");
    Player repaired;
    repaired.takeDamage(1);
    require(repaired.restoreEngine() && repaired.engines() == 4 && !repaired.restoreEngine(), "Motor repara um e limita quatro");
    PlayerConfig custom;
    custom.engineSpeeds[4] = 300;
    Player configurable(custom);
    configurable.setMovementDirection({1, 0});
    configurable.update(dt);
    require(near(configurable.body.position.x, 5), "Velocidades configuraveis");

    WeaponConfig legacyWeapon;
    legacyWeapon.legacyProgression = true;
    Weapon weapon(legacyWeapon);
    require(weapon.level() == 1 && weapon.fireVolley({}, {1, 0}).size() == 1, "Tiro inicial simples");
    require(weapon.upgrade() && weapon.level() == 2, "Segundo nivel de arma");
    weapon.update(1);
    require(weapon.fireVolley({}, {1, 0}).size() == 1 && weapon.cooldown() < weapon.config().cooldown,
            "Segundo nivel melhora cadencia");
    require(weapon.upgrade() && weapon.level() == 3 && !weapon.upgrade(), "Upgrade limitado ao nivel tres");
    weapon.update(1);
    const auto volley = weapon.fireVolley({}, {1, 0});
    require(volley.size() == 3, "Terceiro nivel dispara tres projeteis");
    require(std::any_of(volley.begin(), volley.end(), [](const Projectile& p) { return p.body.velocity.y < -1; }) &&
            std::any_of(volley.begin(), volley.end(), [](const Projectile& p) { return p.body.velocity.y > 1; }),
            "Espalhamento produz direcoes distintas");
    require(weapon.fireVolley({}, {1, 0}).empty(), "Uma rajada respeita cooldown compartilhado");
}

void testAlienPerception() {
    Enemy first({400, 400}, {}, EnemyKind::Normal, 55);
    Enemy second({400, 400}, {}, EnemyKind::Normal, 55);
    for (int i = 0; i < 30; ++i) {
        first.update(dt, {4000, 4000});
        second.update(dt, {-4000, -4000});
        require(first.body.position.equals(second.body.position), "Wandering nao conhece posicao distante do jogador");
        require(first.state() == EnemyState::Wandering, "Normal inicia navegando fora da percepcao");
    }
    const auto closeTarget = first.body.position + Vector2D{100, 0};
    first.update(dt, closeTarget);
    require(first.state() == EnemyState::Chasing && first.body.velocity.x > 0, "Entrada no raio inicia perseguicao");
    first.update(dt, first.body.position + Vector2D{2000, 0});
    require(first.state() == EnemyState::Wandering, "Perda do alvo retorna a navegacao");
    Enemy advanced({400, 400}, {}, EnemyKind::Advanced, 55);
    require(advanced.state() == EnemyState::Chasing, "Avancado persegue desde nascimento");
    advanced.update(dt, {4000, 400});
    require(advanced.body.velocity.x > 0 && near(advanced.body.velocity.y, 0), "Avancado ignora limite perceptivo");
    require(advanced.collision.halfExtents.x > first.collision.halfExtents.x, "Avancado possui maior area de impacto");
}

void testAsteroidFragmentation() {
    Asteroid parent({400, 400}, {1, 0});
    require(parent.collisionEnabled() && parent.fragment().empty(), "Asteroide vivo nao fragmenta");
    const auto oldPosition = parent.body.position;
    parent.update(dt);
    require(parent.body.position.x > oldPosition.x && near(parent.body.position.y, oldPosition.y), "Trajetoria inicial retilinea");
    parent.takeDamage(parent.health());
    auto children = parent.fragment();
    require(children.size() == 4 && parent.fragment().empty(), "Uma destruicao gera um unico grupo de quatro fragmentos");
    for (auto& child : children) {
        require(child.radius() < parent.radius() && child.depth() == 1 && !child.collisionEnabled(),
                "Fragmento menor recebe nivel e protecao inicial");
        require(child.body.velocity.length() <= child.config().maxSpeed + EPSILON, "Velocidade do fragmento limitada");
        child.update(child.config().fragmentGrace);
        require(child.collisionEnabled(), "Fragmento volta a colidir apos protecao");
        child.takeDamage(child.health());
        require(child.fragment().empty(), "Nivel maximo impede fragmentacao infinita");
    }
    AsteroidConfig three;
    three.fragmentCount = 3;
    Asteroid parentThree({}, {1, 0}, three);
    parentThree.takeDamage(three.health);
    require(parentThree.fragment().size() == 3, "Quantidade de fragmentos configuravel");
    Asteroid expired({}, {1, 0});
    expired.update(100);
    require(!expired.active && expired.fragment().empty(), "Expiracao nao gera fragmentos");
}

void testGravity() {
    GravityFieldConfig config;
    config.radius = 200;
    GravityField field({200, 200}, config);
    require(field.accelerationAt({200, 200}).equals({}), "Centro gravitacional sem singularidade");
    require(field.accelerationAt({401, 200}).equals({}), "Gravidade limitada ao raio");
    const auto acceleration = field.accelerationAt({100, 200});
    require(near(acceleration.x, config.accelerationMeters * config.pixelsPerMeter * 0.5f) && near(acceleration.y, 0),
            "Conversao explicita de 10m/s2 e atenuacao radial");
    RigidBody2D body;
    body.position = {100, 200};
    field.apply(body);
    require(body.position.equals({100, 200}) && body.acceleration.x > 0, "Forca nao teleporta o corpo");
    body.integrate(dt);
    require(body.position.x > 100 && body.position.x < 101, "Atracao gradual por integracao");
    Player player;
    player.body.position = {100, 200};
    Enemy enemy({100, 200}, {}, EnemyKind::Advanced);
    for (int i = 0; i < 120; ++i) {
        player.setMovementDirection({});
        field.apply(player.body);
        player.update(dt);
        field.apply(enemy.body);
        enemy.update(dt, enemy.body.position);
    }
    require(player.body.position.x > 110 && player.body.velocity.length() <= player.config().maxDrift + 1,
            "Gravidade acumula deriva limitada mesmo sem WASD");
    require(enemy.body.position.x > 110 && enemy.body.velocity.length() <= enemy.config().maxDrift + 1,
            "Gravidade afeta alien com deriva limitada");
    Asteroid rock({100, 200}, {0, 1});
    Projectile projectile({100, 200}, {0, 1}, 100, 25, 10);
    for (int i = 0; i < 60; ++i) {
        field.apply(rock.body);
        rock.update(dt);
        field.apply(projectile.body);
        projectile.update(dt);
    }
    require(rock.body.position.x > 100 && projectile.body.position.x > 100, "Gravidade afeta asteroide e projetil");
    for (int i = 0; i < 600; ++i) {
        rock.body.acceleration = {10000, 0};
        rock.update(dt);
        require(rock.body.velocity.length() <= rock.config().maxSpeed + 0.01f, "Asteroide tem limite de velocidade");
    }
    field.update(config.duration);
    require(!field.active && field.accelerationAt({100, 200}).equals({}), "Campo expira e deixa de influenciar");
}

void testDropProbabilities() {
    const auto config = quietConfig();
    Game first(arena(), config), second(arena(), config);
    int drops = 0, upgrades = 0;
    constexpr int samples = 20000;
    for (int i = 0; i < samples; ++i) {
        const auto a = GameTestAccess::rollDrop(first);
        const auto b = GameTestAccess::rollDrop(second);
        require(a == b, "Mesma semente reproduz os dois sorteios de drop");
        if (a) {
            ++drops;
            upgrades += *a == PickupKind::WeaponUpgrade ? 1 : 0;
        }
    }
    require(std::abs(static_cast<float>(drops) / samples - 0.75f) < 0.02f, "Drop converge a 75 por cento");
    require(std::abs(static_cast<float>(upgrades) / drops - 0.8f) < 0.02f, "Tipo de drop converge a 80/20 apos primeiro sorteio");
    auto always = config;
    always.drops.chance = 1;
    always.drops.upgradeChance = 0;
    Game engines(arena(), always);
    require(GameTestAccess::rollDrop(engines) == PickupKind::Engine, "Probabilidade extrema de motor");
    always.drops.chance = 0;
    Game never(arena(), always);
    require(!GameTestAccess::rollDrop(never), "Probabilidade zero nao gera item");
}

void testExplosionAndKillAttribution() {
    auto config = quietConfig();
    config.drops.chance = 1;
    config.drops.upgradeChance = 1;
    const auto projectileKill = [&](EnemyKind kind) {
        Game game(arena(), config);
        GameTestAccess::drainSchedule(game);
        const auto position = game.player().body.position;
        EnemyConfig target = game.waves().enemyConfig(kind);
        target.health = 25;
        GameTestAccess::addEnemy(game, position + Vector2D{120, 0}, kind, target);
        GameTestAccess::shot(game, Projectile(position, {1, 0}, 12000, 25, 1));
        game.update(dt, {});
        require(game.enemies().empty() && game.explosions().empty(), "Morte por tiro nao causa explosao");
        const auto expected = kind == EnemyKind::Advanced ? 1u : 0u;
        require(game.pickups().size() == expected, "Somente avancado eliminado pelo jogador gera drop");
        if (expected) {
            const auto& item = game.pickups().front();
            require(game.map().isAreaFree(item.bounds()) && !item.bounds().intersects(game.player().bounds()),
                    "Drop nasce em posicao valida fora do jogador");
        }
        for (int i = 0; i < 5; ++i) game.update(dt, {});
        require(game.pickups().size() == expected, "Morte nao duplica drops nos passos seguintes");
    };
    projectileKill(EnemyKind::Normal);
    projectileKill(EnemyKind::Advanced);

    Game game(arena(), config);
    GameTestAccess::drainSchedule(game);
    const auto position = game.player().body.position;
    GameTestAccess::addEnemy(game, position, EnemyKind::Advanced);
    auto& survivor = GameTestAccess::addEnemy(game, position + Vector2D{60, 0}, EnemyKind::Normal);
    const auto survivorHealth = survivor.health();
    Asteroid asteroid(position + Vector2D{80, 0}, {1, 0});
    GameTestAccess::asteroid(game, std::move(asteroid));
    game.update(dt, {});
    require(game.explosions().size() == 1 && game.player().engines() == 3, "Contato avancado causa uma explosao e um dano");
    require(game.pickups().empty(), "Explosao por contato nao gera drop");
    require(game.enemies().size() == 1 && game.enemies()[0]->health() == survivorHealth,
            "Explosao nao causa dano entre aliens");
    require(!game.asteroids().empty() && std::all_of(game.asteroids().begin(), game.asteroids().end(),
            [](const Asteroid& child) { return child.depth() == 1 && !child.collisionEnabled(); }),
            "Explosao fragmenta asteroide uma vez com protecao dos filhos");
    for (int i = 0; i < 5; ++i) game.update(dt, {});
    require(game.explosions().size() == 1 && game.player().engines() == 3 && game.pickups().empty(),
            "Mesmo evento nao repete explosao, dano ou drop");

    Game collision(arena(), config);
    GameTestAccess::drainSchedule(collision);
    const auto away = collision.player().body.position + Vector2D{250, 0};
    GameTestAccess::addEnemy(collision, away, EnemyKind::Advanced);
    GameTestAccess::asteroid(collision, Asteroid(away, {1, 0}));
    collision.update(dt, {});
    require(collision.explosions().size() == 1 && collision.enemies().empty() && collision.pickups().empty(),
            "Avancado explode ao tocar asteroide sem criar drop");
}

void testItemsAndRoundPreservation() {
    Game game(arena(), quietConfig());
    GameTestAccess::peaceful(game);
    const auto position = game.player().body.position;
    game.player().takeDamage(1);
    GameTestAccess::pickup(game, Pickup(position, PickupKind::Engine));
    game.update(dt, {});
    require(game.player().engines() == 4 && game.pickups().empty(), "Coleta recupera exatamente um motor");
    for (int i = 0; i < 4; ++i) {
        GameTestAccess::pickup(game, Pickup(position, PickupKind::WeaponUpgrade));
        game.update(dt, {});
    }
    require(game.player().weapon().level() == 3 && game.pickups().empty(), "Coleta limita upgrade ao nivel tres");
    Pickup expired(position + Vector2D{100, 0}, PickupKind::Engine, {dt, 12, 12});
    GameTestAccess::pickup(game, std::move(expired));
    game.update(dt, {});
    require(game.pickups().empty(), "Itens expiram sem serem coletados");

    WaveManager waves;
    require(waves.pending() == 5, "Round um introdutorio com cinco aliens");
    waves.startNextRound();
    require(waves.pending() == 7, "Round dois com sete aliens");
    waves.startNextRound();
    require(waves.pending() == 9, "Round tres com nove aliens");
    for (int i = 0; i < 100; ++i) waves.startNextRound();
    require(waves.pending() == 25 && near(waves.spawnInterval(), waves.config().minSpawnInterval),
            "Quantidade e cadencia respeitam limites seguros");
}

void testAlienDistribution() {
    auto config = quietConfig();
    config.waves.initialEnemies = 25;
    config.waves.additionalEnemies = 0;
    config.waves.enemy.speed = 1;
    Game first(arena(), config), second(arena(), config);
    GameTestAccess::nextRound(first);
    GameTestAccess::nextRound(second);
    int advanced = 0;
    constexpr int samples = 4000;
    for (int i = 0; i < samples; ++i) {
        const auto a = GameTestAccess::spawnKind(first);
        const auto b = GameTestAccess::spawnKind(second);
        require(a == b, "Semente reproduz distribuicao de aliens desde round dois");
        advanced += a == EnemyKind::Advanced ? 1 : 0;
    }
    require(std::abs(static_cast<float>(advanced) / samples - 0.25f) < 0.035f,
            "Distribuicao posterior converge a 75 por cento normal e 25 avancado");
}

void testGravityIsolationRestartAndExpiry() {
    auto config = quietConfig();
    Game game(arena(), config);
    GameTestAccess::peaceful(game);
    const auto origin = game.player().body.position;
    GameTestAccess::field(game, GravityField({32, 32}));
    GameTestAccess::shot(game, Projectile(origin, {1, 0}, 600, 25, 1));
    game.update(dt, {});
    require(game.projectiles().size() == 1 && near(game.projectiles()[0].body.velocity.x, 600) &&
            near(game.projectiles()[0].body.position.x, origin.x + 10),
            "Campo distante nao aplica resistencia global aos projeteis");

    Game completing(arena(), config);
    GameTestAccess::drainSchedule(completing);
    AsteroidConfig shortLife;
    shortLife.lifetime = 0.1f;
    GameTestAccess::asteroid(completing, Asteroid(origin + Vector2D{300, 0}, {1, 0}, shortLife));
    GameTestAccess::field(completing, GravityField({32, 32}));
    completing.update(dt, {});
    require(completing.state() == GameState::RoundCompleted && completing.portal().active &&
            !completing.asteroids().empty() && completing.gravityField(),
            "Asteroide e campo ativos nao impedem abertura do portal");
    for (int i = 0; i < 10; ++i) completing.update(dt, {});
    require(completing.asteroids().empty() && completing.gravityField(), "Asteroide expira sem fragmentar apos round");
    completing.restart();
    Game fresh(arena(), config);
    require(completing.gameplayTime() == 0 && completing.asteroids().empty() && !completing.gravityField(),
            "Reinicio limpa relogio e ambiente");
    for (int i = 0; i < 100; ++i) {
        require(GameTestAccess::rollDrop(completing) == GameTestAccess::rollDrop(fresh), "Reinicio reproduz sequencia aleatoria inicial");
    }

    WaveConfig weak;
    weak.enemy.health = 1;
    weak.enemy.advancedHealthMultiplier = 0.5f;
    require(WaveManager(weak).enemyConfig(EnemyKind::Advanced).health == 1, "Progressao nunca cria avancado com vida zero");
    auto wide = quietConfig();
    wide.waves.enemy.advancedHalfExtent = 40;
    Game narrow(TileMap({".."}), wide);
    narrow.update(dt, {});
    require(narrow.enemies().size() == 1 && narrow.enemies()[0]->kind() == EnemyKind::Normal &&
            narrow.map().isAreaFree(narrow.enemies()[0]->bounds()),
            "Caixa de avancado nao bloqueia nascimento valido de normal no round um");
}

void testEnvironmentSchedulingAndCaps() {
    auto config = quietConfig();
    config.asteroid.spawnInterval = 0.1f;
    config.asteroid.spawnChance = 1;
    config.asteroid.maxActive = 2;
    config.gravity.spawnInterval = 0.1f;
    config.gravity.spawnChance = 1;
    config.gravity.duration = 5;
    Game game(arena(), config);
    GameTestAccess::peaceful(game);
    for (int i = 0; i < 5; ++i) game.update(dt, {});
    require(game.asteroids().empty() && !game.gravityField(), "Ambiente espera intervalo completo");
    for (int i = 0; i < 2; ++i) game.update(dt, {});
    require(!game.asteroids().empty() && game.gravityField(), "Sorteios certos criam ambiente no intervalo");
    const auto center = game.gravityField()->body.position;
    for (int i = 0; i < 60; ++i) {
        game.update(dt, {});
        require(game.asteroids().size() <= 2, "Limite simultaneo de asteroides");
        require(game.gravityField() && game.gravityField()->body.position.equals(center), "Campo ativo nao e substituido por sorteios seguintes");
    }
    require(game.gravityField()->remainingLife() < 4.5f && game.gameplayTime() > 1,
            "Campo envelhece e cronometro avanca apos concluir round");
    Game noEnvironment(arena(), quietConfig());
    GameTestAccess::peaceful(noEnvironment);
    for (int i = 0; i < 3601; ++i) noEnvironment.update(dt, {});
    require(noEnvironment.asteroids().empty() && !noEnvironment.gravityField(), "Probabilidade zero impede spawns apos intervalos padrao");

    auto capped = quietConfig();
    capped.waves.maxActive = 2;
    capped.waves.spawnInterval = 0.01f;
    capped.waves.minSpawnInterval = 0.01f;
    capped.waves.enemy.speed = 1;
    Game aliens(arena(), capped);
    for (int i = 0; i < 120; ++i) aliens.update(dt, {});
    require(aliens.enemies().size() == 2 && aliens.waves().pending() == 3, "Limite ativo mantem restantes pendentes");
    require(std::all_of(aliens.enemies().begin(), aliens.enemies().end(), [](const auto& enemy) {
        return enemy->kind() == EnemyKind::Normal;
    }), "Round um nao gera avancados");
}
}

int main() {
    try {
        testEnginesAndUpgrade();
        testAlienPerception();
        testAsteroidFragmentation();
        testGravity();
        testDropProbabilities();
        testExplosionAndKillAttribution();
        testItemsAndRoundPreservation();
        testEnvironmentSchedulingAndCaps();
        testAlienDistribution();
        testGravityIsolationRestartAndExpiry();
        std::cout << "Motores, IA, upgrades, fragmentacao, gravidade, drops e integracao: OK\n";
    } catch (const std::exception& error) {
        std::cerr << "FALHA: " << error.what() << '\n';
        return 1;
    }
}
