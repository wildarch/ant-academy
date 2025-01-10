#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
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

struct Obstacle {
  sf::FloatRect bounds;
};

struct PheromoneMap {
  float values[windowWidth / 4][windowHeight / 4];

  void evaporate(float percentage) {
    for (int x = 0; x < windowWidth / 4; x++) {
      for (int y = 0; y < windowHeight / 4; y++) {
        auto &amount = values[x][y];
        if (amount > 25) {
          // Cap at 100
          amount = 25;
        }

        amount *= (1.0f - percentage);
        if (amount < 0.5f) {
          amount = 0.0f;
        }
      }
    }
  }

  float blurKernel(int w, int h) {
    float middle = 0.5 + 0.125;
    float side = 0.125 * 0.5;
    float diag = 0.0625 * 0.5;
    return middle * values[w][h] + side * values[w - 1][h] +
           side * values[w + 1][h] + side * values[w][h - 1] +
           side * values[w][h + 1] + diag * values[w - 1][h - 1] +
           diag * values[w - 1][h + 1] + diag * values[w + 1][h - 1] +
           diag * values[w + 1][h + 1];
  }

  void blur() {
    // 3x3 gaussian blur
    for (int x = 1; x < windowWidth / 4 - 1; x++) {
      for (int y = 1; y < windowHeight / 4 - 1; y++) {
        values[x][y] = blurKernel(x, y);
      }
    }
  }
};

struct Environment {
  Nest nest;
  std::vector<FoodSource> food_sources;
  std::vector<Obstacle> obstacles;
  // Follow if looking for home, deposit if coming from home.
  PheromoneMap homePheromone;
  // Follow if looking for food, deposit if coming from food.
  PheromoneMap foodPheromone;
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
  int pheromoneAvailable = 2000;
  int confusion = 0;

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
      // rotation = fmodf(rotation, 180);
    }
  }

  void rotateTowardsPheromone(const PheromoneMap &pheromones, int chance) {
    int gridX = (int)floor(position.x / 4);
    int gridY = (int)floor(position.y / 4);
    // Look around for pheromones
    sf::Transform antTransform;
    antTransform.rotate(rotation);
    auto ownDirection = antTransform.transformPoint(sf::Vector2f(0, -1));
    sf::Vector2f pheromoneSum;
    for (int x = -10; x <= 10; x++) {
      for (int y = -10; y <= 10; y++) {
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
          float pheromone_level = pheromones.values[absX][absY];
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
      if (std::rand() % chance == 0) {
        auto softTarget = ownDirection + (pheromoneSum * 0.2f);
        rotation = rotationDegrees(softTarget);
      }
    }
  }

  /*
  void rotateTowardsNest(const Environment &environment, int chance) {
    if (std::rand() % chance == 0) {
      auto target = nestPosition - position;
      rotation = rotationDegrees(target);
    }
  }
   */

  void depositPheromone(PheromoneMap &pheromones) {
    int gridX = (int)floor(position.x / 4);
    int gridY = (int)floor(position.y / 4);

    if (pheromoneAvailable <= 0.0f) {
      return;
    }

    // Deposit pheromones at the current position.
    auto amount = (pheromoneAvailable * 0.0005) + 0.01;
    if (gridX >= 0 && gridX < windowWidth / 4 && gridY >= 0 &&
        gridY < windowHeight / 4) {
      pheromones.values[gridX][gridY] += amount;
      pheromoneAvailable -= amount;
    }
  }

  /** \brief Execute ant behavior.
   *  \note  To be called on every simulation step.
   */
  virtual void update(Environment &environment) {
    // Random movement while searching.

    // HACK: Always have pheromone
    // pheromoneAvailable = 4000;

    if (state == State::SEARCHING) {
      hue = 100;
      // If position is very near food source and there is food available,
      // return.
      for (auto &food : environment.food_sources) {
        auto foodDistance = length(food.position - position);
        if (foodDistance < 80.0 && food.amount_left > 0) {
          food.amount_left--;
          state = State::RETURNING;
          pheromoneAvailable = 2000;
          rotation += 180;
        }
      }

      randomAdjustVelocity();
      randomAdjustRotation();

      if (confusion == 0) {
        rotateTowardsPheromone(environment.foodPheromone, 4);
        depositPheromone(environment.homePheromone);
      }
    }
    // Move in a straight line to the base
    else if (state == State::RETURNING) {
      hue = 0;
      // If position is very near home, start searching.
      auto nest_dist = length(environment.nest.position - position);
      if (nest_dist < 80.0) {
        state = State::SEARCHING;
        pheromoneAvailable = 2000;
        rotation += 180;
        environment.antsReturned++;
      }

      randomAdjustVelocity();
      randomAdjustRotation();
      if (confusion == 0) {
        rotateTowardsPheromone(environment.homePheromone, 4);
        // rotateTowardsNest(environment, 400);
        depositPheromone(environment.foodPheromone);
      }
    }

    // Update position based on current rotation and velocity
    sf::Transform antTransform;
    antTransform.rotate(rotation);
    auto newPosition =
        position + antTransform.transformPoint(sf::Vector2f(0, -velocity));

    bool hitObstacle = false;
    // If we fall off the map, get confused.
    if (newPosition.x <= 0) {
      hitObstacle = true;
    } else if (newPosition.x >= windowWidth) {
      hitObstacle = true;
    } else if (newPosition.y <= 0) {
      // newPosition.y = windowHeight + 100;
      hitObstacle = true;
    } else if (newPosition.y >= windowHeight) {
      hitObstacle = true;
      // newPosition.y = -100;
    }

    // Check for collisions
    for (auto &obs : environment.obstacles) {
      if (obs.bounds.contains(newPosition)) {
        hitObstacle = true;
        break;
      }
    }

    if (hitObstacle) {
      // Not allowed to move here.
      rotation += 90;
      confusion = std::min(confusion + 100, 1000);
    } else {
      if (confusion > 0) {
        confusion--;
      }
      position = newPosition;
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

            // Multiply vector with dot product of vector and ant
            // direction
            float inner_size = inner(unit, ownDirection);
            // Ignore values behind us.
            if (inner_size <= 0) {
              continue;
            }

            unit *= inner_size;

            // Multiply vector with peromone level
            float pheromone_level =
                environment.homePheromone.values[absX][absY];
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

  Environment environment{.nest{
                              .position = sf::Vector2f(600, 400),
                              .nest_size = 100,
                          },
                          .food_sources = {
                              FoodSource{
                                  .position = sf::Vector2f(1200, 800),
                                  .amount_left = 150,
                              },
                              /*
                              FoodSource{
                                  .position = sf::Vector2f(50, 50),
                              },
                              FoodSource{
                                  .position = sf::Vector2f(50, 800),
                              },
                              FoodSource{
                                  .position = sf::Vector2f(1200, 50),
                              },
                              */
                          }};
  environment.obstacles.push_back(Obstacle{
      .bounds = sf::FloatRect(400, 600, 800, 50),
  });

  holeSprite.setPosition(environment.nest.position);

  sf::Sprite foodSprite;
  foodSprite.setTexture(terrainTexture);
  foodSprite.setTextureRect(sf::IntRect(256, 544, 32, 24));
  foodSprite.setScale(3.0, 3.0);
  foodSprite.setOrigin(18, 16);

  sf::Clock simulationStartClock;
  sf::Clock antSpawnClock;
  sf::Clock blurClock;

  std::vector<sf::Vertex> pheromoneTiles((windowWidth / 4) *
                                         (windowHeight / 4) * 6);

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

    if (blurClock.getElapsedTime().asMilliseconds() > 1000) {
      blurClock.restart();
      // Evaporate & draw pheromones.
      environment.homePheromone.evaporate(0.003);
      environment.foodPheromone.evaporate(0.002);
      environment.homePheromone.blur();
      environment.foodPheromone.blur();
    }

    int vidx = 0;
    for (int x = 0; x < windowWidth / 4; x++) {
      for (int y = 0; y < windowHeight / 4; y++) {
        auto homeAmount = environment.homePheromone.values[x][y];
        auto foodAmount = environment.foodPheromone.values[x][y];
        if (foodAmount > 0.0f || homeAmount > 0.0f) {
          sf::Vertex *triangles = &pheromoneTiles[vidx];
          triangles[0].position = sf::Vector2f(x * 4, y * 4);
          triangles[1].position = sf::Vector2f((x + 1) * 4, y * 4);
          triangles[2].position = sf::Vector2f(x * 4, (y + 1) * 4);
          triangles[3].position = sf::Vector2f(x * 4, (y + 1) * 4);
          triangles[4].position = sf::Vector2f((x + 1) * 4, y * 4);
          triangles[5].position = sf::Vector2f((x + 1) * 4, (y + 1) * 4);

          sf::Color color = sf::Color(
              fminf(255, 50 * homeAmount), fminf(255, 50 * foodAmount),
              fminf(255, 50 * (homeAmount + foodAmount)),
              fminf(255, 10 * fmaxf(homeAmount, foodAmount)));
          triangles[0].color = color;
          triangles[1].color = color;
          triangles[2].color = color;
          triangles[3].color = color;
          triangles[4].color = color;
          triangles[5].color = color;

          vidx += 6;
          // window.draw(shape);
        }

        /*
          shape.setPosition(x * 4, y * 4);
          shape.setFillColor(sf::Color(0, 180, 180, foodAmount * 10));
          // window.draw(shape);
        }
        */
      }
    }

    window.draw(pheromoneTiles.data(), vidx, sf::PrimitiveType::Triangles);

    Nest &nest = environment.nest;
    if (nest.ants.size() < nest.nest_size &&
        antSpawnClock.getElapsedTime().asSeconds() > 0.5) {
      // Add a new ant.
      antSpawnClock.restart();
      nest.ants.push_back(std::make_unique<Ant>(
          nest.position, 0, float(std::rand() % 360), std::rand() % 63,
          nest.position, float(std::rand() % 360), Ant::State::SEARCHING));
    }

    // Obstacles
    sf::RectangleShape obstacleShape;
    for (auto &obs : environment.obstacles) {
      obstacleShape.setSize(sf::Vector2f(obs.bounds.width, obs.bounds.height));
      obstacleShape.setPosition(obs.bounds.left, obs.bounds.top);
      obstacleShape.setFillColor(sf::Color::Black);
      window.draw(obstacleShape);
    }

    for (auto &ant : nest.ants) {
      ant->update(environment);

      ant->animateStep();
      ant->draw(window, antSprite);
    }

    window.display();

    if (simulationStartClock.getElapsedTime().asSeconds() > 5.0) {
      /*
      std::cout << "Ants returned per second: "
                << (environment.antsReturned /
                    simulationStartClock.getElapsedTime().asSeconds())
                << "\n";
      */
      for (auto foodsource : environment.food_sources) {
        std::cout << "Amount left: " << foodsource.amount_left << "\n";
      }

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
