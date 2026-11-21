#include "Player.hpp"
#include "Game.hpp"
#include "Dungeon2D.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <limits>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::abs(a - b) < 0.001f; }
void testPlayer() {
    Player player;
    player.body.position = {100, 100};
    player.aimAt({200, 100});
    require(near(player.angleDegrees(), 0), "Mira direita");
    player.aimAt({100, 200});
    require(near(player.angleDegrees(), 90), "Mira baixo com Y SFML");
    player.aimAt({0, 100});
    require(near(player.angleDegrees(), 180), "Mira esquerda");
    player.aimAt({100, 0});
    require(near(player.angleDegrees(), -90), "Mira cima");
    player.aimAt(player.body.position);
    require(near(player.angleDegrees(), -90), "Mira nula deve preservar angulo");
    require(player.body.position.equals({100, 100}), "Mira alterou posicao fisica");
    require(player.aimDirection().equals({0, -1}), "Direcao da mira");
    player.setMovementDirection({1, 1});
    require(near(player.body.velocity.length(), 250), "Velocidade diagonal");
    const auto before = player.body.position;
    player.update(1.0f / 60.0f);
    require(near((player.body.position - before).length(), 250.0f / 60), "Integracao fixa");
    player.setMovementDirection({});
    require(player.body.velocity.equals({}), "Parada");
}
void testWeapons() {
    Weapon weapon({0.2f, 600, 30, 1});
    auto shot = weapon.tryFire({100, 100}, {2, 0});
    require(shot && shot->damage() == 30, "Disparo configuravel");
    require(!weapon.tryFire({}, {1, 0}), "Cadencia imediata");
    weapon.update(0.1f);
    require(!weapon.tryFire({}, {1, 0}), "Cadencia antes do prazo");
    weapon.update(0.1f);
    require(weapon.tryFire({}, {1, 0}).has_value(), "Disparo apos cooldown");
    shot->update(0.25f);
    require(shot->body.position.equals({250, 100}), "Velocidade do projetil");
    shot->update(0.75f);
    require(!shot->active, "Expiracao do projetil");
    Game game(TileMap(TileMap::Matrix(5, std::string(5, '.'))));
    GameInput input{{}, {10000, game.player().body.position.y}, true};
    for (int i = 0; i < 120; ++i) game.update(1.0f / 60, input);
    require(game.projectiles().size() < 5, "Quantidade limitada de projeteis");
    for (const auto& projectile : game.projectiles()) {
        require(game.map().isAreaFree(projectile.bounds()), "Projetil fora do mapa permaneceu ativo");
    }
    for (int i = 0; i < 180; ++i) game.update(1.0f / 60, {});
    require(game.projectiles().empty(), "Limpeza apos soltar gatilho");
}

void testEnemiesAndBounds() {
    Enemy enemy({100, 100});
    enemy.update(1.0f / 60, {200, 100});
    require(near(enemy.body.position.x, 101.5f), "Perseguicao em pixels/s");
    enemy.body.position = {100, 100};
    Player player;
    player.body.position = {100, 100};
    require(enemy.tryAttack(player) == 1, "Dano por contato");
    require(enemy.tryAttack(player) == 0, "Intervalo de ataque");
    enemy.update(0.8f, enemy.body.position);
    require(enemy.tryAttack(player) == 1, "Recarga do contato");
    enemy.takeDamage(25);
    require(enemy.active && enemy.health() == 25, "Dano parcial");
    enemy.takeDamage(25);
    require(!enemy.active && enemy.tryAttack(player) == 0, "Inimigo derrotado");
    require(player.takeDamage(1) && !player.takeDamage(1), "Invulnerabilidade do jogador");
    player.update(player.config().damageInvulnerability);
    require(player.takeDamage(1) && player.engines() == 2, "Fim da invulnerabilidade");
    Game game(TileMap({"...", "...", "..."}));
    for (int i = 0; i < 300; ++i) game.update(1.0f / 60, {{1, 1}, {}, false});
    require(game.map().isAreaFree(game.player().bounds()), "Limites do mapa");
    require(near(game.player().body.position.x, 176), "Limite considera tamanho da nave");
    TileMap wall({".#.", "...", "..."});
    require(!wall.isAreaFree({{60, 20}, {70, 40}}), "Area cruzando parede");
    require(wall.isAreaFree({{32, 20}, {64, 40}}), "Contato sem penetracao");
}

void testSweptCollision() {
    Collision2D shape;
    shape.halfExtents = {4, 4};
    const AABB target{{90, 90}, {110, 110}};
    auto hit = shape.firstIntersection({0, 100}, {200, 100}, target);
    require(hit && near(*hit, 0.43f), "Colisao continua evita atravessamento");
    require(!shape.firstIntersection({0, 50}, {200, 50}, target), "Segmento sem contato");
    require(shape.firstIntersection({100, 100}, {100, 100}, target).has_value(), "Contato inicial");
    require(!shape.firstIntersection({0, 0}, {0, 0}, target), "Segmento nulo separado");
    auto reverse = shape.firstIntersection({200, 100}, {0, 100}, target);
    require(reverse && near(*reverse, 0.43f), "Varredura reversa");
    Enemy nearEnemy({100, 100}), farEnemy({150, 100});
    Projectile shot({0, 100}, {1, 0}, 12000, 50, 1);
    shot.update(1.0f / 60);
    const auto first = shot.collision.firstIntersection(shot.previousPosition(), shot.body.position, nearEnemy.bounds());
    const auto second = shot.collision.firstIntersection(shot.previousPosition(), shot.body.position, farEnemy.bounds());
    require(first && second && *first < *second, "Ordem dos impactos");
}

void testWavesAndSpawns() {
    WaveManager waves;
    require(waves.round() == 1 && waves.pending() == 5 && waves.readyToSpawn(), "Round inicial");
    require(!waves.completed(0), "Pendentes impedem fim prematuro");
    for (int i = 0; i < 5; ++i) {
        require(waves.readyToSpawn(), "Spawn agendado");
        waves.confirmSpawn();
        require(!waves.readyToSpawn(), "Spawn gradual");
        waves.update(2.0f);
    }
    require(!waves.completed(1) && waves.completed(0), "Fim requer vivos e pendentes zerados");
    const auto first = waves.enemyConfig();
    waves.startNextRound();
    require(waves.round() == 2 && waves.pending() == 7 && waves.enemyConfig().health > first.health &&
            waves.enemyConfig().speed > first.speed, "Progressao");
    waves.reset();
    require(waves.round() == 1 && waves.pending() == 5, "Reset de rounds");
    Game game(TileMap(TileMap::Matrix(23, std::string(31, '.'))));
    game.update(1.0f / 60, {});
    require(game.enemies().size() == 1 && game.waves().pending() == 4, "Spawn integrado");
    const auto position = game.enemies().front()->body.position;
    require(position.x < 2 * TILE_SIZE || position.y < 2 * TILE_SIZE ||
            position.x > 29 * TILE_SIZE || position.y > 21 * TILE_SIZE, "Spawn proximo da borda");
    require(game.map().isAreaFree(game.enemies().front()->bounds()), "Spawn com AABB livre");
}

void testPortalAndCombatCycle() {
    Portal portal;
    Player player;
    player.body.position = {100, 100};
    portal.body.position = player.body.position;
    require(!portal.tryEnter(player), "Portal inativo");
    portal.activate(player);
    require(!portal.tryEnter(player), "Ativacao sob nave exige nova entrada");
    player.body.position = {200, 100};
    require(!portal.tryEnter(player), "Sair do portal");
    player.body.position = {100, 100};
    require(portal.tryEnter(player), "Reentrada no portal");
    portal.deactivate();
    require(!portal.tryEnter(player), "Desativacao do portal");

    WaveConfig config;
    config.initialEnemies = 2;
    config.enemy.health = 25;
    config.enemy.speed = 30;
    Game game(TileMap(TileMap::Matrix(11, std::string(15, '.'))), config);
    const auto originalMap = game.map().rows();
    // Controle publico real: mira no alvo e mantem o gatilho, sem alterar vidas.
    for (int i = 0; i < 1800 && game.state() == GameState::Playing; ++i) {
        Vector2D aim = game.player().body.position + Vector2D{1, 0};
        if (!game.enemies().empty()) aim = game.enemies().front()->body.position;
        game.update(1.0f / 60, {{}, aim, true});
    }
    require(game.state() == GameState::RoundCompleted && game.enemiesRemaining() == 0,
            "Ciclo integrado: tiros eliminam todos os inimigos");
    require(game.portal().active && game.projectiles().empty(), "Portal ativa apos combate");
    require(game.portal().body.position.equals({480, 352}), "Portal central em pixels");
    game.player().takeDamage(1);
    game.player().weapon().upgrade();
    game.player().weapon().upgrade();
    // Sai primeiro caso ja esteja no portal e volta usando movimento real.
    for (int i = 0; i < 30; ++i) game.update(1.0f / 60, {{1, 0}, {}, false});
    for (int i = 0; i < 120 && game.state() == GameState::RoundCompleted; ++i) {
        const auto direction = game.portal().body.position - game.player().body.position;
        game.update(1.0f / 60, {direction, {}, false});
    }
    require(game.waves().round() == 2 && game.state() == GameState::Playing && !game.portal().active,
            "Entrada inicia proximo round uma vez");
    require(game.player().engines() == 3 && game.player().weapon().level() == 3,
            "Transicao preserva motores restantes e upgrades");
    require(game.map().rows() == originalMap && game.enemiesRemaining() == 4,
            "Transicao preserva mapa e aumenta quantidade");
    game.update(1.0f / 60, {});
    require(game.waves().round() == 2, "Sem transicoes repetidas");
}

void testSingleImpactIntegration() {
    WaveConfig config;
    config.initialEnemies = 2;
    config.spawnInterval = 0.01f;
    Game game(TileMap(TileMap::Matrix(11, std::string(15, '.'))), config);
    game.update(1.0f / 60, {});
    game.update(1.0f / 60, {});
    require(game.enemies().size() == 2, "Fixture com dois inimigos");
    const auto playerPosition = game.player().body.position;
    game.enemies()[0]->body.position = playerPosition + Vector2D{120, 0};
    game.enemies()[1]->body.position = playerPosition + Vector2D{170, 0};
    game.player().weapon() = Weapon({0.2f, 12000, 50, 1});
    game.update(1.0f / 60, {{}, playerPosition + Vector2D{500, 0}, true});
    require(game.enemies().size() == 1 && game.enemies()[0]->health() == 50 && game.projectiles().empty(),
            "Um projetil rapido elimina apenas o primeiro alvo");
}

void testGameOverAndRestart() {
    WaveConfig configuration;
    configuration.enemy.detectionRadius = 2000;
    configuration.enemy.lossRadius = 2400;
    Game game(TileMap(TileMap::Matrix(11, std::string(15, '.'))), configuration);
    const auto originalMap = game.map().rows();
    for (int i = 0; i < 3600 && game.state() != GameState::GameOver; ++i) game.update(1.0f / 60, {});
    require(game.state() == GameState::GameOver && game.player().health() == 0 && !game.player().active,
            "Contato causa derrota");
    const auto position = game.player().body.position;
    const auto remaining = game.enemiesRemaining();
    game.update(1.0f / 60, {{1, 1}, {}, true});
    require(game.player().body.position.equals(position) && game.enemiesRemaining() == remaining &&
            game.projectiles().empty() && !game.portal().active, "Derrota congela simulacao");
    game.restart();
    require(game.state() == GameState::Playing && game.player().engines() == 4 && game.player().active &&
            game.waves().round() == 1 && game.waves().pending() == 5 && game.enemies().empty() &&
            game.projectiles().empty() && !game.portal().active, "Reinicio completo");
    require(game.player().body.position.equals(game.initialCenter()) && game.map().rows() == originalMap,
            "Reinicio preserva arena e restaura posicao");
    require(game.player().weapon().tryFire(position, {1, 0}).has_value(), "Reinicio limpa cooldown");
}

void testOptionalMaps() {
    TileMap corridor({".....", ".#...", ".#...", ".#...", "....."});
    Enemy enemy(corridor.cellCenter(0, 2), {}, EnemyKind::Advanced);
    const auto target = corridor.cellCenter(2, 2);
    for (int i = 0; i < 900; ++i) {
        enemy.update(1.0f / 60, target, &corridor);
        require(corridor.isAreaFree(enemy.bounds()), "A* do inimigo cruzou parede");
    }
    require((enemy.body.position - target).length() < 2, "Perseguicao contorna parede opcional");
    for (unsigned seed = 0; seed < 10; ++seed) {
        Dungeon2D dungeon(31, 23);
        dungeon.generate(seed);
        TileMap map;
        map.loadFromDungeon(dungeon);
        Game game(map);
        for (int i = 0; i < 660; ++i) {
            game.update(1.0f / 60, {});
            for (const auto& spawned : game.enemies()) {
                require(game.map().isAreaFree(spawned->bounds()), "Spawn/perseguicao procedural bloqueado");
            }
        }
        require(game.waves().pending() == 0, "Round procedural gerou todos os pendentes");
        require(map.isAreaFree(game.portal().bounds()), "Portal procedural livre");
    }
    Game fallback(TileMap({".....", ".....", "..#..", ".....", "....."}));
    require(fallback.map().isAreaFree(fallback.portal().bounds()), "Portal CSV com centro bloqueado");
    Game tiny(TileMap({"."}));
    tiny.update(1.0f / 60, {});
    require(tiny.enemies().empty() && tiny.waves().pending() == 5 && tiny.state() == GameState::Playing,
            "Spawn sem espaco preserva pendentes");
}

void testWallImpactIntegration() {
    auto rows = TileMap::Matrix(11, std::string(15, '.'));
    rows[5][9] = '#';
    WaveConfig config;
    config.initialEnemies = 1;
    Game game(TileMap(rows), config);
    game.update(1.0f / 60, {});
    require(game.enemies().size() == 1, "Fixture com inimigo atras da parede");
    game.enemies()[0]->body.position = game.map().cellCenter(10, 5);
    game.player().weapon() = Weapon({0.2f, 12000, 50, 1});
    game.update(1.0f / 60, {{}, game.map().cellCenter(14, 5), true});
    require(game.enemies().size() == 1 && game.enemies()[0]->health() == 50 && game.projectiles().empty(),
            "Parede consome tiro antes do inimigo");
}

void testInvalidConfiguration() {
    const auto rejects = [](auto make) {
        try { make(); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    require(rejects([] { Weapon weapon({0, 700, 25, 1}); }), "Cooldown zero rejeitado");
    require(rejects([] { Projectile shot({}, {}, 700, 25, 1); }), "Direcao nula rejeitada");
    require(rejects([] { Enemy enemy({}, {0, 90, 10, 0.8f}); }), "Vida nula rejeitada");
    require(rejects([] { WaveConfig config; config.initialEnemies = 0; WaveManager waves(config); }),
            "Onda vazia rejeitada");
    require(rejects([] { Weapon weapon({std::numeric_limits<float>::quiet_NaN(), 700, 25, 1}); }),
            "Configuracao nao finita rejeitada");
    Game game(TileMap({"...", "...", "..."}));
    game.update(std::numeric_limits<float>::quiet_NaN(), {{1, 1}, {}, true});
    require(game.enemies().empty() && game.player().body.position.equals(game.initialCenter()),
            "Passo nao finito ignorado");
    Projectile shot({}, {1, 0}, 100, 25, 0.1f);
    shot.update(0.2f);
    require(!shot.active && near(shot.body.position.x, 10), "Movimento limitado a vida restante");
}

}
int main() {
    try {
        testPlayer();
        testWeapons();
        testEnemiesAndBounds();
        testSweptCollision();
        testWavesAndSpawns();
        testPortalAndCombatCycle();
        testSingleImpactIntegration();
        testGameOverAndRestart();
        testOptionalMaps();
        testWallImpactIntegration();
        testInvalidConfiguration();
        std::cout << "Regras de jogo: OK\n";
    } catch (const std::exception& error) {
        std::cerr << "FALHA: " << error.what() << '\n';
        return 1;
    }
}
