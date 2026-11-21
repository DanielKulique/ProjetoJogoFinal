// Controla janela, eventos, camera, desenho e o unico Game Loop da aplicacao.
#pragma once

#include "Game.hpp"
#include "NPC.hpp"
#include "Menu.hpp"
#include "Button.hpp"

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>

class DungeonVisualizer {
public:
    // Game deve viver mais que o visualizador; nao ha propriedade compartilhada.
    explicit DungeonVisualizer(Game& game, std::filesystem::path dataDirectory = SaveManager::defaultDirectory());
    bool init(unsigned widthPixels, unsigned heightPixels);
    unsigned run(unsigned frameLimit = 0); // Zero: executa ate fechar a janela.
    void setInitialCenter(int col, int row);
    [[nodiscard]] Vector2D mouseWorldPosition() const;

    // Extensao chamada apos Game::update: nao deve integrar a partida novamente.
    using UpdateFunction = std::function<void(float)>;
    void setUpdateFunction(UpdateFunction function);

    // Referencias nao proprietarias, como no exemplo. Remover antes de destruir o NPC.
    void registerNPC(NPC* npc);
    void unregisterNPC(NPC* npc);

private:
    Game& m_game;
    Menu m_menu;
    sf::View m_uiView;
    mutable bool m_inputBlocked{false};
    bool m_nameEditing{false};
    bool m_pathEditing{false};
    sf::RenderWindow m_window;
    sf::View m_view;
    sf::Font m_font;
    std::vector<NPC*> m_npc;
    UpdateFunction m_updateFunction;
    int m_centerCol{0};
    int m_centerRow{0};
    bool m_mapFitted{false};

    void handleEvent(const sf::Event& event);
    void tryMoveCenter(int deltaCol, int deltaRow);
    void fitMap();
    void updateViewport();
    void render();
    void renderHUD();
    void renderMenu();
    std::vector<Button> menuButtons() const;
    void updateUIViewport();
    [[nodiscard]] GameInput readInput() const;
};
