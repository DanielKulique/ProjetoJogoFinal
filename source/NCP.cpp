#include "NPC.hpp"

bool NPC::loadSprite(const std::string& path) {
    auto candidate = std::make_unique<sf::Texture>();
    if (!candidate->loadFromFile(path)) return false;
    sprite.setTexture(*candidate, true);
    texture = std::move(candidate);
    const auto size = texture->getSize();
    sprite.setOrigin(size.x * 0.5f, size.y * 0.5f);
    sprite.setScale(2 * collision.halfExtents.x / size.x,
                    2 * collision.halfExtents.y / size.y);
    return true;
}
