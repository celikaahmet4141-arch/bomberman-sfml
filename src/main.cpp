#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>

const int TILE_SIZE = 36;
const int ROWS = 21;
const int COLS = 25;

// 0 = Empty path
// 1 = Solid block
// 2 = Breakable block
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

void drawFloor(sf::RenderWindow& window, int row, int col)
{
    float x = col * TILE_SIZE;
    float y = row * TILE_SIZE;

    sf::RectangleShape floorTile;
    floorTile.setSize(sf::Vector2f(TILE_SIZE - 1, TILE_SIZE - 1));
    floorTile.setPosition(sf::Vector2f(x, y));

    if ((row + col) % 2 == 0)
        floorTile.setFillColor(sf::Color(24, 23, 28));
    else
        floorTile.setFillColor(sf::Color(18, 18, 23));

    floorTile.setOutlineThickness(1);
    floorTile.setOutlineColor(sf::Color(10, 10, 14));
    window.draw(floorTile);

    sf::RectangleShape crack;
    crack.setSize(sf::Vector2f(TILE_SIZE / 3.0f, 1));
    crack.setPosition(sf::Vector2f(x + 8, y + TILE_SIZE - 10));
    crack.setFillColor(sf::Color(45, 43, 50));
    window.draw(crack);
}

void drawSolidBlock(sf::RenderWindow& window, int row, int col)
{
    float x = col * TILE_SIZE;
    float y = row * TILE_SIZE;

    sf::RectangleShape block;
    block.setSize(sf::Vector2f(TILE_SIZE - 4, TILE_SIZE - 4));
    block.setPosition(sf::Vector2f(x + 2, y + 2));
    block.setFillColor(sf::Color(45, 42, 52));
    block.setOutlineThickness(2);
    block.setOutlineColor(sf::Color(12, 12, 18));
    window.draw(block);

    sf::RectangleShape topLight;
    topLight.setSize(sf::Vector2f(TILE_SIZE - 10, 4));
    topLight.setPosition(sf::Vector2f(x + 5, y + 6));
    topLight.setFillColor(sf::Color(75, 70, 85));
    window.draw(topLight);

    sf::RectangleShape sideShadow;
    sideShadow.setSize(sf::Vector2f(5, TILE_SIZE - 10));
    sideShadow.setPosition(sf::Vector2f(x + TILE_SIZE - 10, y + 5));
    sideShadow.setFillColor(sf::Color(25, 23, 32));
    window.draw(sideShadow);

    sf::RectangleShape bottomShadow;
    bottomShadow.setSize(sf::Vector2f(TILE_SIZE - 10, 5));
    bottomShadow.setPosition(sf::Vector2f(x + 5, y + TILE_SIZE - 10));
    bottomShadow.setFillColor(sf::Color(20, 19, 26));
    window.draw(bottomShadow);
}

void drawBreakableBlock(sf::RenderWindow& window, int row, int col)
{
    float x = col * TILE_SIZE;
    float y = row * TILE_SIZE;

    sf::RectangleShape crate;
    crate.setSize(sf::Vector2f(TILE_SIZE - 6, TILE_SIZE - 6));
    crate.setPosition(sf::Vector2f(x + 3, y + 3));
    crate.setFillColor(sf::Color(105, 62, 32));
    crate.setOutlineThickness(2);
    crate.setOutlineColor(sf::Color(42, 24, 13));
    window.draw(crate);

    sf::RectangleShape topPlank;
    topPlank.setSize(sf::Vector2f(TILE_SIZE - 12, 5));
    topPlank.setPosition(sf::Vector2f(x + 6, y + 8));
    topPlank.setFillColor(sf::Color(145, 88, 45));
    window.draw(topPlank);

    sf::RectangleShape bottomPlank;
    bottomPlank.setSize(sf::Vector2f(TILE_SIZE - 12, 5));
    bottomPlank.setPosition(sf::Vector2f(x + 6, y + TILE_SIZE - 13));
    bottomPlank.setFillColor(sf::Color(72, 40, 20));
    window.draw(bottomPlank);

    sf::RectangleShape leftPlank;
    leftPlank.setSize(sf::Vector2f(5, TILE_SIZE - 12));
    leftPlank.setPosition(sf::Vector2f(x + 8, y + 6));
    leftPlank.setFillColor(sf::Color(82, 45, 22));
    window.draw(leftPlank);

    sf::RectangleShape rightPlank;
    rightPlank.setSize(sf::Vector2f(5, TILE_SIZE - 12));
    rightPlank.setPosition(sf::Vector2f(x + TILE_SIZE - 13, y + 6));
    rightPlank.setFillColor(sf::Color(82, 45, 22));
    window.draw(rightPlank);

    sf::RectangleShape middleLine;
    middleLine.setSize(sf::Vector2f(TILE_SIZE - 14, 3));
    middleLine.setPosition(sf::Vector2f(x + 7, y + TILE_SIZE / 2));
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

void drawPlayer(sf::RenderWindow& window, int row, int col)
{
    float x = col * TILE_SIZE;
    float y = row * TILE_SIZE;

    // Red back cape
    sf::ConvexShape cape;
    cape.setPointCount(4);
    cape.setPoint(0, sf::Vector2f(x + TILE_SIZE * 0.50f, y + TILE_SIZE * 0.20f));
    cape.setPoint(1, sf::Vector2f(x + TILE_SIZE * 0.78f, y + TILE_SIZE * 0.92f));
    cape.setPoint(2, sf::Vector2f(x + TILE_SIZE * 0.50f, y + TILE_SIZE * 0.98f));
    cape.setPoint(3, sf::Vector2f(x + TILE_SIZE * 0.22f, y + TILE_SIZE * 0.92f));
    cape.setFillColor(sf::Color(80, 15, 22));
    cape.setOutlineThickness(1.5f);
    cape.setOutlineColor(sf::Color(35, 5, 8));
    window.draw(cape);

    // Legs
    sf::RectangleShape leftLeg;
    leftLeg.setSize(sf::Vector2f(TILE_SIZE * 0.11f, TILE_SIZE * 0.20f));
    leftLeg.setPosition(sf::Vector2f(x + TILE_SIZE * 0.36f, y + TILE_SIZE * 0.73f));
    leftLeg.setFillColor(sf::Color(95, 95, 105));
    window.draw(leftLeg);

    sf::RectangleShape rightLeg;
    rightLeg.setSize(sf::Vector2f(TILE_SIZE * 0.11f, TILE_SIZE * 0.20f));
    rightLeg.setPosition(sf::Vector2f(x + TILE_SIZE * 0.53f, y + TILE_SIZE * 0.73f));
    rightLeg.setFillColor(sf::Color(95, 95, 105));
    window.draw(rightLeg);

    // Armor body
    sf::RectangleShape body;
    body.setSize(sf::Vector2f(TILE_SIZE * 0.38f, TILE_SIZE * 0.34f));
    body.setPosition(sf::Vector2f(x + TILE_SIZE * 0.31f, y + TILE_SIZE * 0.42f));
    body.setFillColor(sf::Color(135, 135, 150));
    body.setOutlineThickness(2);
    body.setOutlineColor(sf::Color(45, 45, 55));
    window.draw(body);

    // Armor center shine
    sf::RectangleShape armorShine;
    armorShine.setSize(sf::Vector2f(TILE_SIZE * 0.08f, TILE_SIZE * 0.28f));
    armorShine.setPosition(sf::Vector2f(x + TILE_SIZE * 0.45f, y + TILE_SIZE * 0.45f));
    armorShine.setFillColor(sf::Color(175, 175, 190));
    window.draw(armorShine);

    // Arms
    sf::RectangleShape leftArm;
    leftArm.setSize(sf::Vector2f(TILE_SIZE * 0.09f, TILE_SIZE * 0.24f));
    leftArm.setPosition(sf::Vector2f(x + TILE_SIZE * 0.22f, y + TILE_SIZE * 0.45f));
    leftArm.setFillColor(sf::Color(110, 110, 125));
    window.draw(leftArm);

    sf::RectangleShape rightArm;
    rightArm.setSize(sf::Vector2f(TILE_SIZE * 0.09f, TILE_SIZE * 0.24f));
    rightArm.setPosition(sf::Vector2f(x + TILE_SIZE * 0.70f, y + TILE_SIZE * 0.45f));
    rightArm.setFillColor(sf::Color(110, 110, 125));
    window.draw(rightArm);

    // Helmet
    sf::CircleShape helmet(TILE_SIZE * 0.17f);
    helmet.setPosition(sf::Vector2f(x + TILE_SIZE * 0.33f, y + TILE_SIZE * 0.12f));
    helmet.setFillColor(sf::Color(160, 160, 175));
    helmet.setOutlineThickness(2);
    helmet.setOutlineColor(sf::Color(50, 50, 62));
    window.draw(helmet);

    // Helmet lower plate
    sf::RectangleShape helmetPlate;
    helmetPlate.setSize(sf::Vector2f(TILE_SIZE * 0.32f, TILE_SIZE * 0.12f));
    helmetPlate.setPosition(sf::Vector2f(x + TILE_SIZE * 0.34f, y + TILE_SIZE * 0.27f));
    helmetPlate.setFillColor(sf::Color(145, 145, 160));
    helmetPlate.setOutlineThickness(1);
    helmetPlate.setOutlineColor(sf::Color(50, 50, 62));
    window.draw(helmetPlate);

    // Dark visor
    sf::RectangleShape visor;
    visor.setSize(sf::Vector2f(TILE_SIZE * 0.19f, TILE_SIZE * 0.055f));
    visor.setPosition(sf::Vector2f(x + TILE_SIZE * 0.405f, y + TILE_SIZE * 0.245f));
    visor.setFillColor(sf::Color(20, 20, 25));
    window.draw(visor);

    // Helmet crest
    sf::RectangleShape crest;
    crest.setSize(sf::Vector2f(TILE_SIZE * 0.07f, TILE_SIZE * 0.14f));
    crest.setPosition(sf::Vector2f(x + TILE_SIZE * 0.465f, y + TILE_SIZE * 0.035f));
    crest.setFillColor(sf::Color(145, 25, 30));
    window.draw(crest);

    // Shield
    sf::ConvexShape shield;
    shield.setPointCount(5);
    shield.setPoint(0, sf::Vector2f(x + TILE_SIZE * 0.13f, y + TILE_SIZE * 0.44f));
    shield.setPoint(1, sf::Vector2f(x + TILE_SIZE * 0.26f, y + TILE_SIZE * 0.39f));
    shield.setPoint(2, sf::Vector2f(x + TILE_SIZE * 0.31f, y + TILE_SIZE * 0.55f));
    shield.setPoint(3, sf::Vector2f(x + TILE_SIZE * 0.22f, y + TILE_SIZE * 0.74f));
    shield.setPoint(4, sf::Vector2f(x + TILE_SIZE * 0.10f, y + TILE_SIZE * 0.58f));
    shield.setFillColor(sf::Color(115, 18, 25));
    shield.setOutlineThickness(2);
    shield.setOutlineColor(sf::Color(190, 170, 75));
    window.draw(shield);

    // Sword blade
    sf::RectangleShape sword;
    sword.setSize(sf::Vector2f(TILE_SIZE * 0.055f, TILE_SIZE * 0.34f));
    sword.setPosition(sf::Vector2f(x + TILE_SIZE * 0.80f, y + TILE_SIZE * 0.30f));
    sword.setFillColor(sf::Color(205, 205, 215));
    sword.setRotation(sf::degrees(18.f));
    window.draw(sword);

    // Sword handle
    sf::RectangleShape swordHandle;
    swordHandle.setSize(sf::Vector2f(TILE_SIZE * 0.16f, TILE_SIZE * 0.045f));
    swordHandle.setPosition(sf::Vector2f(x + TILE_SIZE * 0.75f, y + TILE_SIZE * 0.56f));
    swordHandle.setFillColor(sf::Color(95, 65, 25));
    swordHandle.setRotation(sf::degrees(18.f));
    window.draw(swordHandle);
}

void drawEnemy(sf::RenderWindow& window, int row, int col)
{
    float x = col * TILE_SIZE;
    float y = row * TILE_SIZE;

    sf::Color skin(65, 105, 42);
    sf::Color darkSkin(35, 65, 25);
    sf::Color outline(18, 35, 15);

    // Left sharp ear
    sf::ConvexShape leftEar;
    leftEar.setPointCount(3);
    leftEar.setPoint(0, sf::Vector2f(x + TILE_SIZE * 0.31f, y + TILE_SIZE * 0.24f));
    leftEar.setPoint(1, sf::Vector2f(x + TILE_SIZE * 0.07f, y + TILE_SIZE * 0.15f));
    leftEar.setPoint(2, sf::Vector2f(x + TILE_SIZE * 0.24f, y + TILE_SIZE * 0.39f));
    leftEar.setFillColor(skin);
    leftEar.setOutlineThickness(1.5f);
    leftEar.setOutlineColor(outline);
    window.draw(leftEar);

    // Right sharp ear
    sf::ConvexShape rightEar;
    rightEar.setPointCount(3);
    rightEar.setPoint(0, sf::Vector2f(x + TILE_SIZE * 0.69f, y + TILE_SIZE * 0.24f));
    rightEar.setPoint(1, sf::Vector2f(x + TILE_SIZE * 0.93f, y + TILE_SIZE * 0.15f));
    rightEar.setPoint(2, sf::Vector2f(x + TILE_SIZE * 0.76f, y + TILE_SIZE * 0.39f));
    rightEar.setFillColor(skin);
    rightEar.setOutlineThickness(1.5f);
    rightEar.setOutlineColor(outline);
    window.draw(rightEar);

    // Body
    sf::RectangleShape body;
    body.setSize(sf::Vector2f(TILE_SIZE * 0.34f, TILE_SIZE * 0.28f));
    body.setPosition(sf::Vector2f(x + TILE_SIZE * 0.33f, y + TILE_SIZE * 0.56f));
    body.setFillColor(darkSkin);
    body.setOutlineThickness(2);
    body.setOutlineColor(outline);
    window.draw(body);

    // Head
    sf::CircleShape head(TILE_SIZE * 0.23f);
    head.setPosition(sf::Vector2f(x + TILE_SIZE * 0.27f, y + TILE_SIZE * 0.13f));
    head.setFillColor(skin);
    head.setOutlineThickness(2);
    head.setOutlineColor(outline);
    window.draw(head);

    // Red eyes
    sf::CircleShape leftEye(TILE_SIZE * 0.04f);
    leftEye.setPosition(sf::Vector2f(x + TILE_SIZE * 0.37f, y + TILE_SIZE * 0.27f));
    leftEye.setFillColor(sf::Color(190, 20, 20));
    window.draw(leftEye);

    sf::CircleShape rightEye(TILE_SIZE * 0.04f);
    rightEye.setPosition(sf::Vector2f(x + TILE_SIZE * 0.55f, y + TILE_SIZE * 0.27f));
    rightEye.setFillColor(sf::Color(190, 20, 20));
    window.draw(rightEye);

    // Angry eyebrows
    sf::RectangleShape leftBrow;
    leftBrow.setSize(sf::Vector2f(TILE_SIZE * 0.12f, TILE_SIZE * 0.035f));
    leftBrow.setPosition(sf::Vector2f(x + TILE_SIZE * 0.34f, y + TILE_SIZE * 0.235f));
    leftBrow.setFillColor(sf::Color(10, 20, 8));
    leftBrow.setRotation(sf::degrees(-20.f));
    window.draw(leftBrow);

    sf::RectangleShape rightBrow;
    rightBrow.setSize(sf::Vector2f(TILE_SIZE * 0.12f, TILE_SIZE * 0.035f));
    rightBrow.setPosition(sf::Vector2f(x + TILE_SIZE * 0.54f, y + TILE_SIZE * 0.20f));
    rightBrow.setFillColor(sf::Color(10, 20, 8));
    rightBrow.setRotation(sf::degrees(20.f));
    window.draw(rightBrow);

    // Mouth
    sf::RectangleShape mouth;
    mouth.setSize(sf::Vector2f(TILE_SIZE * 0.17f, TILE_SIZE * 0.055f));
    mouth.setPosition(sf::Vector2f(x + TILE_SIZE * 0.42f, y + TILE_SIZE * 0.42f));
    mouth.setFillColor(sf::Color(45, 0, 0));
    window.draw(mouth);

    // Teeth
    sf::ConvexShape tooth1;
    tooth1.setPointCount(3);
    tooth1.setPoint(0, sf::Vector2f(x + TILE_SIZE * 0.44f, y + TILE_SIZE * 0.47f));
    tooth1.setPoint(1, sf::Vector2f(x + TILE_SIZE * 0.48f, y + TILE_SIZE * 0.55f));
    tooth1.setPoint(2, sf::Vector2f(x + TILE_SIZE * 0.52f, y + TILE_SIZE * 0.47f));
    tooth1.setFillColor(sf::Color(230, 225, 190));
    window.draw(tooth1);

    sf::ConvexShape tooth2;
    tooth2.setPointCount(3);
    tooth2.setPoint(0, sf::Vector2f(x + TILE_SIZE * 0.54f, y + TILE_SIZE * 0.47f));
    tooth2.setPoint(1, sf::Vector2f(x + TILE_SIZE * 0.58f, y + TILE_SIZE * 0.55f));
    tooth2.setPoint(2, sf::Vector2f(x + TILE_SIZE * 0.62f, y + TILE_SIZE * 0.47f));
    tooth2.setFillColor(sf::Color(230, 225, 190));
    window.draw(tooth2);

    // Arms
    sf::RectangleShape leftArm;
    leftArm.setSize(sf::Vector2f(TILE_SIZE * 0.09f, TILE_SIZE * 0.23f));
    leftArm.setPosition(sf::Vector2f(x + TILE_SIZE * 0.24f, y + TILE_SIZE * 0.58f));
    leftArm.setFillColor(skin);
    leftArm.setRotation(sf::degrees(22.f));
    window.draw(leftArm);

    sf::RectangleShape rightArm;
    rightArm.setSize(sf::Vector2f(TILE_SIZE * 0.09f, TILE_SIZE * 0.23f));
    rightArm.setPosition(sf::Vector2f(x + TILE_SIZE * 0.69f, y + TILE_SIZE * 0.58f));
    rightArm.setFillColor(skin);
    rightArm.setRotation(sf::degrees(-22.f));
    window.draw(rightArm);

    // Claws
    sf::CircleShape leftClaw(TILE_SIZE * 0.035f);
    leftClaw.setPosition(sf::Vector2f(x + TILE_SIZE * 0.18f, y + TILE_SIZE * 0.76f));
    leftClaw.setFillColor(sf::Color(220, 215, 170));
    window.draw(leftClaw);

    sf::CircleShape rightClaw(TILE_SIZE * 0.035f);
    rightClaw.setPosition(sf::Vector2f(x + TILE_SIZE * 0.78f, y + TILE_SIZE * 0.76f));
    rightClaw.setFillColor(sf::Color(220, 215, 170));
    window.draw(rightClaw);

    // Feet
    sf::RectangleShape leftFoot;
    leftFoot.setSize(sf::Vector2f(TILE_SIZE * 0.11f, TILE_SIZE * 0.09f));
    leftFoot.setPosition(sf::Vector2f(x + TILE_SIZE * 0.34f, y + TILE_SIZE * 0.84f));
    leftFoot.setFillColor(sf::Color(25, 50, 18));
    window.draw(leftFoot);

    sf::RectangleShape rightFoot;
    rightFoot.setSize(sf::Vector2f(TILE_SIZE * 0.11f, TILE_SIZE * 0.09f));
    rightFoot.setPosition(sf::Vector2f(x + TILE_SIZE * 0.55f, y + TILE_SIZE * 0.84f));
    rightFoot.setFillColor(sf::Color(25, 50, 18));
    window.draw(rightFoot);
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({COLS * TILE_SIZE, ROWS * TILE_SIZE}),
        "Bomberman Dungeon Arena - Level 1"
    );

    window.setFramerateLimit(60);

    int playerRow = ROWS / 2;
    int playerCol = COLS / 2;

    std::vector<sf::Vector2i> enemies =
    {
        {1, 1},
        {1, COLS - 2},
        {ROWS - 2, 1},
        {ROWS - 2, COLS - 2}
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

        window.clear(sf::Color(6, 6, 10));

        drawTileMap(window);

        drawPlayer(window, playerRow, playerCol);

        drawEnemy(window, enemies[0].x, enemies[0].y);
        drawEnemy(window, enemies[1].x, enemies[1].y);
        drawEnemy(window, enemies[2].x, enemies[2].y);
        drawEnemy(window, enemies[3].x, enemies[3].y);

        window.display();
    }

    return 0;
}
