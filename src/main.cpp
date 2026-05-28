#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <cmath>
#include <random>
#include <algorithm>
#include <cstdint>

const int TILE_SIZE = 36;
const int ROWS = 21;
const int COLS = 25;
const int MAX_PLAYER_LIVES = 3;
const int INITIAL_PLAYER_BOMB_CAPACITY = 1;
const int MAX_PLAYER_BOMB_CAPACITY = 2;
const float BOMB_TIMER = 2.0f;
const float EXPLOSION_DURATION = 0.5f;
const int BOMB_RANGE = 2;
const float PLAYER_INVULNERABILITY_TIME = 1.0f;
const float ENTITY_COLLISION_MARGIN = 8.0f;

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

int originalLevel1[ROWS][COLS];

void copyLevelMap(const int source[ROWS][COLS], int target[ROWS][COLS])
{
    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLS; col++)
        {
            target[row][col] = source[row][col];
        }
    }
}

enum class Direction
{
    Up,
    Down,
    Left,
    Right
};
enum class GameState
{
    Playing,
    GameOver,
    LevelComplete
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

struct Bomb
{
    int row;
    int col;
    float timer;
    bool playerCanPass;
};

struct Explosion
{
    std::vector<sf::Vector2i> tiles;
    float timer;
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

void resetLevel(
    float& playerX,
    float& playerY,
    int& playerLives,
    int& playerBombCapacity,
    float& playerInvulnerabilityTimer,
    std::vector<Enemy>& enemies,
    std::vector<Bomb>& bombs,
    std::vector<Explosion>& explosions
)
{
    copyLevelMap(originalLevel1, level1);

    playerX = static_cast<float>((COLS / 2) * TILE_SIZE);
    playerY = static_cast<float>((ROWS / 2) * TILE_SIZE);

    playerLives = MAX_PLAYER_LIVES;
    playerBombCapacity = INITIAL_PLAYER_BOMB_CAPACITY;
    playerInvulnerabilityTimer = 0.0f;

    enemies.clear();

    enemies.push_back(createEnemy(1, 1, Direction::Right));
    enemies.push_back(createEnemy(1, COLS - 2, Direction::Left));
    enemies.push_back(createEnemy(ROWS - 2, 1, Direction::Right));
    enemies.push_back(createEnemy(ROWS - 2, COLS - 2, Direction::Left));

    bombs.clear();
    explosions.clear();
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

bool hasBombAt(const std::vector<Bomb>& bombs, int row, int col)
{
    for (const Bomb& bomb : bombs)
    {
        if (bomb.row == row && bomb.col == col)
        {
            return true;
        }
    }

    return false;
}

bool entityOverlapsTile(float x, float y, int row, int col)
{
    const float margin = 7.f;

    float entityLeft = x + margin;
    float entityRight = x + TILE_SIZE - margin;
    float entityTop = y + margin;
    float entityBottom = y + TILE_SIZE - margin;

    float tileLeft = static_cast<float>(col * TILE_SIZE);
    float tileRight = tileLeft + TILE_SIZE;
    float tileTop = static_cast<float>(row * TILE_SIZE);
    float tileBottom = tileTop + TILE_SIZE;

    bool overlapX = entityLeft < tileRight && entityRight > tileLeft;
    bool overlapY = entityTop < tileBottom && entityBottom > tileTop;

    return overlapX && overlapY;
}

bool canMoveToPixelWithBombs(float x, float y, const std::vector<Bomb>& bombs)
{
    if (!canMoveToPixel(x, y))
    {
        return false;
    }

    const float margin = 7.f;

    int leftCol = static_cast<int>((x + margin) / TILE_SIZE);
    int rightCol = static_cast<int>((x + TILE_SIZE - margin) / TILE_SIZE);
    int topRow = static_cast<int>((y + margin) / TILE_SIZE);
    int bottomRow = static_cast<int>((y + TILE_SIZE - margin) / TILE_SIZE);

    for (const Bomb& bomb : bombs)
    {
        if (bomb.playerCanPass)
        {
            continue;
        }

        bool touchesBombTile =
            (topRow == bomb.row && leftCol == bomb.col) ||
            (topRow == bomb.row && rightCol == bomb.col) ||
            (bottomRow == bomb.row && leftCol == bomb.col) ||
            (bottomRow == bomb.row && rightCol == bomb.col);

        if (touchesBombTile)
        {
            return false;
        }
    }

    return true;
}

void updateBombPassState(std::vector<Bomb>& bombs, float playerX, float playerY)
{
    for (Bomb& bomb : bombs)
    {
        if (bomb.playerCanPass)
        {
            if (!entityOverlapsTile(playerX, playerY, bomb.row, bomb.col))
            {
                bomb.playerCanPass = false;
            }
        }
    }
}

void placeBomb(
    std::vector<Bomb>& bombs,
    float playerX,
    float playerY,
    int playerBombCapacity
)
{
    if (static_cast<int>(bombs.size()) >= playerBombCapacity)
    {
        return;
    }

    sf::Vector2i playerTile = getTileFromPixel(playerX, playerY);

    int col = playerTile.x;
    int row = playerTile.y;

    if (level1[row][col] != 0)
    {
        return;
    }

    if (hasBombAt(bombs, row, col))
    {
        return;
    }

    Bomb bomb;
    bomb.row = row;
    bomb.col = col;
    bomb.timer = BOMB_TIMER;
    bomb.playerCanPass = true;

    bombs.push_back(bomb);
}

std::vector<sf::Vector2i> createExplosionTiles(int bombRow, int bombCol)
{
    std::vector<sf::Vector2i> tiles;

    // Center tile
    tiles.push_back(sf::Vector2i(bombCol, bombRow));

    std::vector<sf::Vector2i> directions =
    {
        sf::Vector2i(0, -1), // Up
        sf::Vector2i(0, 1),  // Down
        sf::Vector2i(-1, 0), // Left
        sf::Vector2i(1, 0)   // Right
    };

    for (const sf::Vector2i& direction : directions)
    {
        for (int distance = 1; distance <= BOMB_RANGE; distance++)
        {
            int currentCol = bombCol + direction.x * distance;
            int currentRow = bombRow + direction.y * distance;

            if (currentRow < 0 || currentRow >= ROWS || currentCol < 0 || currentCol >= COLS)
            {
                break;
            }

            if (level1[currentRow][currentCol] == 1)
            {
                break;
            }

            tiles.push_back(sf::Vector2i(currentCol, currentRow));

            if (level1[currentRow][currentCol] == 2)
            {
                break;
            }
        }
    }

    return tiles;
}

void destroyBreakableBlocksInExplosion(const std::vector<sf::Vector2i>& explosionTiles)
{
    for (const sf::Vector2i& tile : explosionTiles)
    {
        int col = tile.x;
        int row = tile.y;

        if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        {
            continue;
        }

        if (level1[row][col] == 2)
        {
            level1[row][col] = 0;
        }
    }
}

void createExplosion(std::vector<Explosion>& explosions, int row, int col)
{
    Explosion explosion;
    explosion.tiles = createExplosionTiles(row, col);
    explosion.timer = EXPLOSION_DURATION;

    destroyBreakableBlocksInExplosion(explosion.tiles);

    explosions.push_back(explosion);
}

void updateBombTimers(
    std::vector<Bomb>& bombs,
    std::vector<Explosion>& explosions,
    float deltaTime
)
{
    for (int i = static_cast<int>(bombs.size()) - 1; i >= 0; i--)
    {
        bombs[i].timer -= deltaTime;

        if (bombs[i].timer <= 0.0f)
        {
            createExplosion(explosions, bombs[i].row, bombs[i].col);
            bombs.erase(bombs.begin() + i);
        }
    }
}

void updateExplosions(std::vector<Explosion>& explosions, float deltaTime)
{
    for (Explosion& explosion : explosions)
    {
        explosion.timer -= deltaTime;
    }

    explosions.erase(
        std::remove_if(
            explosions.begin(),
            explosions.end(),
            [](const Explosion& explosion)
            {
                return explosion.timer <= 0.0f;
            }
        ),
        explosions.end()
    );
}

bool isEnemyHitByExplosion(const Enemy& enemy, const std::vector<Explosion>& explosions)
{
    for (const Explosion& explosion : explosions)
    {
        for (const sf::Vector2i& tile : explosion.tiles)
        {
            int col = tile.x;
            int row = tile.y;

            if (entityOverlapsTile(enemy.x, enemy.y, row, col))
            {
                return true;
            }
        }
    }

    return false;
}

void removeEnemiesHitByExplosions(
    std::vector<Enemy>& enemies,
    const std::vector<Explosion>& explosions
)
{
    enemies.erase(
        std::remove_if(
            enemies.begin(),
            enemies.end(),
            [&explosions](const Enemy& enemy)
            {
                return isEnemyHitByExplosion(enemy, explosions);
            }
        ),
        enemies.end()
    );
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

void drawGothicHeart(sf::RenderWindow& window, float x, float y, bool full)
{
    sf::Color outlineColor(25, 12, 16);
    sf::Color shadowColor(8, 8, 12, 180);

    sf::Color heartColor;
    sf::Color highlightColor;

    if (full)
    {
        heartColor = sf::Color(165, 22, 38);
        highlightColor = sf::Color(235, 70, 85);
    }
    else
    {
        heartColor = sf::Color(45, 43, 50);
        highlightColor = sf::Color(75, 72, 82);
    }

    // Heart shadow
    sf::CircleShape shadowLeft(7.f);
    shadowLeft.setPosition(sf::Vector2f(x + 3.f, y + 4.f));
    shadowLeft.setFillColor(shadowColor);
    window.draw(shadowLeft);

    sf::CircleShape shadowRight(7.f);
    shadowRight.setPosition(sf::Vector2f(x + 13.f, y + 4.f));
    shadowRight.setFillColor(shadowColor);
    window.draw(shadowRight);

    sf::ConvexShape shadowBottom;
    shadowBottom.setPointCount(3);
    shadowBottom.setPoint(0, sf::Vector2f(x + 1.f, y + 12.f));
    shadowBottom.setPoint(1, sf::Vector2f(x + 29.f, y + 12.f));
    shadowBottom.setPoint(2, sf::Vector2f(x + 15.f, y + 31.f));
    shadowBottom.setFillColor(shadowColor);
    window.draw(shadowBottom);

    // Dark outer heart base
    sf::CircleShape outerLeft(8.f);
    outerLeft.setPosition(sf::Vector2f(x + 1.f, y + 1.f));
    outerLeft.setFillColor(outlineColor);
    window.draw(outerLeft);

    sf::CircleShape outerRight(8.f);
    outerRight.setPosition(sf::Vector2f(x + 12.f, y + 1.f));
    outerRight.setFillColor(outlineColor);
    window.draw(outerRight);

    sf::ConvexShape outerBottom;
    outerBottom.setPointCount(3);
    outerBottom.setPoint(0, sf::Vector2f(x - 1.f, y + 10.f));
    outerBottom.setPoint(1, sf::Vector2f(x + 29.f, y + 10.f));
    outerBottom.setPoint(2, sf::Vector2f(x + 14.f, y + 31.f));
    outerBottom.setFillColor(outlineColor);
    window.draw(outerBottom);

    // Inner red / empty heart
    sf::CircleShape innerLeft(6.5f);
    innerLeft.setPosition(sf::Vector2f(x + 3.f, y + 3.f));
    innerLeft.setFillColor(heartColor);
    window.draw(innerLeft);

    sf::CircleShape innerRight(6.5f);
    innerRight.setPosition(sf::Vector2f(x + 13.5f, y + 3.f));
    innerRight.setFillColor(heartColor);
    window.draw(innerRight);

    sf::ConvexShape innerBottom;
    innerBottom.setPointCount(3);
    innerBottom.setPoint(0, sf::Vector2f(x + 2.f, y + 11.f));
    innerBottom.setPoint(1, sf::Vector2f(x + 26.f, y + 11.f));
    innerBottom.setPoint(2, sf::Vector2f(x + 14.f, y + 28.f));
    innerBottom.setFillColor(heartColor);
    window.draw(innerBottom);

    // Top shine
    sf::CircleShape shine(2.4f);
    shine.setPosition(sf::Vector2f(x + 7.f, y + 6.f));
    shine.setFillColor(highlightColor);
    window.draw(shine);

    // Small gothic cut in the middle
    sf::ConvexShape notch;
    notch.setPointCount(3);
    notch.setPoint(0, sf::Vector2f(x + 11.f, y + 10.f));
    notch.setPoint(1, sf::Vector2f(x + 17.f, y + 10.f));
    notch.setPoint(2, sf::Vector2f(x + 14.f, y + 15.f));
    notch.setFillColor(outlineColor);
    window.draw(notch);

    // Empty heart crack detail
    if (!full)
    {
        sf::RectangleShape crack1;
        crack1.setSize(sf::Vector2f(2.f, 9.f));
        crack1.setPosition(sf::Vector2f(x + 14.f, y + 11.f));
        crack1.setFillColor(sf::Color(18, 18, 22));
        window.draw(crack1);

        sf::RectangleShape crack2;
        crack2.setSize(sf::Vector2f(2.f, 7.f));
        crack2.setPosition(sf::Vector2f(x + 17.f, y + 17.f));
        crack2.setFillColor(sf::Color(18, 18, 22));
        window.draw(crack2);
    }
}

void drawBomb(sf::RenderWindow& window, const Bomb& bomb)
{
    float x = static_cast<float>(bomb.col * TILE_SIZE);
    float y = static_cast<float>(bomb.row * TILE_SIZE);

    float timerRatio = bomb.timer / BOMB_TIMER;

    if (timerRatio < 0.0f)
        timerRatio = 0.0f;

    if (timerRatio > 1.0f)
        timerRatio = 1.0f;

    float pulse = 1.0f + (1.0f - timerRatio) * 0.12f;

    // Shadow
    sf::CircleShape shadow(TILE_SIZE * 0.28f);
    shadow.setPosition(sf::Vector2f(x + 8.f, y + 13.f));
    shadow.setFillColor(sf::Color(0, 0, 0, 130));
    window.draw(shadow);

    // Bomb body
    sf::CircleShape body(TILE_SIZE * 0.28f * pulse);
    body.setPosition(sf::Vector2f(
        x + 8.f - ((pulse - 1.0f) * 4.f),
        y + 8.f - ((pulse - 1.0f) * 4.f)
    ));
    body.setFillColor(sf::Color(17, 17, 22));
    body.setOutlineThickness(2.f);
    body.setOutlineColor(sf::Color(78, 72, 85));
    window.draw(body);

    // Red warning glow when close to exploding
    if (bomb.timer < 0.7f)
    {
        sf::CircleShape dangerGlow(TILE_SIZE * 0.12f);
        dangerGlow.setPosition(sf::Vector2f(x + 13.f, y + 13.f));
        dangerGlow.setFillColor(sf::Color(150, 20, 25, 130));
        window.draw(dangerGlow);
    }

    // Bomb shine
    sf::CircleShape shine(TILE_SIZE * 0.065f);
    shine.setPosition(sf::Vector2f(x + 15.f, y + 13.f));
    shine.setFillColor(sf::Color(100, 100, 112));
    window.draw(shine);

    // Fuse base
    sf::RectangleShape fuseBase;
    fuseBase.setSize(sf::Vector2f(6.f, 5.f));
    fuseBase.setPosition(sf::Vector2f(x + 22.f, y + 6.f));
    fuseBase.setFillColor(sf::Color(70, 55, 35));
    fuseBase.setRotation(sf::degrees(-20.f));
    window.draw(fuseBase);

    // Fuse
    sf::RectangleShape fuse;
    fuse.setSize(sf::Vector2f(14.f, 3.f));
    fuse.setPosition(sf::Vector2f(x + 25.f, y + 3.f));
    fuse.setFillColor(sf::Color(135, 90, 35));
    fuse.setRotation(sf::degrees(-25.f));
    window.draw(fuse);

    // Spark blinks faster near explosion
    int blinkSpeed = bomb.timer < 0.7f ? 18 : 8;
    int blinkFrame = static_cast<int>(bomb.timer * blinkSpeed);

    if (blinkFrame % 2 == 0)
    {
        sf::CircleShape spark(3.5f);
        spark.setPosition(sf::Vector2f(x + 34.f, y - 1.f));
        spark.setFillColor(sf::Color(240, 125, 25));
        window.draw(spark);

        sf::CircleShape smallSpark(2.f);
        smallSpark.setPosition(sf::Vector2f(x + 38.f, y + 2.f));
        smallSpark.setFillColor(sf::Color(255, 210, 70));
        window.draw(smallSpark);
    }
}

void drawBombs(sf::RenderWindow& window, const std::vector<Bomb>& bombs)
{
    for (const Bomb& bomb : bombs)
    {
        drawBomb(window, bomb);
    }
}

void drawExplosionTile(
    sf::RenderWindow& window,
    int row,
    int col,
    float timer,
    bool isCenter
)
{
    float x = static_cast<float>(col * TILE_SIZE);
    float y = static_cast<float>(row * TILE_SIZE);

    float ratio = timer / EXPLOSION_DURATION;

    if (ratio < 0.0f)
        ratio = 0.0f;

    if (ratio > 1.0f)
        ratio = 1.0f;

    std::uint8_t alpha = static_cast<std::uint8_t>(210 * ratio + 45);

    sf::RectangleShape outerFlame;
    outerFlame.setSize(sf::Vector2f(TILE_SIZE - 6.f, TILE_SIZE - 6.f));
    outerFlame.setPosition(sf::Vector2f(x + 3.f, y + 3.f));
    outerFlame.setFillColor(sf::Color(150, 30, 20, alpha));
    outerFlame.setOutlineThickness(1.f);
    outerFlame.setOutlineColor(sf::Color(245, 120, 35, alpha));
    window.draw(outerFlame);

    sf::RectangleShape innerFlame;
    innerFlame.setSize(sf::Vector2f(TILE_SIZE - 16.f, TILE_SIZE - 16.f));
    innerFlame.setPosition(sf::Vector2f(x + 8.f, y + 8.f));
    innerFlame.setFillColor(sf::Color(245, 145, 35, alpha));
    window.draw(innerFlame);

    sf::RectangleShape core;
    core.setSize(sf::Vector2f(TILE_SIZE - 24.f, TILE_SIZE - 24.f));
    core.setPosition(sf::Vector2f(x + 12.f, y + 12.f));
    core.setFillColor(sf::Color(255, 225, 100, alpha));
    window.draw(core);

    if (isCenter)
    {
        sf::CircleShape centerGlow(TILE_SIZE * 0.34f);
        centerGlow.setPosition(sf::Vector2f(x + 5.f, y + 5.f));
        centerGlow.setFillColor(sf::Color(255, 70, 35, static_cast<std::uint8_t>(130 * ratio)));
        window.draw(centerGlow);

        sf::CircleShape centerCore(TILE_SIZE * 0.20f);
        centerCore.setPosition(sf::Vector2f(x + 11.f, y + 11.f));
        centerCore.setFillColor(sf::Color(255, 230, 120, alpha));
        window.draw(centerCore);
    }
}

void drawExplosions(sf::RenderWindow& window, const std::vector<Explosion>& explosions)
{
    for (const Explosion& explosion : explosions)
    {
        for (int i = 0; i < static_cast<int>(explosion.tiles.size()); i++)
        {
            int col = explosion.tiles[i].x;
            int row = explosion.tiles[i].y;

            bool isCenter = i == 0;

            drawExplosionTile(window, row, col, explosion.timer, isCenter);
        }
    }
}

void drawHealthHUD(sf::RenderWindow& window, int playerLives)
{
    sf::RectangleShape panel;
    panel.setSize(sf::Vector2f(145.f, 42.f));
    panel.setPosition(sf::Vector2f(8.f, 8.f));
    panel.setFillColor(sf::Color(9, 9, 13, 230));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(95, 78, 48));
    window.draw(panel);

    sf::RectangleShape innerPanel;
    innerPanel.setSize(sf::Vector2f(135.f, 32.f));
    innerPanel.setPosition(sf::Vector2f(13.f, 13.f));
    innerPanel.setFillColor(sf::Color(18, 17, 23, 220));
    innerPanel.setOutlineThickness(1.f);
    innerPanel.setOutlineColor(sf::Color(45, 38, 30));
    window.draw(innerPanel);

    sf::RectangleShape topLine;
    topLine.setSize(sf::Vector2f(35.f, 2.f));
    topLine.setPosition(sf::Vector2f(18.f, 16.f));
    topLine.setFillColor(sf::Color(135, 105, 48));
    window.draw(topLine);

    sf::RectangleShape bottomLine;
    bottomLine.setSize(sf::Vector2f(35.f, 2.f));
    bottomLine.setPosition(sf::Vector2f(108.f, 41.f));
    bottomLine.setFillColor(sf::Color(135, 105, 48));
    window.draw(bottomLine);

    for (int i = 0; i < MAX_PLAYER_LIVES; i++)
    {
        bool full = i < playerLives;
        drawGothicHeart(window, 23.f + i * 38.f, 17.f, full);
    }
}

void drawBombCooldownHUD(
    sf::RenderWindow& window,
    const std::vector<Bomb>& bombs,
    int playerBombCapacity
)
{
    float screenWidth = static_cast<float>(COLS * TILE_SIZE);

    float panelX = screenWidth - 155.f;
    float panelY = 8.f;

    bool bombReady = static_cast<int>(bombs.size()) < playerBombCapacity;

    float readyRatio = 1.0f;

    if (!bombReady && !bombs.empty())
    {
        readyRatio = 1.0f - (bombs.front().timer / BOMB_TIMER);

        if (readyRatio < 0.0f)
            readyRatio = 0.0f;

        if (readyRatio > 1.0f)
            readyRatio = 1.0f;
    }

    sf::RectangleShape panel;
    panel.setSize(sf::Vector2f(145.f, 42.f));
    panel.setPosition(sf::Vector2f(panelX, panelY));
    panel.setFillColor(sf::Color(9, 9, 13, 230));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(95, 78, 48));
    window.draw(panel);

    sf::RectangleShape innerPanel;
    innerPanel.setSize(sf::Vector2f(135.f, 32.f));
    innerPanel.setPosition(sf::Vector2f(panelX + 5.f, panelY + 5.f));
    innerPanel.setFillColor(sf::Color(18, 17, 23, 220));
    innerPanel.setOutlineThickness(1.f);
    innerPanel.setOutlineColor(sf::Color(45, 38, 30));
    window.draw(innerPanel);

    sf::CircleShape bombShadow(11.f);
    bombShadow.setPosition(sf::Vector2f(panelX + 19.f, panelY + 14.f));
    bombShadow.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(bombShadow);

    sf::CircleShape bombIcon(10.f);
    bombIcon.setPosition(sf::Vector2f(panelX + 18.f, panelY + 12.f));

    if (bombReady)
        bombIcon.setFillColor(sf::Color(42, 85, 55));
    else
        bombIcon.setFillColor(sf::Color(82, 32, 32));

    bombIcon.setOutlineThickness(2.f);
    bombIcon.setOutlineColor(sf::Color(130, 105, 55));
    window.draw(bombIcon);

    sf::CircleShape shine(3.f);
    shine.setPosition(sf::Vector2f(panelX + 25.f, panelY + 16.f));

    if (bombReady)
        shine.setFillColor(sf::Color(135, 210, 130));
    else
        shine.setFillColor(sf::Color(210, 95, 70));

    window.draw(shine);

    sf::RectangleShape fuse;
    fuse.setSize(sf::Vector2f(12.f, 3.f));
    fuse.setPosition(sf::Vector2f(panelX + 35.f, panelY + 12.f));
    fuse.setFillColor(sf::Color(135, 90, 35));
    fuse.setRotation(sf::degrees(-25.f));
    window.draw(fuse);

    sf::CircleShape statusLight(5.f);
    statusLight.setPosition(sf::Vector2f(panelX + 53.f, panelY + 16.f));

    if (bombReady)
        statusLight.setFillColor(sf::Color(70, 190, 80));
    else
        statusLight.setFillColor(sf::Color(190, 45, 35));

    window.draw(statusLight);

    sf::RectangleShape barBack;
    barBack.setSize(sf::Vector2f(68.f, 8.f));
    barBack.setPosition(sf::Vector2f(panelX + 67.f, panelY + 17.f));
    barBack.setFillColor(sf::Color(35, 32, 38));
    barBack.setOutlineThickness(1.f);
    barBack.setOutlineColor(sf::Color(80, 68, 48));
    window.draw(barBack);

    sf::RectangleShape barFill;
    barFill.setSize(sf::Vector2f(68.f * readyRatio, 8.f));
    barFill.setPosition(sf::Vector2f(panelX + 67.f, panelY + 17.f));

    if (bombReady)
        barFill.setFillColor(sf::Color(75, 170, 80));
    else
        barFill.setFillColor(sf::Color(180, 65, 35));

    window.draw(barFill);

    sf::RectangleShape topLine;
    topLine.setSize(sf::Vector2f(28.f, 2.f));
    topLine.setPosition(sf::Vector2f(panelX + 102.f, panelY + 11.f));
    topLine.setFillColor(sf::Color(135, 105, 48));
    window.draw(topLine);

    sf::RectangleShape slot;
    slot.setSize(sf::Vector2f(11.f, 11.f));
    slot.setPosition(sf::Vector2f(panelX + 121.f, panelY + 27.f));
    slot.setOutlineThickness(1.f);
    slot.setOutlineColor(sf::Color(115, 92, 52));

    if (bombReady)
        slot.setFillColor(sf::Color(55, 95, 58));
    else
        slot.setFillColor(sf::Color(85, 38, 35));

    window.draw(slot);
}

bool areEntitiesTouching(float firstX, float firstY, float secondX, float secondY)
{
    float firstLeft = firstX + ENTITY_COLLISION_MARGIN;
    float firstRight = firstX + TILE_SIZE - ENTITY_COLLISION_MARGIN;
    float firstTop = firstY + ENTITY_COLLISION_MARGIN;
    float firstBottom = firstY + TILE_SIZE - ENTITY_COLLISION_MARGIN;

    float secondLeft = secondX + ENTITY_COLLISION_MARGIN;
    float secondRight = secondX + TILE_SIZE - ENTITY_COLLISION_MARGIN;
    float secondTop = secondY + ENTITY_COLLISION_MARGIN;
    float secondBottom = secondY + TILE_SIZE - ENTITY_COLLISION_MARGIN;

    bool overlapX = firstLeft < secondRight && firstRight > secondLeft;
    bool overlapY = firstTop < secondBottom && firstBottom > secondTop;

    return overlapX && overlapY;
}

void checkEnemyContactDamage(
    float playerX,
    float playerY,
    const std::vector<Enemy>& enemies,
    int& playerLives,
    float& playerInvulnerabilityTimer
)
{
    if (playerInvulnerabilityTimer > 0.0f)
        return;

    if (playerLives <= 0)
        return;

    for (const Enemy& enemy : enemies)
    {
        if (areEntitiesTouching(playerX, playerY, enemy.x, enemy.y))
        {
            playerLives--;

            if (playerLives < 0)
            {
                playerLives = 0;
            }

            playerInvulnerabilityTimer = PLAYER_INVULNERABILITY_TIME;
            break;
        }
    }
}

bool isPlayerHitByExplosion(
    float playerX,
    float playerY,
    const std::vector<Explosion>& explosions
)
{
    for (const Explosion& explosion : explosions)
    {
        for (const sf::Vector2i& tile : explosion.tiles)
        {
            int col = tile.x;
            int row = tile.y;

            if (entityOverlapsTile(playerX, playerY, row, col))
            {
                return true;
            }
        }
    }

    return false;
}

void checkPlayerExplosionDamage(
    float playerX,
    float playerY,
    const std::vector<Explosion>& explosions,
    int& playerLives,
    float& playerInvulnerabilityTimer
)
{
    if (playerInvulnerabilityTimer > 0.0f)
        return;

    if (playerLives <= 0)
        return;

    if (isPlayerHitByExplosion(playerX, playerY, explosions))
    {
        playerLives--;

        if (playerLives < 0)
        {
            playerLives = 0;
        }

        playerInvulnerabilityTimer = PLAYER_INVULNERABILITY_TIME;
    }
}

void drawGameOverScreen(sf::RenderWindow& window, const sf::Font& font)
{
    float screenWidth = static_cast<float>(COLS * TILE_SIZE);
    float screenHeight = static_cast<float>(ROWS * TILE_SIZE);

    sf::RectangleShape darkOverlay;
    darkOverlay.setSize(sf::Vector2f(screenWidth, screenHeight));
    darkOverlay.setPosition(sf::Vector2f(0.f, 0.f));
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 185));
    window.draw(darkOverlay);

    sf::RectangleShape panel;
    panel.setSize(sf::Vector2f(470.f, 260.f));
    panel.setPosition(sf::Vector2f(screenWidth / 2.f - 235.f, screenHeight / 2.f - 130.f));
    panel.setFillColor(sf::Color(14, 12, 18, 245));
    panel.setOutlineThickness(3.f);
    panel.setOutlineColor(sf::Color(120, 30, 35));
    window.draw(panel);

    sf::RectangleShape innerPanel;
    innerPanel.setSize(sf::Vector2f(440.f, 230.f));
    innerPanel.setPosition(sf::Vector2f(screenWidth / 2.f - 220.f, screenHeight / 2.f - 115.f));
    innerPanel.setFillColor(sf::Color(22, 19, 27, 235));
    innerPanel.setOutlineThickness(1.f);
    innerPanel.setOutlineColor(sf::Color(135, 105, 55));
    window.draw(innerPanel);

    // Decorative top line
    sf::RectangleShape topLine;
    topLine.setSize(sf::Vector2f(240.f, 3.f));
    topLine.setPosition(sf::Vector2f(screenWidth / 2.f - 120.f, screenHeight / 2.f - 85.f));
    topLine.setFillColor(sf::Color(150, 115, 60));
    window.draw(topLine);

    // Decorative bottom line
    sf::RectangleShape bottomLine;
    bottomLine.setSize(sf::Vector2f(240.f, 3.f));
    bottomLine.setPosition(sf::Vector2f(screenWidth / 2.f - 120.f, screenHeight / 2.f + 72.f));
    bottomLine.setFillColor(sf::Color(150, 115, 60));
    window.draw(bottomLine);

    sf::Text title(font);
    title.setString("GAME OVER");
    title.setCharacterSize(62);
    title.setFillColor(sf::Color(185, 35, 42));
    title.setOutlineThickness(3.f);
    title.setOutlineColor(sf::Color(55, 10, 12));
    title.setPosition(sf::Vector2f(screenWidth / 2.f - 175.f, screenHeight / 2.f - 65.f));
    window.draw(title);

    sf::Text subtitle(font);
    subtitle.setString("Press R to continue");
    subtitle.setCharacterSize(28);
    subtitle.setFillColor(sf::Color(215, 200, 165));
    subtitle.setOutlineThickness(1.5f);
    subtitle.setOutlineColor(sf::Color(45, 35, 28));
    subtitle.setPosition(sf::Vector2f(screenWidth / 2.f - 135.f, screenHeight / 2.f + 5.f));
    window.draw(subtitle);

    sf::Text exitText(font);
    exitText.setString("Press Esc to quit");
    exitText.setCharacterSize(22);
    exitText.setFillColor(sf::Color(155, 145, 125));
    exitText.setOutlineThickness(1.f);
    exitText.setOutlineColor(sf::Color(35, 30, 25));
    exitText.setPosition(sf::Vector2f(screenWidth / 2.f - 105.f, screenHeight / 2.f + 45.f));
    window.draw(exitText);
}

void drawLevelCompleteScreen(sf::RenderWindow& window, const sf::Font& font)
{
    float screenWidth = static_cast<float>(COLS * TILE_SIZE);
    float screenHeight = static_cast<float>(ROWS * TILE_SIZE);

    sf::RectangleShape overlay;
    overlay.setSize(sf::Vector2f(screenWidth, screenHeight));
    overlay.setFillColor(sf::Color(0, 0, 0, 165));
    window.draw(overlay);

    sf::Text title(font);
    title.setString("    VICTORY!  ");
    title.setCharacterSize(52);
    title.setFillColor(sf::Color(220, 190, 95));
    title.setOutlineThickness(3.f);
    title.setOutlineColor(sf::Color(45, 35, 12));
    title.setPosition(sf::Vector2f(screenWidth / 2.f - 190.f, screenHeight / 2.f - 70.f));
    window.draw(title);

    sf::Text subtitle(font);
    subtitle.setString("All goblins defeated");
    subtitle.setCharacterSize(26);
    subtitle.setFillColor(sf::Color(215, 205, 175));
    subtitle.setOutlineThickness(1.5f);
    subtitle.setOutlineColor(sf::Color(40, 35, 28));
    subtitle.setPosition(sf::Vector2f(screenWidth / 2.f - 135.f, screenHeight / 2.f + 0.f));
    window.draw(subtitle);

    sf::Text restartText(font);
    restartText.setString("Press R to restart");
    restartText.setCharacterSize(22);
    restartText.setFillColor(sf::Color(160, 150, 125));
    restartText.setOutlineThickness(1.f);
    restartText.setOutlineColor(sf::Color(30, 28, 24));
    restartText.setPosition(sf::Vector2f(screenWidth / 2.f - 105.f, screenHeight / 2.f + 42.f));
    window.draw(restartText);
}

int main()
{

    
    sf::RenderWindow window(
        sf::VideoMode({COLS * TILE_SIZE, ROWS * TILE_SIZE}),
        "Bomberman Dungeon Arena - Level 1"
    );

    window.setFramerateLimit(60);

    copyLevelMap(level1, originalLevel1);

 
    sf::Font gameFont;

if (!gameFont.openFromFile("C:/Windows/Fonts/arial.ttf"))
{
    return -1;
}



    float playerX = static_cast<float>((COLS / 2) * TILE_SIZE);
    float playerY = static_cast<float>((ROWS / 2) * TILE_SIZE);
    float playerSpeed = 180.f;

    int playerLives = MAX_PLAYER_LIVES;
    int playerBombCapacity = INITIAL_PLAYER_BOMB_CAPACITY;
    float playerInvulnerabilityTimer = 0.0f;
    GameState gameState = GameState::Playing;

    sf::Clock deltaClock;

 std::mt19937 rng(std::random_device{}());

std::vector<Enemy> enemies =
{
    createEnemy(1, 1, Direction::Right),
    createEnemy(1, COLS - 2, Direction::Left),
    createEnemy(ROWS - 2, 1, Direction::Right),
    createEnemy(ROWS - 2, COLS - 2, Direction::Left)
};

std::vector<Bomb> bombs;
std::vector<Explosion> explosions;
bool spaceWasPressed = false;

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

        if (gameState == GameState::GameOver)
{
    if (gameState == GameState::GameOver || gameState == GameState::LevelComplete)
{
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R))
    {
        resetLevel(
            playerX,
            playerY,
            playerLives,
            playerBombCapacity,
            playerInvulnerabilityTimer,
            enemies,
            bombs,
            explosions
        );

        gameState = GameState::Playing;
        deltaClock.restart();
    }
}
}

        float deltaTime = deltaClock.restart().asSeconds();
        if (gameState == GameState::Playing)
{
    bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);


    if (spacePressed && !spaceWasPressed)
{
    placeBomb(bombs, playerX, playerY, playerBombCapacity);
}

spaceWasPressed = spacePressed;

        if (playerInvulnerabilityTimer > 0.0f)
{
    playerInvulnerabilityTimer -= deltaTime;

    if (playerInvulnerabilityTimer < 0.0f)
    {
        playerInvulnerabilityTimer = 0.0f;
    }
}

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

        if (canMoveToPixelWithBombs(newX, playerY, bombs))
{
    playerX = newX;
}

if (canMoveToPixelWithBombs(playerX, newY, bombs))
{
    playerY = newY;
}


        for (Enemy& enemy : enemies)
    {
    updateEnemy(enemy, deltaTime, rng);
    }

    
updateBombPassState(bombs, playerX, playerY);
updateBombTimers(bombs, explosions, deltaTime);
updateExplosions(explosions, deltaTime);
removeEnemiesHitByExplosions(enemies, explosions);

checkPlayerExplosionDamage(
    playerX,
    playerY,
    explosions,
    playerLives,
    playerInvulnerabilityTimer
);

checkEnemyContactDamage(
    playerX,
    playerY,
    enemies,
    playerLives,
    playerInvulnerabilityTimer
);

if (playerLives <= 0)
{
    gameState = GameState::GameOver;
}
else if (enemies.empty() && bombs.empty() && explosions.empty())
{
    gameState = GameState::LevelComplete;
}

}

        window.clear(sf::Color(6, 6, 10));

        drawTileMap(window);

        drawExplosions(window, explosions);
        drawBombs(window, bombs);

        bool shouldDrawPlayer = true;

if (playerInvulnerabilityTimer > 0.0f)
{
    int blinkFrame = static_cast<int>(playerInvulnerabilityTimer * 12.0f);
    shouldDrawPlayer = blinkFrame % 2 == 0;
}

if (shouldDrawPlayer)
{
    drawKnight(window, playerX, playerY, static_cast<float>(TILE_SIZE));
}

        for (const Enemy& enemy : enemies)
{
        drawGoblin(window, enemy.x, enemy.y, static_cast<float>(TILE_SIZE));
}
        drawHealthHUD(window, playerLives);
        drawBombCooldownHUD(window, bombs, playerBombCapacity);

        if (gameState == GameState::GameOver)
{
    drawGameOverScreen(window, gameFont);
}

if (gameState == GameState::LevelComplete)
{
    drawLevelCompleteScreen(window, gameFont);
}

        window.display();

        
    }

    
    


    return 0;
}
