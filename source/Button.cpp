#include "Button.hpp"
#include <cmath>
void Button::draw(sf::RenderTarget& target,const sf::Font& font,bool selected) const {
    sf::ConvexShape box(28);
    const float radius=8;
    for(int corner=0;corner<4;++corner){
        const sf::Vector2f center{corner==0 || corner==3?radius:bounds.width-radius,corner<2?radius:bounds.height-radius};
        for(int step=0;step<7;++step){const float angle=(180+corner*90+step*15)*3.14159265358979323846f/180;
            box.setPoint(static_cast<std::size_t>(corner*7+step),{center.x+radius*std::cos(angle),center.y+radius*std::sin(angle)});}
    }
    box.setPosition(bounds.left,bounds.top);
    box.setFillColor(!enabled?sf::Color(12,21,35):selected?sf::Color(19,57,77):sf::Color(15,28,48));
    box.setOutlineThickness(2);box.setOutlineColor(!enabled?sf::Color(29,43,62):selected?sf::Color(40,195,245):sf::Color(44,71,100));target.draw(box);
    sf::Text text(sf::String::fromUtf8(label.begin(),label.end()),font,26);text.setStyle(sf::Text::Bold);text.setLetterSpacing(1.2f);
    if(text.getLocalBounds().width>bounds.width-24)text.setCharacterSize(static_cast<unsigned>(26*(bounds.width-24)/text.getLocalBounds().width));
    text.setFillColor(!enabled?sf::Color(83,104,127):selected?sf::Color(125,215,255):sf::Color(207,222,241));
    const auto b=text.getLocalBounds();text.setOrigin(b.left+b.width/2,b.top+b.height/2);text.setPosition(bounds.left+bounds.width/2,bounds.top+bounds.height/2);target.draw(text);
}
