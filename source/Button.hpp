#pragma once
#include <SFML/Graphics.hpp>
#include <string>
struct Button {
    sf::FloatRect bounds;
    std::string label;
    bool enabled{true};
    void draw(sf::RenderTarget& target,const sf::Font& font,bool selected) const;
};
