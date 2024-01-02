#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>

int main() {
  uint32_t windowWidth = 1920;
  uint32_t windowHeight = 1080;
  auto window = sf::RenderWindow{{windowWidth, windowHeight}, "Ant Academy"};
  window.setFramerateLimit(144);

  sf::Texture antTexture;
  if (!antTexture.loadFromFile("ant.png")) {
    // TODO
    std::cerr << "Failed to load ant.png\n";
    return 1;
  }

  sf::Sprite antSprite;
  antSprite.setTexture(antTexture);
  int antStepCount = 0;

  float antCircleDegree = 0;
  auto antBasePosition = sf::Vector2f(1000, 500);

  while (window.isOpen()) {
    for (auto event = sf::Event{}; window.pollEvent(event);) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    window.clear(sf::Color(200, 200, 200));

    // Draw grid for reference.
    constexpr uint32_t tileSize = 60;
    sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
    tile.setFillColor(sf::Color(150, 150, 150));
    for (size_t x = 0; x < windowWidth; x += tileSize) {
      for (size_t y = 0; y < windowHeight; y += tileSize) {
        tile.setPosition(sf::Vector2f(x, y));
        if (((x / tileSize + y / tileSize) % 2) == 0) {
          window.draw(tile);
        }
      }
    }

    // Draw ants
    int left = (antStepCount % 8) * 202;
    int top = (antStepCount / 8) * 248;
    antSprite.setTextureRect(sf::IntRect(left, top, 202, 248));
    antSprite.setPosition(sf::Vector2f(1000, 500));
    antSprite.setScale(0.5f, 0.5f);
    antSprite.setOrigin(sf::Vector2f(101, 124));

    // Walk in a circle.
    antCircleDegree -= 0.3;
    sf::Transform antTransform;
    // antTransform.translate(antCircleDegree, 0);
    antTransform.rotate(antCircleDegree);
    antSprite.setPosition(sf::Vector2f(1000, 500) +
                          antTransform.transformPoint(sf::Vector2f(100, 0)));
    antSprite.setRotation(antCircleDegree);

    window.draw(antSprite);

    window.display();
    antStepCount++;
    if (antStepCount == 62) {
      antStepCount = 0;
    }
  }
}