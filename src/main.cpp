#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>

struct Ant {
  sf::Vector2f position;
  float velocity;
  float rotation = 0;
  int stepCounter = 0;
};

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

  float antCircleDegree = 0;
  auto antBasePosition = sf::Vector2f(1000, 500);

  std::vector<Ant> ants(100);
  for (auto &ant : ants) {
    ant.stepCounter = std::rand() % 63;
    ant.position = sf::Vector2f(std::rand() % 1900, std::rand() % 1000);
    ant.rotation = std::rand() % 360;
  }

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

    for (auto &ant : ants) {
      ant.velocity += ((std::rand() % 11) - 5) / 30.f;
      ant.velocity = std::max(ant.velocity, 0.f);
      ant.rotation += (std::rand() % 361 - 180) / 50.0f;

      sf::Transform antTransform;

      antTransform.rotate(ant.rotation);
      ant.position +=
          antTransform.transformPoint(sf::Vector2f(0, -ant.velocity));
      ant.stepCounter++;
      if (ant.stepCounter == 62) {
        ant.stepCounter = 0;
      }

      // Draw ants
      int left = (ant.stepCounter % 8) * 202;
      int top = (ant.stepCounter / 8) * 248;
      antSprite.setTextureRect(sf::IntRect(left, top, 202, 248));
      antSprite.setPosition(sf::Vector2f(1000, 500));
      antSprite.setScale(0.5f, 0.5f);
      antSprite.setOrigin(sf::Vector2f(101, 124));

      // Walk in a circle.
      // antTransform.translate(antCircleDegree, 0);
      antTransform.rotate(antCircleDegree);
      antSprite.setPosition(ant.position);
      antSprite.setRotation(ant.rotation);
      window.draw(antSprite);
    }

    window.display();
  }
}