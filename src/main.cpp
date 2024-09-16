#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <memory>

constexpr uint32_t windowWidth = 1600;
constexpr uint32_t windowHeight = 900;

static sf::Color hsv2rgb(double hue, double sat, double val);

// Vector helper functions
float inner(sf::Vector2f v, sf::Vector2f u) { return v.x * u.x + v.y * u.y; }
float length(sf::Vector2f v) { return std::sqrt(inner(v, v)); }
sf::Vector2f normalize(sf::Vector2f v) { return v / length(v); }
float rotationDegrees(sf::Vector2f v) { return 180 * atan2(v.x, -v.y) / M_PI; }
sf::Vector2f vectorOf(float angleDegrees) {
  sf::Transform t;
  t.rotate(angleDegrees);
  return t.transformPoint(sf::Vector2f(0, -1));
}

struct Ant;

struct Nest {
  sf::Vector2f position;
  std::vector<std::unique_ptr<Ant>> ants;
  int nest_size;
};

struct FoodSource {
  sf::Vector2f position;
  int amount_left;
};

struct Environment {
  Nest nest;
  std::vector<FoodSource> food_sources;
  float pheromones[windowWidth / 4][windowHeight / 4];
  int antsReturned = 0;
};

std::ostream &operator<<(std::ostream &out, sf::Vector2f const &v) {
  out << "(" << v.x << ", " << v.y << ")";
  return out;
}

struct Ant {
  sf::Vector2f position;
  float velocity;
  float rotation = 0;
  int stepCounter = 0;
  sf::Vector2f nestPosition;
  float hue;
  int pheromoneAvailable = 0;

  enum class State {
    SEARCHING,
    RETURNING,
  } state;

  Ant(sf::Vector2f position, float velocity, float rotation, int stepCounter,
      sf::Vector2f nestPosition, float hue, State state)
      : position(position), velocity(velocity), rotation(rotation),
        stepCounter(stepCounter), nestPosition(nestPosition), hue(hue),
        state(state) {}

  void randomAdjustVelocity() {
    if (std::rand() % 5 == 0) {
      velocity += ((std::rand() % 11) - 5) / 40.f;
    }
    velocity = std::max(velocity, 0.f);
    velocity = std::min(velocity, 2.0f);
  }

  void randomAdjustRotation() {
    if (std::rand() % 20 == 0) {
      // Periodically sample a new rotation
      rotation += (std::rand() % 361 - 180) / 10.0f;
      rotation = fmodf(rotation, 180);
    }
  }

  void rotateTowardsPheromone(const Environment &environment, int chance) {
    int gridX = (int)floor(position.x / 4);
    int gridY = (int)floor(position.y / 4);
    // Look around for pheromones
    sf::Transform antTransform;
    antTransform.rotate(rotation);
    auto ownDirection = antTransform.transformPoint(sf::Vector2f(0, -1));
    sf::Vector2f pheromoneSum;
    for (int x = -5; x <= 5; x++) {
      for (int y = -5; y <= 5; y++) {
        if (x == 0 && y == 0) {
          continue;
        }

        auto absX = gridX + x;
        auto absY = gridY + y;
        if (absX >= 0 && absX < windowWidth / 4 && absY >= 0 &&
            absY < windowHeight / 4) {
          // Create unit vector from x and y
          sf::Vector2f unit(x, y);
          unit = normalize(unit);

          // Multiply vector with dot product of vector and ant direction
          float inner_size = inner(unit, ownDirection);
          // Ignore values behind us.
          if (inner_size <= 0) {
            continue;
          }

          unit *= inner_size;

          // Multiply vector with peromone level
          float pheromone_level = environment.pheromones[absX][absY];
          unit *= pheromone_level;

          // Add vector to pheromoneSum
          pheromoneSum += unit;
        }
      }
    }
    // Check if pheromoneSum is nonzero
    if (pheromoneSum.x != 0.0 && pheromoneSum.y != 0.0) {
      pheromoneSum = normalize(pheromoneSum);
      // Create direction from pheromoneSum
      // std::cout << "Pheromone sum is: " << pheromoneSum << std::endl;
      // std::cout << "Pheromone sum rotation is: "
      //          << rotationDegrees(pheromoneSum) << "\n";
      // std::cout << "Current rotation: " << rotation << "\n";
      // rotation = rotationDegrees(pheromoneSum);

      if (std::rand() % chance == 0) {
        auto softTarget = ownDirection + (pheromoneSum * 0.2f);
        rotation = rotationDegrees(softTarget);
      }
    }
  }

  void depositPheromone(Environment &environment) {
    int gridX = (int)floor(position.x / 4);
    int gridY = (int)floor(position.y / 4);

    if (pheromoneAvailable <= 0.0f) {
      return;
    }

    // Deposit pheromones at the current position.
    auto amount = (pheromoneAvailable * 0.0005) + 0.01;
    if (gridX >= 0 && gridX < windowWidth / 4 && gridY >= 0 &&
        gridY < windowHeight / 4) {
      environment.pheromones[gridX][gridY] += amount;
      pheromoneAvailable -= amount;
    }
  }

  /** \brief Execute ant behavior.
   *  \note  To be called on every simulation step.
   */
  virtual void update(Environment &environment) {
    // Random movement while searching.
    if (state == State::SEARCHING) {
      hue = 100;
      // If position is very near food source, return.
      for (auto food : environment.food_sources) {
        auto foodDistance = length(food.position - position);
        if (foodDistance < 80.0) {
          state = State::RETURNING;
          pheromoneAvailable = 2000;
        }
      }

      randomAdjustVelocity();
      randomAdjustRotation();
      rotateTowardsPheromone(environment, 3);
    }
    // Move in a straight line to the base
    else if (state == State::RETURNING) {
      hue = 0;
      // If position is very near home, start searching.
      auto nest_dist = length(environment.nest.position - position);
      if (nest_dist < 80.0) {
        state = State::SEARCHING;
        environment.antsReturned++;
      }

      randomAdjustVelocity();
      randomAdjustRotation();
      rotateTowardsPheromone(environment, 20);
      depositPheromone(environment);
    }

    // Update position based on current rotation and velocity
    sf::Transform antTransform;
    antTransform.rotate(rotation);
    position += antTransform.transformPoint(sf::Vector2f(0, -velocity));
    if (position.x < -100) {
      position.x = windowWidth + 100;
    } else if (position.x > windowWidth + 100) {
      position.x = -100;
    }

    // If we fall off the map, re-appear on the other sideo.
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
    antSprite.setColor(hsv2rgb(hue, 1, 120));
    window.draw(antSprite);
  }
};

struct ControllableAnt : public Ant {
  using Ant::Ant;

  void update(Environment &environment) override {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
      rotation -= 1;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
      rotation += 1;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
      velocity += 0.01;
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
      velocity -= 0.01;
    } else if (velocity > 0) {
      velocity *= 0.99;
    }

    // Apply velocity
    sf::Transform antTransform;
    antTransform.rotate(rotation);
    position += antTransform.transformPoint(sf::Vector2f(0, -velocity));

    // Don't fall off the screen
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

    {
      // Pheromones analysis
      int gridX = (int)floor(position.x / 4);
      int gridY = (int)floor(position.y / 4);

      sf::Transform antTransform;
      antTransform.rotate(rotation);
      auto ownDirection = antTransform.transformPoint(sf::Vector2f(0, -1));

      sf::Vector2f pheromoneSum;
      for (int x = -5; x <= 5; x++) {
        for (int y = -5; y <= 5; y++) {
          if (x == 0 && y == 0) {
            continue;
          }

          auto absX = gridX + x;
          auto absY = gridY + y;
          if (absX >= 0 && absX < windowWidth / 4 && absY >= 0 &&
              absY < windowHeight / 4) {
            // Create unit vector from x and y
            sf::Vector2f unit(x, y);
            unit = normalize(unit);

            // Multiply vector with dot product of vector and ant direction
            float inner_size = inner(unit, ownDirection);
            // Ignore values behind us.
            if (inner_size <= 0) {
              continue;
            }

            unit *= inner_size;

            // Multiply vector with peromone level
            float pheromone_level = environment.pheromones[absX][absY];
            unit *= pheromone_level;

            // Add vector to pheromoneSum
            pheromoneSum += unit;
          }
        }
      }
      // Check if pheromoneSum is nonzero
      if (pheromoneSum.x != 0.0 && pheromoneSum.y != 0.0) {
        pheromoneSum = normalize(pheromoneSum);
        // Create direction from pheromoneSum
        // std::cout << "Pheromone sum is: " << pheromoneSum << std::endl;
        // std::cout << "Pheromone sum rotation is: "
        //          << rotationDegrees(pheromoneSum) << "\n";
        // std::cout << "Current rotation: " << rotation << "\n";
      }
    }
  }
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
  antSprite.setScale(0.25f, 0.25f);
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

  Environment environment{
      .nest{
          .position = sf::Vector2f(600, 400),
          .nest_size = 100,
      },
      .food_sources = {FoodSource{
                           .position = sf::Vector2f(1200, 800),
                       },
                       FoodSource{
                           .position = sf::Vector2f(50, 50),
                       },
                       FoodSource{
                           .position = sf::Vector2f(50, 800),
                       },
                       FoodSource{
                           .position = sf::Vector2f(1200, 50),
                       }}};

  holeSprite.setPosition(environment.nest.position);

  sf::Sprite foodSprite;
  foodSprite.setTexture(terrainTexture);
  foodSprite.setTextureRect(sf::IntRect(256, 544, 32, 24));
  foodSprite.setScale(3.0, 3.0);
  foodSprite.setOrigin(18, 16);

  sf::Clock simulationStartClock;
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

    for (auto &food : environment.food_sources) {
      foodSprite.setPosition(food.position);
      window.draw(foodSprite);
    }

    // Evaporate & draw pheromones.
    sf::CircleShape shape(10);
    shape.setOrigin(5, 5);
    for (int x = 0; x < windowWidth / 4; x++) {
      for (int y = 0; y < windowHeight / 4; y++) {
        auto &amount = environment.pheromones[x][y];
        amount *= 0.999f;
        if (amount < 0.5f) {
          amount = 0.0f;
        } else {
          shape.setPosition(x * 4, y * 4);
          shape.setFillColor(sf::Color(180, 0, 180, amount * 10));
          window.draw(shape);
        }
      }
    }

    Nest &nest = environment.nest;
    if (nest.ants.size() < nest.nest_size &&
        antSpawnClock.getElapsedTime().asSeconds() > 0.5) {
      // Add a new ant.
      antSpawnClock.restart();
      nest.ants.push_back(std::make_unique<Ant>(
          nest.position, 0, float(std::rand() % 360), std::rand() % 63,
          nest.position, float(std::rand() % 360), Ant::State::SEARCHING));
    }

    for (auto &ant : nest.ants) {
      ant->update(environment);

      ant->animateStep();
      ant->draw(window, antSprite);
    }

    window.display();

    if (simulationStartClock.getElapsedTime().asSeconds() > 5.0) {
      std::cout << "Ants returned per second: "
                << (environment.antsReturned /
                    simulationStartClock.getElapsedTime().asSeconds())
                << "\n";
      simulationStartClock.restart();
      environment.antsReturned = 0;
    }
  }
}

// see:
// https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
static sf::Color hsv2rgb(double hue, double sat, double val) {
  double hh, p, q, t, ff;
  long i;
  sf::Color out;
  out.a = 255;

  if (sat <= 0.0) { // < is bogus, just shuts up warnings
    out.r = val;
    out.g = val;
    out.b = val;
    return out;
  }
  hh = hue;
  if (hh >= 360.0)
    hh = 0.0;
  hh /= 60.0;
  i = (long)hh;
  ff = hh - i;
  p = val * (1.0 - sat);
  q = val * (1.0 - (sat * ff));
  t = val * (1.0 - (sat * (1.0 - ff)));

  switch (i) {
  case 0:
    out.r = val;
    out.g = t;
    out.b = p;
    break;
  case 1:
    out.r = q;
    out.g = val;
    out.b = p;
    break;
  case 2:
    out.r = p;
    out.g = val;
    out.b = t;
    break;

  case 3:
    out.r = p;
    out.g = q;
    out.b = val;
    break;
  case 4:
    out.r = t;
    out.g = p;
    out.b = val;
    break;
  case 5:
  default:
    out.r = val;
    out.g = p;
    out.b = q;
    break;
  }
  return out;
}
