// Adapta o visualizador do professor aos contratos locais de pixels, sprite e passo fixo.
#include "DungeonVisualizer.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace {
constexpr float fixedDt = 1.0f / 60.0f;
}

DungeonVisualizer::DungeonVisualizer(Game& game, std::filesystem::path dataDirectory) : m_game(game), m_menu(game, std::move(dataDirectory)) {
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
    updateUIViewport();
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
    const float top = std::min(106.0f, height * 0.25f);
    const float bottom = std::min(46.0f, height * 0.1f);
    m_view.setViewport({0, top / height, 1, (height - top - bottom) / height});
}

void DungeonVisualizer::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        m_window.close();
    } else if (event.type == sf::Event::Resized && event.size.width && event.size.height) {
        if(event.size.width<640 || event.size.height<600) { m_window.setSize({std::max(640u,event.size.width),std::max(600u,event.size.height)}); return; }
        updateUIViewport();
        updateViewport();
        if (m_mapFitted) {
            fitMap();
        } else {
            const float aspect = static_cast<float>(event.size.width) /
                                 (event.size.height * m_view.getViewport().height);
            m_view.setSize(m_view.getSize().y * aspect, m_view.getSize().y);
        }
    } else if (event.type == sf::Event::LostFocus) {
        if(m_menu.screen()==Screen::Playing) m_menu.escape();
        m_inputBlocked=true;
    } else if(event.type==sf::Event::TextEntered && m_menu.screen()==Screen::CustomScenario && m_pathEditing){
        auto& path=m_menu.csvPath();
        if(event.text.unicode==8 && !path.empty()){
            do {const auto c=static_cast<unsigned char>(path.back());path.pop_back();if((c&0xc0)!=0x80)break;}while(!path.empty());
        } else if(event.text.unicode>=32 && event.text.unicode!=127 && event.text.unicode<=0x10ffff && path.size()<4090){
            auto utf8=sf::String(event.text.unicode).toUtf8();path.append(utf8.begin(),utf8.end());
        }
    } else if (event.type == sf::Event::TextEntered && m_menu.screen()==Screen::GameOver && !m_menu.scoreRecorded() && m_nameEditing) {
        auto& name=m_menu.playerName();
        if(event.text.unicode==8 && !name.empty())name.pop_back();
        else if(event.text.unicode>=32 && event.text.unicode<127 && name.size()<32)name+=static_cast<char>(event.text.unicode);
    } else if (m_menu.screen()!=Screen::Playing && event.type==sf::Event::MouseMoved) {
        const auto point=m_window.mapPixelToCoords({event.mouseMove.x,event.mouseMove.y},m_uiView);
        const auto buttons=menuButtons();for(std::size_t i=0;i<buttons.size();++i)if(buttons[i].bounds.contains(point))m_menu.select(static_cast<int>(i));
    } else if (m_menu.screen()!=Screen::Playing && event.type==sf::Event::MouseButtonPressed && event.mouseButton.button==sf::Mouse::Left) {
        const auto point=m_window.mapPixelToCoords({event.mouseButton.x,event.mouseButton.y},m_uiView);
        if(m_menu.screen()==Screen::CustomScenario && sf::FloatRect(70,230,680,70).contains(point)){m_pathEditing=true;return;}
        m_pathEditing=false;
        if(m_menu.screen()==Screen::GameOver && !m_menu.scoreRecorded() && sf::FloatRect(148,445,524,60).contains(point)){
            m_nameEditing=true;m_menu.playerName().clear();return;
        }
        m_nameEditing=false;
        const auto buttons=menuButtons();for(std::size_t i=0;i<buttons.size();++i)if(buttons[i].bounds.contains(point) && buttons[i].enabled){m_menu.select(static_cast<int>(i));m_menu.activate();m_inputBlocked=true;break;}
    } else if (event.type == sf::Event::KeyPressed) {
        if(m_menu.screen()==Screen::CustomScenario && m_pathEditing){
            if(event.key.code==sf::Keyboard::Enter){m_menu.select(1);m_menu.activate();m_pathEditing=false;}
            else if(event.key.code==sf::Keyboard::Escape || event.key.code==sf::Keyboard::Tab)m_pathEditing=false;
            return;
        }
        if(m_menu.screen()==Screen::CustomScenario && event.key.code==sf::Keyboard::Tab){m_pathEditing=true;return;}
        if(m_menu.screen()==Screen::GameOver && m_nameEditing){
            if(event.key.code==sf::Keyboard::Enter){m_menu.select(0);m_menu.activate();m_nameEditing=false;}
            else if(event.key.code==sf::Keyboard::Escape || event.key.code==sf::Keyboard::Tab)m_nameEditing=false;
            return;
        }
        if(m_menu.screen()==Screen::GameOver && !m_menu.scoreRecorded() && event.key.code==sf::Keyboard::Tab){m_nameEditing=true;m_menu.playerName().clear();return;}
        if(event.key.code==sf::Keyboard::Escape){m_menu.escape();m_inputBlocked=true;return;}
        if(m_menu.screen()!=Screen::Playing){
            if(event.key.code==sf::Keyboard::Up || event.key.code==sf::Keyboard::W)m_menu.navigate(-1);
            if(event.key.code==sf::Keyboard::Down || event.key.code==sf::Keyboard::S)m_menu.navigate(1);
            if(event.key.code==sf::Keyboard::Enter || event.key.code==sf::Keyboard::Space){m_menu.activate();m_inputBlocked=true;}
            if(event.key.code==sf::Keyboard::R)m_menu.restartAfterDefeat();
            if(m_menu.screen()==Screen::Mechanics){if(event.key.code==sf::Keyboard::Right)m_menu.turnPage(1);if(event.key.code==sf::Keyboard::Left)m_menu.turnPage(-1);}
            return;
        }

        switch (event.key.code) {

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
        bool screenChanged=false;
        while (m_window.pollEvent(event)) {
            const auto before=m_menu.screen();
            handleEvent(event);
            screenChanged=screenChanged || before!=m_menu.screen();
            if(before!=m_menu.screen() && m_menu.screen()==Screen::Playing && before!=Screen::Paused){
                const auto center=m_game.initialCenter();
                setInitialCenter(static_cast<int>(center.x/TILE_SIZE),static_cast<int>(center.y/TILE_SIZE));
                fitMap();
            }
        }
        if (m_menu.quitting()) m_window.close();
        if (!m_window.isOpen()) break;

        // Pausas nao causam saltos: limita o tempo acumulado e ignora falta de foco.
        float elapsed = std::min(clock.restart().asSeconds(), 0.25f);
        if(screenChanged){elapsed=0;accumulator=0;}
        if (m_window.hasFocus() && m_menu.screen()==Screen::Playing) {
            accumulator += elapsed;
            while (accumulator >= fixedDt) {
                m_menu.update(fixedDt, readInput());
                if (m_updateFunction && m_menu.screen()==Screen::Playing) m_updateFunction(fixedDt);
                if(m_menu.screen()!=Screen::Playing){accumulator=0;break;}
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
    if(m_inputBlocked && !sf::Mouse::isButtonPressed(sf::Mouse::Left)) m_inputBlocked=false;
    input.fire = !m_inputBlocked && sf::Mouse::isButtonPressed(sf::Mouse::Left);
    return input;
}

void DungeonVisualizer::render() {
    if(m_menu.screen()!=Screen::Playing){renderMenu();m_window.display();return;}
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
    sf::RectangleShape panel({static_cast<float>(size.x), 100});
    panel.setFillColor(sf::Color(8, 12, 22, 235));
    m_window.draw(panel);
    sf::Text text;
    text.setFont(m_font);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color(225, 237, 250));
    text.setPosition(18, 10);
    text.setString("MOTORES " + std::to_string(m_game.player().engines()) + "/4   ROUND " +
        std::to_string(m_game.waves().round()) + "  SCORE " + std::to_string(m_game.score()) + "  INIMIGOS " + std::to_string(m_game.enemiesRemaining()));
    m_window.draw(text);
    sf::RectangleShape healthBack({180, 7});
    healthBack.setPosition(18, 42);
    healthBack.setFillColor(sf::Color(70, 40, 55));
    m_window.draw(healthBack);
    healthBack.setSize({180.0f * m_game.player().health() / m_game.player().maxHealth(), 7});
    healthBack.setFillColor(sf::Color(70, 225, 190));
    m_window.draw(healthBack);
    text.setCharacterSize(13);
    text.setPosition(210, 38);
    const auto& weapon = m_game.player().weapon();
    text.setString("Mods: D" + std::to_string(weapon.acquiredTier(UpgradeKind::Damage)) +
        " Cad" + std::to_string(weapon.acquiredTier(UpgradeKind::FireRate)) +
        " Vel" + std::to_string(weapon.acquiredTier(UpgradeKind::ProjectileSpeed)) +
        " Vida" + std::to_string(weapon.acquiredTier(UpgradeKind::Lifetime)));
    m_window.draw(text);
    text.setPosition(18, 54);
    text.setString("ARMA: " + m_game.player().weapon().summary());
    m_window.draw(text);
    text.setPosition(18, 78);
    text.setString("WASD: mover   Mouse: mirar   Botao esquerdo: disparar   Setas/F/Home: camera   Esc: pausa");
    m_window.draw(text);
    text.setCharacterSize(12);
    text.setString(std::string(m_game.scenario().type==ScenarioType::Classic?"Arena":m_game.scenario().type==ScenarioType::Procedural?"Procedural":"CSV") + " | Seed " + std::to_string(m_game.config().seed) + " | ofertas " +
                   std::to_string(m_game.upgrades().unlocked().size()));
    const auto seedBounds = text.getLocalBounds();
    text.setPosition(std::max(18.0f, size.x - seedBounds.width - 18), 38);
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

void DungeonVisualizer::updateUIViewport() {
    const auto size=m_window.getSize();if(!size.x || !size.y)return;
    m_uiView=sf::View(sf::FloatRect(0,0,820,900));
    const float ratio=static_cast<float>(size.x)/size.y, target=820.0f/900.0f;
    if(ratio>target)m_uiView.setViewport({(1-target/ratio)/2,0,target/ratio,1});
    else m_uiView.setViewport({0,(1-ratio/target)/2,1,ratio/target});
}
std::vector<Button> DungeonVisualizer::menuButtons() const {
    std::vector<Button> result;const auto choices=m_menu.choices();
    float top=320,height=64,gap=18;
    if(m_menu.screen()==Screen::Controls || m_menu.screen()==Screen::Mechanics || m_menu.screen()==Screen::Scores)top=800;
    if(m_menu.screen()==Screen::ConfirmNewGame)top=480;
    if(m_menu.screen()==Screen::CustomScenario)top=420;
    if(m_menu.screen()==Screen::GameOver){top=590;height=56;}
    if(m_menu.screen()==Screen::Mechanics){for(std::size_t i=0;i<choices.size();++i)result.push_back({{70+static_cast<float>(i)*245,800,190,64},choices[i],true});return result;}
    for(std::size_t i=0;i<choices.size();++i)result.push_back({{178,top+static_cast<float>(i)*(height+gap),464,height},choices[i],m_menu.enabled(i)});
    return result;
}
void DungeonVisualizer::renderMenu() {
    m_window.clear(sf::Color(6,12,27));m_window.setView(m_uiView);
    auto text=[&](const std::string& value,float x,float y,unsigned size,sf::Color color=sf::Color(213,235,252),bool center=false){
        sf::Text t(sf::String::fromUtf8(value.begin(),value.end()),m_font,size);t.setFillColor(color);
        if(center){auto b=t.getLocalBounds();t.setOrigin(b.left+b.width/2,0);}t.setPosition(x,y);m_window.draw(t);
    };
    // Decoracao deterministica: nao consome o RNG da partida nem avanca simulacao.
    sf::CircleShape planet(196,96);planet.setOrigin(196,196);planet.setPosition(750,200);planet.setFillColor(sf::Color(16,34,62));m_window.draw(planet);
    sf::CircleShape orbit(255,96);orbit.setOrigin(255,255);orbit.setScale(1.4f,.35f);orbit.setRotation(-28);orbit.setPosition(750,220);orbit.setFillColor(sf::Color::Transparent);orbit.setOutlineThickness(3);orbit.setOutlineColor(sf::Color(48,75,113));m_window.draw(orbit);
    for(int i=0;i<85;++i){sf::CircleShape star(i%13==0?2.5f:1.1f);star.setPosition(static_cast<float>((i*317+37)%820),static_cast<float>((i*137+29)%900));star.setFillColor(sf::Color(120,150,185,static_cast<sf::Uint8>(70+i%100)));m_window.draw(star);}
    const auto screen=m_menu.screen();
    if(screen==Screen::MainMenu){
        sf::ConvexShape ship(4);ship.setPoint(0,{410,58});ship.setPoint(1,{444,140});ship.setPoint(2,{410,122});ship.setPoint(3,{376,140});ship.setFillColor(sf::Color(40,180,245));ship.setOutlineThickness(3);ship.setOutlineColor(sf::Color(220,240,255));m_window.draw(ship);
        text("SPACE ARENA",410,156,64,sf::Color(219,240,255),true);
        text("S U R V I V E   T H E   U N K N O W N",410,250,19,sf::Color(112,158,194),true);
    } else {
        const char* title=screen==Screen::Paused?"PAUSA":screen==Screen::Scores?"SCORES":screen==Screen::Controls?"CONTROLES":screen==Screen::Mechanics?"MECÂNICAS":screen==Screen::GameOver?"FIM DE JOGO":screen==Screen::ScenarioSelection?"ESCOLHA O CENARIO":screen==Screen::CustomScenario?"CSV PERSONALIZADO":"NOVA PARTIDA";
        text(title,410,75,44,sf::Color(219,240,255),true);
    }
    if(screen==Screen::ScenarioSelection){
        text("A mesma aventura, tres tipos de arena.",410,200,22,sf::Color(135,183,218),true);
        text("Livre | Gerada por semente | Arena gerada por CSV",410,255,17,sf::Color(135,183,218),true);
    }
    if(screen==Screen::CustomScenario){
        text("Clique / Tab para editar o caminho; Enter carrega.",410,175,20,sf::Color(135,183,218),true);
        sf::RectangleShape field({680,70});field.setPosition(70,230);field.setFillColor(sf::Color(15,28,48));field.setOutlineThickness(2);field.setOutlineColor(m_pathEditing?sf::Color(40,195,245):sf::Color(44,71,100));m_window.draw(field);
        sf::Text path(sf::String::fromUtf8(m_menu.csvPath().begin(),m_menu.csvPath().end()),m_font,17);
        auto visible=path.getString();while(path.getLocalBounds().width>645 && visible.getSize()>3){visible.erase(0,1);path.setString(visible);}
        path.setPosition(85,251);path.setFillColor(sf::Color(210,235,250));m_window.draw(path);
        text("CSV: 0 = piso, 1 = parede. Sem cabecalho.",410,335,19,sf::Color(135,183,218),true);
    }
    if(screen==Screen::Paused)text("O universo aguarda. Todos os relogios estao pausados.",410,240,19,sf::Color(135,183,218),true);
    if(screen==Screen::ConfirmNewGame){text("Existe uma partida salva.",410,290,26,sf::Color(218,238,255),true);text("Substituir o salvamento por uma nova jornada?",410,345,21,sf::Color(145,185,218),true);}
    if(screen==Screen::Controls){
        const std::vector<std::pair<std::string,std::string>> keys={{"W A S D","Movimentar a nave"},{"MOUSE","Orientar a nave"},{"CLIQUE ESQUERDO","Disparar / selecionar"},{"ESC","Pausar, retomar ou voltar"},{"R","Nova partida apos Game Over"},{"SETAS / F / HOME","Camera / enquadrar / escala 1:1"},{"ENTER / SETAS","Confirmar / navegar nos menus"},{"TAB (DERROTA)","Editar nome no ranking"}};
        for(std::size_t i=0;i<keys.size();++i){float y=210+static_cast<float>(i)*74;Button key{{58,y,270,52},keys[i].first,true};key.draw(m_window,m_font,false);text(keys[i].second,350,y+13,18);}
    }
    if(screen==Screen::Scores){
        auto scores=m_menu.scores();
        text("#",60,185,19,sf::Color(120,205,248));text("PILOTO",110,185,19,sf::Color(120,205,248));text("PONTOS",410,185,19,sf::Color(120,205,248));text("ROUND",560,185,19,sf::Color(120,205,248));text("TEMPO",660,185,19,sf::Color(120,205,248));
        for(std::size_t i=0;i<scores.size();++i){const auto& e=scores[i];const float y=235+static_cast<float>(i)*47;
            text(std::to_string(i+1),60,y,18);text(e.name.substr(0,18),110,y,18);text(std::to_string(e.score),410,y,18);text(std::to_string(e.round),560,y,18);text(std::to_string(static_cast<int>(e.seconds))+"s",660,y,18);
        }
        if(scores.empty())text("Ainda nao ha resultados registrados.",410,350,23,sf::Color(140,180,210),true);
    }
    if(screen==Screen::GameOver){
        text("Pontuacao: "+std::to_string(m_game.score()),410,205,30,sf::Color(120,210,255),true);
        text("Round "+std::to_string(m_game.waves().round())+"  |  Sobrevivencia "+std::to_string(static_cast<int>(m_game.gameplayTime()))+" s",410,270,22,sf::Color(210,230,245),true);
        text("Inimigos eliminados: "+std::to_string(m_game.stats().enemyKills),410,315,22,sf::Color(210,230,245),true);
        text("Clique / Tab para editar; Enter registra (ate 32 caracteres).",410,405,18,sf::Color(135,183,218),true);
        sf::RectangleShape field({524,60});field.setPosition(148,445);field.setFillColor(sf::Color(15,28,48));field.setOutlineThickness(2);field.setOutlineColor(m_nameEditing?sf::Color(40,195,245):sf::Color(44,71,100));m_window.draw(field);
        text(m_menu.playerName()+(m_nameEditing?"_":""),410,450,30,sf::Color(125,215,255),true);
    }
    if(screen==Screen::Mechanics){
        const auto& c=m_game.config();auto number=[](double n){std::ostringstream o;o<<std::fixed<<std::setprecision(0)<<n;return o.str();};
        text("Pagina "+std::to_string(m_menu.mechanicsPage()+1)+" / 3  |  Use as setas esquerda e direita",410,155,18,sf::Color(125,180,220),true);
        std::vector<std::pair<std::string,std::string>> cards;
        if(m_menu.mechanicsPage()==0)cards={{"ALIEN NORMAL","Navega autonomamente. Detecta a nave a "+number(c.waves.enemy.detectionRadius)+" px.\nMais frequente. Nao gera drops."},{"ALIEN AVANCADO","Antena, perseguicao desde o nascimento e explosao de\n"+number(c.waves.enemy.explosionRadius)+" px ao tocar nave ou asteroide. Sem dano entre aliens.\nMorto pelo jogador: "+number(c.drops.chance*100)+"% de chance nominal de item."},{"ASTEROIDES","Trajetoria inicial retilinea; atingem nave e aliens.\nDestrutiveis, geram "+std::to_string(c.asteroid.fragmentCount)+" fragmentos ate nivel "+std::to_string(c.asteroid.maxFragmentDepth)+". Sofrem gravidade.\nTentativa a cada "+number(c.asteroid.spawnInterval)+" s, chance "+number(c.asteroid.spawnChance*100)+"%."}};
        if(m_menu.mechanicsPage()==1)cards={{"AREA GRAVITACIONAL","Atrai corpos dinamicos gradualmente, sem teletransporte.\nRaio "+number(c.gravity.radius)+" px; "+number(c.gravity.accelerationMeters)+" m/s2, "+number(c.gravity.pixelsPerMeter)+" px/m.\nA cada "+number(c.gravity.spawnInterval)+" s: "+number(c.gravity.spawnChance*100)+"% de chance; dura "+number(c.gravity.duration)+" s."},{"PORTAL","Surge quando nao ha aliens vivos nem pendentes.\nEntrar inicia o proximo round na mesma arena.\nSe surgir sob a nave, saia e entre novamente."},{"MOTORES","Quatro vidas. Perder motores reduz a velocidade.\nReparacao recupera um, ate quatro. Zero: Game Over.\nVelocidades 4/3/2/1: "+number(c.player.engineSpeeds[4])+" / "+number(c.player.engineSpeeds[3])+" / "+number(c.player.engineSpeeds[2])+" / "+number(c.player.engineSpeeds[1])+" px/s."}};
        if(m_menu.mechanicsPage()==2)cards={{"ARMAS E UPGRADES","Novo tipo desbloqueado por round, em ordem aleatoria.\nDesbloquear nao e adquirir: colete os itens.\nUm padrao ativo com modificadores compativeis e compensacoes."},{"ITENS","Motores e upgrades aparecem em posicoes validas aleatorias.\nMistura nominal: "+number((1-c.drops.upgradeChance)*100)+"% motor / "+number(c.drops.upgradeChance*100)+"% upgrade.\nItens duram "+number(c.pickup.lifetime)+" s. Reparos inuteis sao evitados."},{"PROGRESSAO","Aliens: "+std::to_string(c.waves.initialEnemies)+" no primeiro round, +"+std::to_string(c.waves.additionalEnemies)+" ate "+std::to_string(c.waves.maxEnemies)+".\nDesde round 2: "+number(c.waves.advancedChance*100)+"% avancados, mais resistencia.\nPontuacao por alien: "+std::to_string(c.scoring.normal)+" / "+std::to_string(c.scoring.advanced)+"; round: "+std::to_string(c.scoring.round)+"."}};
        for(std::size_t i=0;i<cards.size();++i){const float y=225+static_cast<float>(i)*180;
            const float radius=i==1?24.0f:18.0f;sf::CircleShape icon(radius,m_menu.mechanicsPage()==0?(i==0?6:i==1?8:7):m_menu.mechanicsPage()==1?(i<2?48:4):3);icon.setOrigin(radius,radius);icon.setPosition(85,y+40);icon.setFillColor(m_menu.mechanicsPage()==0?(i==1?sf::Color(220,105,245):i==0?sf::Color(245,85,95):sf::Color(125,115,105)):m_menu.mechanicsPage()==1?(i==0?sf::Color(110,165,255):i==1?sf::Color(185,130,255):sf::Color(70,235,150)):sf::Color(255,205,65));m_window.draw(icon);
            if(m_menu.mechanicsPage()==0 && i==1){sf::RectangleShape a({3,18});a.setPosition(84,y-2);a.setFillColor(sf::Color::Yellow);m_window.draw(a);}
            if(m_menu.mechanicsPage()==1 && i<2){sf::CircleShape ring(34,48);ring.setOrigin(34,34);ring.setPosition(85,y+40);ring.setFillColor(sf::Color::Transparent);ring.setOutlineThickness(2);ring.setOutlineColor(i==0?sf::Color(110,165,255):sf::Color(185,130,255));m_window.draw(ring);}
            text(cards[i].first,140,y,24,sf::Color(125,215,255));text(cards[i].second,140,y+43,17);
        }
    }
    const auto buttons=menuButtons();for(std::size_t i=0;i<buttons.size();++i)buttons[i].draw(m_window,m_font,static_cast<int>(i)==m_menu.selected());
    if(!m_menu.message().empty())text(m_menu.message().substr(0,105),410,865,14,sf::Color(245,194,100),true);
    else text("C++17  |  SFML  |  UTFPR",410,865,16,sf::Color(105,135,164),true);
}
