// Adapta o visualizador do professor aos contratos locais de pixels, sprite e passo fixo.
#include "DungeonVisualizer.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <cmath>

namespace {
constexpr float fixedDt = 1.0f / 60.0f;
}

DungeonVisualizer::DungeonVisualizer(Game& game) : m_game(game) {
    const auto center = game.initialCenter();
    m_centerCol = static_cast<int>(center.x / TILE_SIZE);
    m_centerRow = static_cast<int>(center.y / TILE_SIZE);
}

bool DungeonVisualizer::init(unsigned widthPixels, unsigned heightPixels) {
    if (widthPixels == 0 || heightPixels == 0) return false;
    m_window.create(sf::VideoMode(widthPixels, heightPixels),
                    "Space Arena | WASD: mover | Mouse: mirar e atirar | R: reiniciar");
    if (!m_window.isOpen()) return false;
    m_window.setFramerateLimit(60);
    m_window.setKeyRepeatEnabled(false);
    if (!m_font.loadFromFile(SPACE_ARENA_FONT_PATH)) {
        throw std::runtime_error("Fonte do HUD ausente: reconfigure o projeto com CMake");
    }
    m_view = m_window.getDefaultView();
    setInitialCenter(m_centerCol, m_centerRow);
    fitMap();
    return true;
}

void DungeonVisualizer::setInitialCenter(int col, int row) {
    if (!m_game.map().isFree(col, row)) return;
    m_mapFitted = false;
    m_centerCol = col;
    m_centerRow = row;
    const auto center = m_game.map().cellCenter(col, row);
    m_view.setCenter(center.x, center.y);
}

void DungeonVisualizer::setUpdateFunction(UpdateFunction function) {
    m_updateFunction = std::move(function);
}

Vector2D DungeonVisualizer::mouseWorldPosition() const {
    const auto world = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window), m_view);
    return {world.x, world.y};
}

void DungeonVisualizer::registerNPC(NPC* npc) {
    if (npc && std::find(m_npc.begin(), m_npc.end(), npc) == m_npc.end()) {
        m_npc.push_back(npc);
    }
}

void DungeonVisualizer::unregisterNPC(NPC* npc) {
    m_npc.erase(std::remove(m_npc.begin(), m_npc.end(), npc), m_npc.end());
}

void DungeonVisualizer::tryMoveCenter(int deltaCol, int deltaRow) {
    setInitialCenter(m_centerCol + deltaCol, m_centerRow + deltaRow);
}

void DungeonVisualizer::fitMap() {
    const auto size = m_window.getSize();
    if (size.x == 0 || size.y == 0) return;
    updateViewport();
    const float aspect = static_cast<float>(size.x) / (size.y * m_view.getViewport().height);
    const float mapWidth = m_game.map().width() * TILE_SIZE;
    const float mapHeight = m_game.map().height() * TILE_SIZE;
    const float height = std::max(mapHeight, mapWidth / aspect) + TILE_SIZE;
    m_view.setSize(height * aspect, height);
    m_view.setCenter(mapWidth * 0.5f, mapHeight * 0.5f);
    m_mapFitted = true;
}

void DungeonVisualizer::updateViewport() {
    const float height = static_cast<float>(m_window.getSize().y);
    if (height <= 0) return;
    const float top = std::min(84.0f, height * 0.2f);
    const float bottom = std::min(46.0f, height * 0.1f);
    m_view.setViewport({0, top / height, 1, (height - top - bottom) / height});
}

void DungeonVisualizer::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        m_window.close();
    } else if (event.type == sf::Event::Resized && event.size.width && event.size.height) {
        updateViewport();
        if (m_mapFitted) {
            fitMap();
        } else {
            const float aspect = static_cast<float>(event.size.width) /
                                 (event.size.height * m_view.getViewport().height);
            m_view.setSize(m_view.getSize().y * aspect, m_view.getSize().y);
        }
    } else if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
        case sf::Keyboard::Escape: m_window.close(); break;
        case sf::Keyboard::Left: tryMoveCenter(-1, 0); break;
        case sf::Keyboard::Right: tryMoveCenter(1, 0); break;
        case sf::Keyboard::Up: tryMoveCenter(0, -1); break;
        case sf::Keyboard::Down: tryMoveCenter(0, 1); break;
        case sf::Keyboard::F: fitMap(); break;
        case sf::Keyboard::R:
            if (m_game.state() == GameState::GameOver) m_game.restart();
            break;
        case sf::Keyboard::Home: {
            const auto size = m_window.getSize();
            if (size.x && size.y) {
                m_view.setSize(static_cast<float>(size.x), size.y * m_view.getViewport().height);
            }
            const auto center = m_game.initialCenter();
            setInitialCenter(static_cast<int>(center.x / TILE_SIZE),
                             static_cast<int>(center.y / TILE_SIZE));
            break;
        }
        default: break;
        }
    }
}

unsigned DungeonVisualizer::run(unsigned frameLimit) {
    if (!m_window.isOpen()) throw std::logic_error("Inicialize a janela antes de run()");
    sf::Clock clock;
    float accumulator = 0.0f;
    unsigned frames = 0;
    while (m_window.isOpen() && (frameLimit == 0 || frames < frameLimit)) {
        sf::Event event{};
        while (m_window.pollEvent(event)) handleEvent(event);
        if (!m_window.isOpen()) break;

        // Pausas nao causam saltos: limita o tempo acumulado e ignora falta de foco.
        const float elapsed = std::min(clock.restart().asSeconds(), 0.25f);
        if (m_window.hasFocus()) {
            accumulator += elapsed;
            while (accumulator >= fixedDt) {
                m_game.update(fixedDt, readInput());
                if (m_updateFunction) m_updateFunction(fixedDt);
                accumulator -= fixedDt;
            }
        } else {
            accumulator = 0.0f;
        }
        render();
        ++frames;
    }
    m_window.close(); // Retorno normal permite destruir todos os recursos por RAII.
    return frames;
}

GameInput DungeonVisualizer::readInput() const {
    GameInput input;
    input.movement.x = static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::D)) -
                       static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::A));
    input.movement.y = static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::S)) -
                       static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::W));
    input.aimTarget = mouseWorldPosition();
    input.fire = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    return input;
}

void DungeonVisualizer::render() {
    m_window.clear(sf::Color(10, 14, 24));
    m_window.setView(m_view);
    sf::RectangleShape tile({TILE_SIZE, TILE_SIZE});
    const auto& map = m_game.map();
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            tile.setPosition(col * TILE_SIZE, row * TILE_SIZE);
            tile.setFillColor(map.isWall(col, row) ? sf::Color(85, 108, 140)
                                                  : sf::Color(23, 35, 51));
            m_window.draw(tile);
        }
    }
    for (NPC* npc : m_npc) {
        if (!npc->active || !npc->sprite.getTexture()) continue;
        // body.position ja esta em pixels e indica o centro; nao multiplicar por TILE_SIZE.
        npc->sprite.setPosition(npc->body.position.x, npc->body.position.y);
        m_window.draw(npc->sprite);
    }

    if (m_game.portal().active) {
        sf::CircleShape ring(24, 48);
        ring.setOrigin(24, 24);
        const auto position = m_game.portal().body.position;
        ring.setPosition(position.x, position.y);
        ring.setFillColor(sf::Color(110, 60, 220, 80));
        ring.setOutlineColor(sf::Color(185, 130, 255));
        ring.setOutlineThickness(4);
        m_window.draw(ring);
    }

    if (m_game.gravityField()) {
        const auto& field = *m_game.gravityField();
        sf::CircleShape reach(field.radius(), 96);
        reach.setOrigin(field.radius(), field.radius());
        reach.setPosition(field.body.position.x, field.body.position.y);
        reach.setFillColor(sf::Color(55, 105, 230, 25));
        reach.setOutlineColor(sf::Color(110, 165, 255, 180));
        reach.setOutlineThickness(2);
        m_window.draw(reach);
        sf::CircleShape core(10, 24);
        core.setOrigin(10, 10);
        core.setPosition(field.body.position.x, field.body.position.y);
        core.setFillColor(sf::Color(160, 200, 255));
        m_window.draw(core);
    }
    for (const auto& rock : m_game.asteroids()) {
        sf::CircleShape shape(rock.radius(), rock.depth() == 0 ? 7 : 5);
        shape.setOrigin(rock.radius(), rock.radius());
        shape.setPosition(rock.body.position.x, rock.body.position.y);
        shape.setFillColor(rock.collisionEnabled() ? sf::Color(125, 115, 105) : sf::Color(185, 175, 155));
        shape.setOutlineColor(sf::Color(205, 190, 165));
        shape.setOutlineThickness(2);
        m_window.draw(shape);
    }
    for (const auto& item : m_game.pickups()) {
        const bool engine = item.kind() == PickupKind::Engine;
        const float radius = item.config().halfExtent;
        sf::CircleShape shape(radius, engine ? 4 : 3);
        shape.setOrigin(radius, radius);
        shape.setPosition(item.body.position.x, item.body.position.y);
        shape.setFillColor(engine ? sf::Color(70, 235, 150) : sf::Color(255, 205, 65));
        m_window.draw(shape);
        sf::Text label(engine ? "M" : "+", m_font, 14);
        const auto bounds = label.getLocalBounds();
        label.setOrigin(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
        label.setPosition(item.body.position.x, item.body.position.y);
        label.setFillColor(sf::Color(15, 25, 35));
        m_window.draw(label);
    }
    for (const auto& enemy : m_game.enemies()) {
        const bool advanced = enemy->kind() == EnemyKind::Advanced;
        const float radius = enemy->collision.halfExtents.x;
        sf::CircleShape shape(radius, advanced ? 8 : 6);
        shape.setOrigin(radius, radius);
        shape.setPosition(enemy->body.position.x, enemy->body.position.y);
        shape.setFillColor(advanced ? sf::Color(220, 105, 245) : sf::Color(245, 85, 95));
        shape.setOutlineColor(enemy->state() == EnemyState::Chasing ? sf::Color::White : sf::Color(130, 50, 55));
        shape.setOutlineThickness(1.5f);
        m_window.draw(shape);
        if (advanced) {
            sf::RectangleShape antenna({3, 17});
            antenna.setOrigin(1.5f, 17);
            antenna.setPosition(enemy->body.position.x, enemy->body.position.y - radius + 4);
            antenna.setFillColor(sf::Color(245, 215, 255));
            m_window.draw(antenna);
            sf::CircleShape tip(4);
            tip.setOrigin(4, 4);
            tip.setPosition(enemy->body.position.x, enemy->body.position.y - radius - 13);
            tip.setFillColor(sf::Color::Yellow);
            m_window.draw(tip);
        }
    }
    for (const auto& event : m_game.explosions()) {
        sf::CircleShape blast(event.radius, 64);
        blast.setOrigin(event.radius, event.radius);
        blast.setPosition(event.position.x, event.position.y);
        const auto opacity = static_cast<sf::Uint8>(120 * std::clamp(event.remainingLife / m_game.config().explosionLifetime, 0.0f, 1.0f));
        blast.setFillColor(sf::Color(255, 160, 55, opacity));
        blast.setOutlineColor(sf::Color(255, 220, 130, opacity));
        blast.setOutlineThickness(3);
        m_window.draw(blast);
    }

    sf::CircleShape projectile(4.0f);
    projectile.setOrigin(4, 4);
    projectile.setFillColor(sf::Color(255, 225, 100));
    for (const auto& shot : m_game.projectiles()) {
        projectile.setPosition(shot.body.position.x, shot.body.position.y);
        m_window.draw(projectile);
    }

    sf::CircleShape ship(20.0f, 3);

    const auto& position = m_game.player().body.position;

    ship.setOrigin(20,20);
    ship.setPosition(position.x, position.y);
    ship.setRotation(m_game.player().angleDegrees() + 90.0f);
    ship.setFillColor(m_game.player().active ?
        (m_game.player().invulnerable() ? sf::Color::White : sf::Color::Cyan) : sf::Color(90, 100, 110));

    m_window.draw(ship);
    const float rotation = m_game.player().angleDegrees() * 3.14159265358979323846f / 180.0f;
    const Vector2D forward{std::cos(rotation), std::sin(rotation)};
    const Vector2D side{-forward.y, forward.x};
    for (int i = 0; i < m_game.player().engines(); ++i) {
        const auto enginePosition = position - forward * 12.0f + side * ((i - 1.5f) * 5.0f);
        sf::CircleShape engine(2);
        engine.setOrigin(2, 2);
        engine.setPosition(enginePosition.x, enginePosition.y);
        engine.setFillColor(sf::Color(255, 165, 50));
        m_window.draw(engine);
    }
    renderHUD();
    m_window.display();
}

void DungeonVisualizer::renderHUD() {
    const auto size = m_window.getSize();
    m_window.setView(sf::View(sf::FloatRect(0, 0, static_cast<float>(size.x), static_cast<float>(size.y))));
    sf::RectangleShape panel({static_cast<float>(size.x), 78});
    panel.setFillColor(sf::Color(8, 12, 22, 235));
    m_window.draw(panel);
    sf::Text text;
    text.setFont(m_font);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color(225, 237, 250));
    text.setPosition(18, 10);
    text.setString("MOTORES " + std::to_string(m_game.player().engines()) + "/4   TIRO " +
        std::to_string(m_game.player().weapon().level()) + "/3   ROUND " +
        std::to_string(m_game.waves().round()) + "     INIMIGOS " + std::to_string(m_game.enemiesRemaining()));
    m_window.draw(text);
    sf::RectangleShape healthBack({180, 7});
    healthBack.setPosition(18, 42);
    healthBack.setFillColor(sf::Color(70, 40, 55));
    m_window.draw(healthBack);
    healthBack.setSize({180.0f * m_game.player().health() / m_game.player().maxHealth(), 7});
    healthBack.setFillColor(sf::Color(70, 225, 190));
    m_window.draw(healthBack);
    text.setCharacterSize(13);
    text.setPosition(18, 56);
    text.setString("WASD: mover   Mouse: mirar   Botao esquerdo: disparar   Setas/F/Home: camera   Esc: sair");
    m_window.draw(text);

    if (m_game.state() == GameState::RoundCompleted) {
        panel.setSize({static_cast<float>(size.x), 42});
        panel.setPosition(0, static_cast<float>(size.y) - 42);
        m_window.draw(panel);
        text.setCharacterSize(18);
        text.setPosition(18, static_cast<float>(size.y) - 34);
        text.setString(m_game.portal().waitingForExit()
            ? "ROUND CONCLUIDO! Saia do portal e entre novamente para continuar."
            : "ROUND CONCLUIDO! Entre no portal violeta para o proximo round.");
        m_window.draw(text);
    } else if (m_game.state() == GameState::GameOver) {
        panel.setSize({static_cast<float>(size.x), 130});
        panel.setPosition(0, size.y * 0.5f - 65);
        m_window.draw(panel);
        text.setCharacterSize(30);
        text.setString("FIM DE JOGO");
        auto bounds = text.getLocalBounds();
        text.setPosition((size.x - bounds.width) * 0.5f, size.y * 0.5f - 48);
        m_window.draw(text);
        text.setCharacterSize(18);
        text.setString("Pressione R para reiniciar a partida");
        bounds = text.getLocalBounds();
        text.setPosition((size.x - bounds.width) * 0.5f, size.y * 0.5f + 10);
        m_window.draw(text);
    }
}
