#ifndef GAME_NPC_HPP
#define GAME_NPC_HPP

#include <SFML/Graphics.hpp>
#include "Entity.hpp"
#include <string>
#include <memory>

/**
 * @file NPC.hpp
 * @brief A non-player entity with a visible sprite.
 *
 * NPC is an Entity plus a sprite for on-screen display. The sprite is a public
 * member: the game loop reads and writes it directly each frame to position and
 * draw the NPC.
 */
struct NPC : Entity {
    // A textura pertence ao NPC e deve viver mais que seu sprite.
    // Alocacao tardia: NPCs logicos podem existir sem display/contexto OpenGL.
    std::unique_ptr<sf::Texture> texture;
    sf::Sprite sprite;   ///< Public: positioned and drawn by the game loop.

    NPC() = default;
    NPC(const NPC&) = delete;
    NPC& operator=(const NPC&) = delete;
    bool loadSprite(const std::string& path);
};

#endif // GAME_NPC_HPP
