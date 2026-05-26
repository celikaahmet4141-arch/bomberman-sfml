#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <cmath>
#include <random>
#include <algorithm>

const int TILE_SIZE = 36;
const int ROWS = 21;
const int COLS = 25;

// 0 = Empty path
// 1 = Solid wall
// 2 = Breakable crate
int level1[ROWS][COLS] =
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,2,0,0,0,0,0,2,0,0,1,0,0,0,0,0,1},
    {1,0,1,0,1,0,1,0,1,0,0,2,0,2,0,0,1,0,1,0,1,0,1,0,1},
    {1,0,0,2,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,0,0,2,0,0,1},
    {1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1},
    {1,0,0,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,0,0,1},
    {1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,1},
    {1,0,1,0,1,0,1,0,1,2,1,0,1,0,1,0,1,2,1,0,1,0,1,0,1},
    {1,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,1},
    {1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1},
    {1,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,1},
    {1,0,1,0,1,0,1,0,1,2,1,0,1,0,1,0,1,2,1,0,1,0,1,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,1},
    {1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1},
    {1,0,0,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,0,0,1},
    {1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1},
    {1,0,0,2,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,0,0,2,0,0,1},
    {1,0,1,0,1,0,1,0,1,0,0,2,0,2,0,0,1,0,1,0,1,0,1,0,1},
    {1,0,0,0,0,0,1,0,0,2,0,0,0,0,0,2,0,0,1,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

enum class Direction
{
    Up,
    Down,
    Left,
    Right
};

struct Enemy
{
    float x;
    float y;
    float speed;
    Direction direction;
    float decisionTimer;
    float stuckTimer;
    sf::Vector2f lastPosition;
    std::vector<sf::Vector2i> recentTiles;
};

bool canMoveToPixel(float x, float y);

sf::Vector2f getDirectionVector(Direction direction)
{
    if (direction == Direction::Up)
        return sf::Vector2f(0.f, -1.f);

    if (direction == Direction::Down)
        return sf::Vector2f(0.f, 1.f);

    if (direction == Direction::Left)
        return sf::Vector2f(-1.f, 0.f);

    return sf::Vector2f(1.f, 0.f);
}

Direction getOppositeDirection(Direction direction)
{
    if (direction == Direction::Up)
        return Direction::Down;

    if (direction == Direction::Down)
        return Direction::Up;

    if (direction == Direction::Left)
        return Direction::Right;

    return Direction::Left;
}

sf::Vector2i getTileFromPixel(float x, float y)
{
    int col = static_cast<int>((x + TILE_SIZE / 2.f) / TILE_SIZE);
    int row = static_cast<int>((y + TILE_SIZE / 2.f) / TILE_SIZE);

    return sf::Vector2i(col, row);
}

bool isWalkableTile(int row, int col)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return false;

    return level1[row][col] == 0;
}

int countOpenNeighborTiles(int row, int col)
{
    int count = 0;

    if (isWalkableTile(row - 1, col))
        count++;

    if (isWalkableTile(row + 1, col))
        count++;

    if (isWalkableTile(row, col - 1))
        count++;

    if (isWalkableTile(row, col + 1))
        count++;

    return count;
}

bool isDeadEndTile(int row, int col)
{
    if (!isWalkableTile(row, col))
        return false;

    return countOpenNeighborTiles(row, col) <= 1;
}

sf::Vector2i getTargetTileForDirection(const Enemy& enemy, Direction direction)
{
    sf::Vector2i currentTile = getTileFromPixel(enemy.x, enemy.y);

    if (direction == Direction::Up)
        return sf::Vector2i(currentTile.x, currentTile.y - 1);

    if (direction == Direction::Down)
        return sf::Vector2i(currentTile.x, currentTile.y + 1);

    if (direction == Direction::Left)
        return sf::Vector2i(currentTile.x - 1, currentTile.y);

    return sf::Vector2i(currentTile.x + 1, currentTile.y);
}

bool isTileRecentlyVisited(const Enemy& enemy, sf::Vector2i tile)
{
    for (const auto& recentTile : enemy.recentTiles)
    {
        if (recentTile == tile)
            return true;
    }

    return false;
}

void rememberEnemyTile(Enemy& enemy)
{
    sf::Vector2i currentTile = getTileFromPixel(enemy.x, enemy.y);

    if (enemy.recentTiles.empty() || enemy.recentTiles.back() != currentTile)
    {
        enemy.recentTiles.push_back(currentTile);
    }

    if (enemy.recentTiles.size() > 6)
    {
        enemy.recentTiles.erase(enemy.recentTiles.begin());
    }
}

std::vector<Direction> getValidEnemyDirections(const Enemy& enemy, bool avoidDeadEnds)
{
    std::vector<Direction> validDirections;

    std::vector<Direction> allDirections =
    {
        Direction::Up,
        Direction::Down,
        Direction::Left,
        Direction::Right
    };

    for (Direction direction : allDirections)
    {
        sf::Vector2i targetTile = getTargetTileForDirection(enemy, direction);

        int targetCol = targetTile.x;
        int targetRow = targetTile.y;

        if (!isWalkableTile(targetRow, targetCol))
            continue;

        if (avoidDeadEnds && isDeadEndTile(targetRow, targetCol))
            continue;

        sf::Vector2f movement = getDirectionVector(direction);

        float testX = enemy.x + movement.x * 6.f;
        float testY = enemy.y + movement.y * 6.f;

        if (canMoveToPixel(testX, testY))
        {
            validDirections.push_back(direction);
        }
    }

    return validDirections;
}

Direction chooseEnemyDirection(Enemy& enemy, std::mt19937& rng)
{
    sf::Vector2i currentTile = getTileFromPixel(enemy.x, enemy.y);

    int currentCol = currentTile.x;
    int currentRow = currentTile.y;

    // Eger dusman cikmaz sokagin icindeyse direkt tek cikisa yonel.
    if (isDeadEndTile(currentRow, currentCol))
    {
        std::vector<Direction> escapeDirections =
        {
            Direction::Up,
            Direction::Down,
            Direction::Left,
            Direction::Right
        };

        for (Direction direction : escapeDirections)
        {
            sf::Vector2i targetTile = getTargetTileForDirection(enemy, direction);

            int targetCol = targetTile.x;
            int targetRow = targetTile.y;

            if (isWalkableTile(targetRow, targetCol))
            {
                return direction;
            }
        }
    }

    // Normal durumda once cikmaz sokaklardan kacin.
    std::vector<Direction> validDirections = getValidEnemyDirections(enemy, true);

    // Eger cikmaz sokaklardan kacininca secenek kalmadiysa mecburen normal seceneklere bak.
    if (validDirections.empty())
    {
        validDirections = getValidEnemyDirections(enemy, false);
    }

    if (validDirections.empty())
    {
        return getOppositeDirection(enemy.direction);
    }

    Direction opposite = getOppositeDirection(enemy.direction);

    std::vector<Direction> filteredDirections;

    for (Direction direction : validDirections)
    {
        if (direction != opposite || validDirections.size() == 1)
        {
            filteredDirections.push_back(direction);
        }
    }

    std::vector<Direction> freshDirections;

    for (Direction direction : filteredDirections)
    {
        sf::Vector2i targetTile = getTargetTileForDirection(enemy, direction);

        if (!isTileRecentlyVisited(enemy, targetTile))
        {
            freshDirections.push_back(direction);
        }
    }

    if (!freshDirections.empty())
    {
        std::shuffle(freshDirections.begin(), freshDirections.end(), rng);
        return freshDirections.front();
    }

    if (!filteredDirections.empty())
    {
        std::shuffle(filteredDirections.begin(), filteredDirections.end(), rng);
        return filteredDirections.front();
    }

    std::shuffle(validDirections.begin(), validDirections.end(), rng);
    return validDirections.front();
}

void updateEnemy(Enemy& enemy, float deltaTime, std::mt19937& rng)
{
    rememberEnemyTile(enemy);

    enemy.decisionTimer -= deltaTime;

    if (enemy.decisionTimer <= 0.f)
    {
        enemy.direction = chooseEnemyDirection(enemy, rng);

        std::uniform_real_distribution<float> timeDistribution(0.8f, 2.0f);
        enemy.decisionTimer = timeDistribution(rng);
    }

    sf::Vector2f movement = getDirectionVector(enemy.direction);

    float newX = enemy.x + movement.x * enemy.speed * deltaTime;
    float newY = enemy.y + movement.y * enemy.speed * deltaTime;

    if (canMoveToPixel(newX, newY))
    {
        enemy.x = newX;
        enemy.y = newY;
    }
    else
    {
        enemy.direction = chooseEnemyDirection(enemy, rng);
        enemy.decisionTimer = 0.25f;
    }

    float movedDistance =
        std::abs(enemy.x - enemy.lastPosition.x) +
        std::abs(enemy.y - enemy.lastPosition.y);

    if (movedDistance < 0.4f)
    {
        enemy.stuckTimer += deltaTime;
    }
    else
    {
        enemy.stuckTimer = 0.f;
        enemy.lastPosition = sf::Vector2f(enemy.x, enemy.y);
    }

    if (enemy.stuckTimer > 0.6f)
    {
        enemy.direction = chooseEnemyDirection(enemy, rng);
        enemy.decisionTimer = 0.1f;
        enemy.stuckTimer = 0.f;
    }
}

Enemy createEnemy(int row, int col, Direction startDirection)
{
    Enemy enemy;

    enemy.x = col * TILE_SIZE;
    enemy.y = row * TILE_SIZE;
    enemy.speed = 70.f;
    enemy.direction = startDirection;
    enemy.decisionTimer = 1.f;
    enemy.stuckTimer = 0.f;
    enemy.lastPosition = sf::Vector2f(enemy.x, enemy.y);

    return enemy;
}

void drawFloor(sf::RenderWindow& window, int row, int col)
{
    float x = static_cast<float>(col * TILE_SIZE);
    float y = static_cast<float>(row * TILE_SIZE);

    sf::RectangleShape floorTile;
    floorTile.setSize(sf::Vector2f(TILE_SIZE - 1.f, TILE_SIZE - 1.f));
    floorTile.setPosition(sf::Vector2f(x, y));

    if ((row + col) % 2 == 0)
        floorTile.setFillColor(sf::Color(24, 23, 28));
    else
        floorTile.setFillColor(sf::Color(18, 18, 23));

    floorTile.setOutlineThickness(1.f);
    floorTile.setOutlineColor(sf::Color(10, 10, 14));
    window.draw(floorTile);

    sf::RectangleShape crack;
    crack.setSize(sf::Vector2f(TILE_SIZE / 3.f, 1.f));
    crack.setPosition(sf::Vector2f(x + 8.f, y + TILE_SIZE - 10.f));
    crack.setFillColor(sf::Color(45, 43, 50));
    window.draw(crack);
}

void drawSolidBlock(sf::RenderWindow& window, int row, int col)
{
    float x = static_cast<float>(col * TILE_SIZE);
    float y = static_cast<float>(row * TILE_SIZE);

    sf::RectangleShape block;
    block.setSize(sf::Vector2f(TILE_SIZE - 4.f, TILE_SIZE - 4.f));
    block.setPosition(sf::Vector2f(x + 2.f, y + 2.f));
    block.setFillColor(sf::Color(45, 42, 52));
    block.setOutlineThickness(2.f);
    block.setOutlineColor(sf::Color(12, 12, 18));
    window.draw(block);

    sf::RectangleShape topLight;
    topLight.setSize(sf::Vector2f(TILE_SIZE - 10.f, 4.f));
    topLight.setPosition(sf::Vector2f(x + 5.f, y + 6.f));
    topLight.setFillColor(sf::Color(75, 70, 85));
    window.draw(topLight);

    sf::RectangleShape sideShadow;
    sideShadow.setSize(sf::Vector2f(5.f, TILE_SIZE - 10.f));
    sideShadow.setPosition(sf::Vector2f(x + TILE_SIZE - 10.f, y + 5.f));
    sideShadow.setFillColor(sf::Color(25, 23, 32));
    window.draw(sideShadow);

    sf::RectangleShape bottomShadow;
    bottomShadow.setSize(sf::Vector2f(TILE_SIZE - 10.f, 5.f));
    bottomShadow.setPosition(sf::Vector2f(x + 5.f, y + TILE_SIZE - 10.f));
    bottomShadow.setFillColor(sf::Color(20, 19, 26));
    window.draw(bottomShadow);
}

void drawBreakableBlock(sf::RenderWindow& window, int row, int col)
{
    float x = static_cast<float>(col * TILE_SIZE);
    float y = static_cast<float>(row * TILE_SIZE);

    sf::RectangleShape crate;
    crate.setSize(sf::Vector2f(TILE_SIZE - 6.f, TILE_SIZE - 6.f));
    crate.setPosition(sf::Vector2f(x + 3.f, y + 3.f));
    crate.setFillColor(sf::Color(105, 62, 32));
    crate.setOutlineThickness(2.f);
    crate.setOutlineColor(sf::Color(42, 24, 13));
    window.draw(crate);

    sf::RectangleShape topPlank;
    topPlank.setSize(sf::Vector2f(TILE_SIZE - 12.f, 5.f));
    topPlank.setPosition(sf::Vector2f(x + 6.f, y + 8.f));
    topPlank.setFillColor(sf::Color(145, 88, 45));
    window.draw(topPlank);

    sf::RectangleShape bottomPlank;
    bottomPlank.setSize(sf::Vector2f(TILE_SIZE - 12.f, 5.f));
    bottomPlank.setPosition(sf::Vector2f(x + 6.f, y + TILE_SIZE - 13.f));
    bottomPlank.setFillColor(sf::Color(72, 40, 20));
    window.draw(bottomPlank);

    sf::RectangleShape leftPlank;
    leftPlank.setSize(sf::Vector2f(5.f, TILE_SIZE - 12.f));
    leftPlank.setPosition(sf::Vector2f(x + 8.f, y + 6.f));
    leftPlank.setFillColor(sf::Color(82, 45, 22));
    window.draw(leftPlank);

    sf::RectangleShape rightPlank;
    rightPlank.setSize(sf::Vector2f(5.f, TILE_SIZE - 12.f));
    rightPlank.setPosition(sf::Vector2f(x + TILE_SIZE - 13.f, y + 6.f));
    rightPlank.setFillColor(sf::Color(82, 45, 22));
    window.draw(rightPlank);

    sf::RectangleShape middleLine;
    middleLine.setSize(sf::Vector2f(TILE_SIZE - 14.f, 3.f));
    middleLine.setPosition(sf::Vector2f(x + 7.f, y + TILE_SIZE / 2.f));
    middleLine.setFillColor(sf::Color(50, 28, 14));
    window.draw(middleLine);
}

void drawTileMap(sf::RenderWindow& window)
{
    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLS; col++)
        {
            drawFloor(window, row, col);

            if (level1[row][col] == 1)
                drawSolidBlock(window, row, col);
            else if (level1[row][col] == 2)
                drawBreakableBlock(window, row, col);
        }
    }
}

bool canMoveToPixel(float x, float y)
{
    // A smaller collision box makes movement near walls feel smoother.
    const float margin = 7.f;

    int leftCol = static_cast<int>((x + margin) / TILE_SIZE);
    int rightCol = static_cast<int>((x + TILE_SIZE - margin) / TILE_SIZE);
    int topRow = static_cast<int>((y + margin) / TILE_SIZE);
    int bottomRow = static_cast<int>((y + TILE_SIZE - margin) / TILE_SIZE);

    if (topRow < 0 || bottomRow >= ROWS || leftCol < 0 || rightCol >= COLS)
        return false;

    if (level1[topRow][leftCol] != 0)
        return false;

    if (level1[topRow][rightCol] != 0)
        return false;

    if (level1[bottomRow][leftCol] != 0)
        return false;

    if (level1[bottomRow][rightCol] != 0)
        return false;

    return true;
}

void drawKnight(sf::RenderWindow& window, float x, float y, float size)
{
    sf::ConvexShape cape;
    cape.setPointCount(3);
    cape.setPoint(0, sf::Vector2f(x + size * 0.50f, y + size * 0.34f));
    cape.setPoint(1, sf::Vector2f(x + size * 0.18f, y + size * 0.96f));
    cape.setPoint(2, sf::Vector2f(x + size * 0.82f, y + size * 0.96f));
    cape.setFillColor(sf::Color(90, 18, 22));
    cape.setOutlineThickness(1.5f);
    cape.setOutlineColor(sf::Color(38, 8, 12));
    window.draw(cape);

    sf::RectangleShape body;
    body.setSize(sf::Vector2f(size * 0.42f, size * 0.38f));
    body.setPosition(sf::Vector2f(x + size * 0.29f, y + size * 0.42f));
    body.setFillColor(sf::Color(130, 130, 142));
    body.setOutlineThickness(2.f);
    body.setOutlineColor(sf::Color(55, 55, 65));
    window.draw(body);

    sf::RectangleShape chestLight;
    chestLight.setSize(sf::Vector2f(size * 0.26f, size * 0.05f));
    chestLight.setPosition(sf::Vector2f(x + size * 0.37f, y + size * 0.49f));
    chestLight.setFillColor(sf::Color(185, 185, 195));
    window.draw(chestLight);

    sf::RectangleShape leftLeg;
    leftLeg.setSize(sf::Vector2f(size * 0.10f, size * 0.20f));
    leftLeg.setPosition(sf::Vector2f(x + size * 0.35f, y + size * 0.75f));
    leftLeg.setFillColor(sf::Color(95, 95, 108));
    window.draw(leftLeg);

    sf::RectangleShape rightLeg;
    rightLeg.setSize(sf::Vector2f(size * 0.10f, size * 0.20f));
    rightLeg.setPosition(sf::Vector2f(x + size * 0.55f, y + size * 0.75f));
    rightLeg.setFillColor(sf::Color(95, 95, 108));
    window.draw(rightLeg);

    sf::RectangleShape leftBoot;
    leftBoot.setSize(sf::Vector2f(size * 0.15f, size * 0.06f));
    leftBoot.setPosition(sf::Vector2f(x + size * 0.31f, y + size * 0.92f));
    leftBoot.setFillColor(sf::Color(42, 36, 35));
    window.draw(leftBoot);

    sf::RectangleShape rightBoot;
    rightBoot.setSize(sf::Vector2f(size * 0.15f, size * 0.06f));
    rightBoot.setPosition(sf::Vector2f(x + size * 0.54f, y + size * 0.92f));
    rightBoot.setFillColor(sf::Color(42, 36, 35));
    window.draw(rightBoot);

    sf::RectangleShape leftArm;
    leftArm.setSize(sf::Vector2f(size * 0.10f, size * 0.22f));
    leftArm.setPosition(sf::Vector2f(x + size * 0.21f, y + size * 0.47f));
    leftArm.setFillColor(sf::Color(110, 110, 122));
    window.draw(leftArm);

    sf::RectangleShape rightArm;
    rightArm.setSize(sf::Vector2f(size * 0.10f, size * 0.22f));
    rightArm.setPosition(sf::Vector2f(x + size * 0.70f, y + size * 0.46f));
    rightArm.setFillColor(sf::Color(110, 110, 122));
    window.draw(rightArm);

    sf::CircleShape helmet(size * 0.17f);
    helmet.setPosition(sf::Vector2f(x + size * 0.33f, y + size * 0.14f));
    helmet.setFillColor(sf::Color(160, 160, 172));
    helmet.setOutlineThickness(2.f);
    helmet.setOutlineColor(sf::Color(52, 52, 62));
    window.draw(helmet);

    sf::RectangleShape visor;
    visor.setSize(sf::Vector2f(size * 0.20f, size * 0.075f));
    visor.setPosition(sf::Vector2f(x + size * 0.40f, y + size * 0.245f));
    visor.setFillColor(sf::Color(28, 28, 34));
    window.draw(visor);

    sf::RectangleShape crest;
    crest.setSize(sf::Vector2f(size * 0.08f, size * 0.11f));
    crest.setPosition(sf::Vector2f(x + size * 0.46f, y + size * 0.065f));
    crest.setFillColor(sf::Color(150, 25, 28));
    window.draw(crest);

    sf::ConvexShape shield;
    shield.setPointCount(5);
    shield.setPoint(0, sf::Vector2f(x + size * 0.12f, y + size * 0.45f));
    shield.setPoint(1, sf::Vector2f(x + size * 0.25f, y + size * 0.39f));
    shield.setPoint(2, sf::Vector2f(x + size * 0.30f, y + size * 0.55f));
    shield.setPoint(3, sf::Vector2f(x + size * 0.20f, y + size * 0.74f));
    shield.setPoint(4, sf::Vector2f(x + size * 0.09f, y + size * 0.59f));
    shield.setFillColor(sf::Color(115, 18, 22));
    shield.setOutlineThickness(2.f);
    shield.setOutlineColor(sf::Color(205, 178, 72));
    window.draw(shield);

    sf::RectangleShape shieldLine;
    shieldLine.setSize(sf::Vector2f(size * 0.035f, size * 0.26f));
    shieldLine.setPosition(sf::Vector2f(x + size * 0.185f, y + size * 0.46f));
    shieldLine.setFillColor(sf::Color(205, 178, 72));
    window.draw(shieldLine);

    sf::RectangleShape sword;
    sword.setSize(sf::Vector2f(size * 0.075f, size * 0.31f));
    sword.setPosition(sf::Vector2f(x + size * 0.78f, y + size * 0.34f));
    sword.setFillColor(sf::Color(190, 190, 205));
    sword.setOutlineThickness(1.f);
    sword.setOutlineColor(sf::Color(70, 70, 85));
    sword.setRotation(sf::degrees(20.f));
    window.draw(sword);

    sf::RectangleShape swordHandle;
    swordHandle.setSize(sf::Vector2f(size * 0.14f, size * 0.055f));
    swordHandle.setPosition(sf::Vector2f(x + size * 0.735f, y + size * 0.56f));
    swordHandle.setFillColor(sf::Color(95, 62, 22));
    swordHandle.setRotation(sf::degrees(20.f));
    window.draw(swordHandle);
}

void drawGoblin(sf::RenderWindow& window, float x, float y, float size)
{
    sf::ConvexShape leftEar;
    leftEar.setPointCount(3);
    leftEar.setPoint(0, sf::Vector2f(x + size * 0.27f, y + size * 0.22f));
    leftEar.setPoint(1, sf::Vector2f(x + size * 0.08f, y + size * 0.11f));
    leftEar.setPoint(2, sf::Vector2f(x + size * 0.22f, y + size * 0.38f));
    leftEar.setFillColor(sf::Color(62, 103, 38));
    leftEar.setOutlineThickness(1.f);
    leftEar.setOutlineColor(sf::Color(24, 45, 18));
    window.draw(leftEar);

    sf::ConvexShape rightEar;
    rightEar.setPointCount(3);
    rightEar.setPoint(0, sf::Vector2f(x + size * 0.73f, y + size * 0.22f));
    rightEar.setPoint(1, sf::Vector2f(x + size * 0.92f, y + size * 0.11f));
    rightEar.setPoint(2, sf::Vector2f(x + size * 0.78f, y + size * 0.38f));
    rightEar.setFillColor(sf::Color(62, 103, 38));
    rightEar.setOutlineThickness(1.f);
    rightEar.setOutlineColor(sf::Color(24, 45, 18));
    window.draw(rightEar);

    sf::CircleShape head(size * 0.22f);
    head.setPosition(sf::Vector2f(x + size * 0.28f, y + size * 0.12f));
    head.setFillColor(sf::Color(78, 118, 42));
    head.setOutlineThickness(2.f);
    head.setOutlineColor(sf::Color(28, 55, 18));
    window.draw(head);

    sf::CircleShape headShadow(size * 0.13f);
    headShadow.setPosition(sf::Vector2f(x + size * 0.35f, y + size * 0.16f));
    headShadow.setFillColor(sf::Color(98, 140, 55));
    window.draw(headShadow);

    sf::CircleShape leftEye(size * 0.045f);
    leftEye.setPosition(sf::Vector2f(x + size * 0.37f, y + size * 0.255f));
    leftEye.setFillColor(sf::Color(220, 20, 20));
    window.draw(leftEye);

    sf::CircleShape rightEye(size * 0.045f);
    rightEye.setPosition(sf::Vector2f(x + size * 0.535f, y + size * 0.255f));
    rightEye.setFillColor(sf::Color(220, 20, 20));
    window.draw(rightEye);

    sf::RectangleShape leftBrow;
    leftBrow.setSize(sf::Vector2f(size * 0.12f, size * 0.025f));
    leftBrow.setPosition(sf::Vector2f(x + size * 0.34f, y + size * 0.235f));
    leftBrow.setFillColor(sf::Color(12, 14, 10));
    leftBrow.setRotation(sf::degrees(-20.f));
    window.draw(leftBrow);

    sf::RectangleShape rightBrow;
    rightBrow.setSize(sf::Vector2f(size * 0.12f, size * 0.025f));
    rightBrow.setPosition(sf::Vector2f(x + size * 0.53f, y + size * 0.235f));
    rightBrow.setFillColor(sf::Color(12, 14, 10));
    rightBrow.setRotation(sf::degrees(20.f));
    window.draw(rightBrow);

    sf::RectangleShape mouth;
    mouth.setSize(sf::Vector2f(size * 0.18f, size * 0.055f));
    mouth.setPosition(sf::Vector2f(x + size * 0.41f, y + size * 0.40f));
    mouth.setFillColor(sf::Color(52, 0, 0));
    window.draw(mouth);

    sf::ConvexShape tooth1;
    tooth1.setPointCount(3);
    tooth1.setPoint(0, sf::Vector2f(x + size * 0.44f, y + size * 0.445f));
    tooth1.setPoint(1, sf::Vector2f(x + size * 0.48f, y + size * 0.515f));
    tooth1.setPoint(2, sf::Vector2f(x + size * 0.515f, y + size * 0.445f));
    tooth1.setFillColor(sf::Color(235, 230, 200));
    window.draw(tooth1);

    sf::ConvexShape tooth2;
    tooth2.setPointCount(3);
    tooth2.setPoint(0, sf::Vector2f(x + size * 0.535f, y + size * 0.445f));
    tooth2.setPoint(1, sf::Vector2f(x + size * 0.575f, y + size * 0.515f));
    tooth2.setPoint(2, sf::Vector2f(x + size * 0.61f, y + size * 0.445f));
    tooth2.setFillColor(sf::Color(235, 230, 200));
    window.draw(tooth2);

    sf::RectangleShape body;
    body.setSize(sf::Vector2f(size * 0.34f, size * 0.28f));
    body.setPosition(sf::Vector2f(x + size * 0.33f, y + size * 0.53f));
    body.setFillColor(sf::Color(58, 86, 33));
    body.setOutlineThickness(2.f);
    body.setOutlineColor(sf::Color(25, 42, 15));
    window.draw(body);

    sf::RectangleShape leftArm;
    leftArm.setSize(sf::Vector2f(size * 0.10f, size * 0.22f));
    leftArm.setPosition(sf::Vector2f(x + size * 0.23f, y + size * 0.57f));
    leftArm.setFillColor(sf::Color(70, 108, 40));
    leftArm.setRotation(sf::degrees(22.f));
    window.draw(leftArm);

    sf::RectangleShape rightArm;
    rightArm.setSize(sf::Vector2f(size * 0.10f, size * 0.22f));
    rightArm.setPosition(sf::Vector2f(x + size * 0.69f, y + size * 0.57f));
    rightArm.setFillColor(sf::Color(70, 108, 40));
    rightArm.setRotation(sf::degrees(-22.f));
    window.draw(rightArm);

    sf::CircleShape leftClaw(size * 0.035f);
    leftClaw.setPosition(sf::Vector2f(x + size * 0.18f, y + size * 0.74f));
    leftClaw.setFillColor(sf::Color(220, 215, 180));
    window.draw(leftClaw);

    sf::CircleShape rightClaw(size * 0.035f);
    rightClaw.setPosition(sf::Vector2f(x + size * 0.77f, y + size * 0.74f));
    rightClaw.setFillColor(sf::Color(220, 215, 180));
    window.draw(rightClaw);

    sf::RectangleShape leftLeg;
    leftLeg.setSize(sf::Vector2f(size * 0.08f, size * 0.16f));
    leftLeg.setPosition(sf::Vector2f(x + size * 0.38f, y + size * 0.79f));
    leftLeg.setFillColor(sf::Color(48, 72, 28));
    window.draw(leftLeg);

    sf::RectangleShape rightLeg;
    rightLeg.setSize(sf::Vector2f(size * 0.08f, size * 0.16f));
    rightLeg.setPosition(sf::Vector2f(x + size * 0.54f, y + size * 0.79f));
    rightLeg.setFillColor(sf::Color(48, 72, 28));
    window.draw(rightLeg);
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({COLS * TILE_SIZE, ROWS * TILE_SIZE}),
        "Bomberman Dungeon Arena - Level 1"
    );

    window.setFramerateLimit(60);

    float playerX = static_cast<float>((COLS / 2) * TILE_SIZE);
    float playerY = static_cast<float>((ROWS / 2) * TILE_SIZE);
    float playerSpeed = 180.f;

    sf::Clock deltaClock;

 std::mt19937 rng(std::random_device{}());

std::vector<Enemy> enemies =
{
    createEnemy(1, 1, Direction::Right),
    createEnemy(1, COLS - 2, Direction::Left),
    createEnemy(ROWS - 2, 1, Direction::Right),
    createEnemy(ROWS - 2, COLS - 2, Direction::Left)
};

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
        {
            window.close();
        }

        float deltaTime = deltaClock.restart().asSeconds();

        float moveX = 0.f;
        float moveY = 0.f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        {
            moveY -= 1.f;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        {
            moveY += 1.f;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        {
            moveX -= 1.f;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        {
            moveX += 1.f;
        }

        float length = std::sqrt(moveX * moveX + moveY * moveY);
        if (length != 0.f)
        {
            moveX /= length;
            moveY /= length;
        }

        float newX = playerX + moveX * playerSpeed * deltaTime;
        float newY = playerY + moveY * playerSpeed * deltaTime;

        if (canMoveToPixel(newX, playerY))
        {
            playerX = newX;
        }

        if (canMoveToPixel(playerX, newY))
        {
            playerY = newY;
        }

        for (Enemy& enemy : enemies)
{
    updateEnemy(enemy, deltaTime, rng);
}

        window.clear(sf::Color(6, 6, 10));

        drawTileMap(window);

        drawKnight(window, playerX, playerY, static_cast<float>(TILE_SIZE));

        for (const Enemy& enemy : enemies)
{
        drawGoblin(window, enemy.x, enemy.y, static_cast<float>(TILE_SIZE));
}
        window.display();
    }

    return 0;
}
