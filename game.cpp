#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

using namespace sf;
using namespace std;

// Bullet 클래스 정의
class Bullet {
public:
    enum Type { EMPTY, LIVE };  // EMPTY와 LIVE로 수정

    Bullet(Type t) : type(t) {}

    bool operator==(const Bullet& other) const {
        return this->type == other.type;
    }

    Type getType() const { return type; }

private:
    Type type;
};

// Item 클래스 정의
class Item {
public:
    enum Type { HEAL, DAMAGE };

    Item(Type t) : type(t) {}

    bool operator==(const Item& other) const {
        return this->type == other.type;
    }

private:
    Type type;
};

// 플레이어 클래스
class Player {
public:
    string name;
    int health = 5;
    bool isTurn = false;
    int turn = 0;

    Player(const string& playerName) : name(playerName) {}

    string getName() { return name; }
    int getHealth() { return health; }
    void addHealth(int h) { health = min(health + h, 8); }
    bool isTurnActive() { return isTurn; }
    void toggleTurn() { isTurn = !isTurn; }
    void shootBullet(vector<Bullet>& bullets) {
        if (bullets[turn] == Bullet(Bullet::LIVE)) {
            health--;
            cout << name << " got hit! Health: " << health << endl;
        }
        turn = (turn + 1) % bullets.size();
    }

    void useItem(vector<Item>& items) {
        for (auto& item : items) {
            if (item == Item(Item::HEAL)) {
                addHealth(1);
                cout << name << " healed! Health: " << health << endl;
            }
        }
    }
};

class Game {
public:
    Game() : window(VideoMode(1200, 800), "Russian Roulette"), player1("You"), player2("Opponent") {
        font.loadFromFile("assets/Freesentation-5Medium.ttf");
        srand(time(0));
        createBullets();
        createItems();
    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            update();
            render();
        }
    }

    void setTurnForPlayer(Player& player, bool turnStatus) {
        player.isTurn = turnStatus;
    }

private:
    RenderWindow window;
    Font font;
    Player player1;
    Player player2;
    vector<Bullet> bullets;
    vector<Item> items;
    Text turnText, healthText1, healthText2, bulletInfoText;

    void createBullets() {
        for (int i = 0; i < 8; ++i) {
            if (rand() % 4 == 0) {
                bullets.push_back(Bullet(Bullet::LIVE));
            }
            else {
                bullets.push_back(Bullet(Bullet::EMPTY));  // EMPTY로 수정
            }
        }
    }

    void createItems() {
        for (int i = 0; i < 3; ++i) {
            items.push_back(Item(Item::HEAL));  // 회복 아이템 추가
        }
    }

    void handleEvents() {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Up) {
                    if (player1.isTurnActive()) {
                        player1.shootBullet(bullets);
                        player1.useItem(items);
                        player1.toggleTurn();
                        player2.toggleTurn();
                    }
                }

                if (event.key.code == Keyboard::Down) {
                    if (player2.isTurnActive()) {
                        player2.shootBullet(bullets);
                        player2.useItem(items);
                        player2.toggleTurn();
                        player1.toggleTurn();
                    }
                }
            }
        }
    }

    void update() {
        string healthText = "Health: " + to_string(player1.getHealth()) + " / " + to_string(player2.getHealth());
        healthText1.setString(healthText);
        healthText1.setFont(font);
        healthText1.setCharacterSize(24);
        healthText1.setPosition(10, 10);

        healthText = "Health: " + to_string(player2.getHealth()) + " / " + to_string(player1.getHealth());
        healthText2.setString(healthText);
        healthText2.setFont(font);
        healthText2.setCharacterSize(24);
        healthText2.setPosition(1000, 10);

        bulletInfoText.setFont(font);
        bulletInfoText.setCharacterSize(20);
        bulletInfoText.setFillColor(Color::Red);
        bulletInfoText.setPosition(10, 50);
        string bulletInfo = "Bullets: ";
        int emptyCount = 0, liveCount = 0;
        for (auto& b : bullets) {
            if (b.getType() == Bullet::EMPTY)
                emptyCount++;
            else
                liveCount++;
        }
        bulletInfo += "Empty: " + to_string(emptyCount) + " Live: " + to_string(liveCount);
        bulletInfoText.setString(bulletInfo);

        if (player1.isTurnActive()) {
            turnText.setString("Turn: " + player1.getName());
        }
        else {
            turnText.setString("Turn: " + player2.getName());
        }

        turnText.setFont(font);
        turnText.setCharacterSize(30);
        turnText.setPosition(500, 700);
    }

    void render() {
        window.clear(Color::White);

        window.draw(turnText);
        window.draw(healthText1);
        window.draw(healthText2);
        window.draw(bulletInfoText);

        window.display();
    }
};

