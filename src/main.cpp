#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

int main() {
  uint32_t windowWidth = 1920;
  uint32_t windowHeight = 1080;
  auto window = sf::RenderWindow{{windowWidth, windowHeight}, "Ant Academy"};
  window.setFramerateLimit(144);

  while (window.isOpen()) {
    for (auto event = sf::Event{}; window.pollEvent(event);) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    window.clear();

    // Draw grid for reference.
    constexpr uint32_t tileSize = 60;
    sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
    for (size_t x = 0; x < windowWidth; x += tileSize) {
      for (size_t y = 0; y < windowHeight; y += tileSize) {
        tile.setPosition(sf::Vector2f(x, y));
        if (((x / tileSize + y / tileSize) % 2) == 0) {
          tile.setFillColor(sf::Color(20, 20, 20));
        } else {
          tile.setFillColor(sf::Color(60, 60, 60));
        }
        window.draw(tile);
      }
    }

    window.display();
  }
}