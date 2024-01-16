#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <math.h>

constexpr uint32_t windowWidth = 1600;
constexpr uint32_t windowHeight = 900;

struct Ant {
  sf::Vector2f position;
  float velocity;
  float rotation = 0;
  int stepCounter = 0;
  sf::Vector2f nestPosition;

  void updatePosition() {
    if (std::rand() % 5 == 0) {
      velocity += ((std::rand() % 11) - 5) / 20.f;
    }
    velocity = std::max(velocity, 0.f);
    velocity = std::min(velocity, 4.0f);
    if (std::rand() % 20 == 0) {
      // Periodically sample a new rotation
      rotation += (std::rand() % 361 - 180) / 10.0f;
    }

    sf::Transform antTransform;

    antTransform.rotate(rotation);
    position += antTransform.transformPoint(sf::Vector2f(0, -velocity));

    if (position.x < -100) {
      position.x = windowWidth + 100;
    } else if (position.x > windowWidth + 100) {
      position.x = -100;
    }

    if (position.y < -100) {
      position.y = windowHeight + 100;
    } else if (position.y > windowHeight + 100) {
      position.y = -100;
    }
  }

  void animateStep() {
    stepCounter++;
    if (stepCounter == 62) {
      stepCounter = 0;
    }
  }

  void draw(sf::RenderWindow &window, sf::Sprite antSprite) {
    int left = (stepCounter % 8) * 202;
    int top = (stepCounter / 8) * 248;
    antSprite.setTextureRect(sf::IntRect(left, top, 202, 248));
    antSprite.setPosition(position);
    antSprite.setRotation(rotation);
    window.draw(antSprite);
  }
};

struct Nest {
  sf::Vector2f position;
  std::vector<Ant> ants;
  int nest_size;
};

int main() {
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
  antSprite.setScale(0.5f, 0.5f);
  antSprite.setOrigin(sf::Vector2f(101, 124));

  sf::Texture terrainTexture;
  if (!terrainTexture.loadFromFile("terrain.png")) {
    std::cerr << "Failed to load terrain.png\n";
    return 1;
  }

  sf::Sprite holeSprite;
  holeSprite.setTexture(terrainTexture);
  holeSprite.setTextureRect(sf::IntRect(680, 70, 80, 80));
  holeSprite.setScale(2.0, 2.0);
  holeSprite.setOrigin(40, 40);

  Nest nest{
      .position = sf::Vector2f(100, 100),
      .nest_size = 100,
  };

  holeSprite.setPosition(nest.position);

  nest.ants.emplace_back(Ant{
      .position = nest.position,
      .nestPosition = nest.position,
  });

  sf::Clock antSpawnClock;

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

    window.draw(holeSprite);

    if (nest.ants.size() < nest.nest_size &&
        antSpawnClock.getElapsedTime().asSeconds() > 0.5) {
      // Add a new ant.
      antSpawnClock.restart();
      nest.ants.emplace_back(Ant{
          .position = nest.position,
          .rotation = float(std::rand() % 360),
          .stepCounter = std::rand() % 63,
          .nestPosition = nest.position,

      });
    }

    for (auto &ant : nest.ants) {
      ant.updatePosition();
      ant.animateStep();
      ant.draw(window, antSprite);
    }

    window.display();
  }
}