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
    enum Type { EMPTY, LIVE };

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
    enum Type { HEAL, MAGNIFY, COIN, BAN };

    Item(Type t) : type(t) {}

    bool operator==(const Item& other) const {
        return this->type == other.type;
    }

    Type getType() const { return type; }

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
    vector<Item> items;

    Player(const string& playerName) : name(playerName) {}

    string getName() { return name; }
    int getHealth() { return health; }
    void addHealth(int h) { health = min(health + h, 8); }
    void useItem(vector<Bullet>& bullets) {
        if (!items.empty()) {
            Item item = items.back();
            items.pop_back();
            switch (item.getType()) {
            case Item::HEAL:
                addHealth(1);
                cout << name << " healed! Health: " << health << endl;
                break;
            case Item::MAGNIFY:
                revealBullets(bullets);
                break;
            case Item::COIN:
                // 코인 사용: 총알 한 개를 소모하고 턴 넘기지 않음
                if (!bullets.empty()) {
                    bullets.pop_back();
                    cout << name << " used a coin and shot a bullet!" << endl;
                }
                break;
            case Item::BAN:
                // 금지: 상대방 한 턴 쉬게하기
                cout << name << " used a BAN item! Opponent skips next turn!" << endl;
                // 게임 로직에서 상대방의 턴을 건너뛰도록 설정 필요
                break;
            }
        }
    }

    void revealBullets(vector<Bullet>& bullets) {
        int emptyCount = 0, liveCount = 0;
        for (auto& b : bullets) {
            if (b.getType() == Bullet::EMPTY) {
                emptyCount++;
            }
            else {
                liveCount++;
            }
        }
        cout << "Bullets - Empty: " << emptyCount << " Live: " << liveCount << endl;
    }

    bool isTurnActive() { return isTurn; }
    void toggleTurn() { isTurn = !isTurn; }
    void shootBullet(vector<Bullet>& bullets) {
        if (bullets[turn] == Bullet(Bullet::LIVE)) {
            health--;
            cout << name << " got hit! Health: " << health << endl;
        }
        turn = (turn + 1) % bullets.size();
    }
};

// Game 클래스
class Game {
public:
    Game() : window(VideoMode(1200, 800), "Russian Roulette"), player1("You"), player2("Opponent") {
        if (!font.loadFromFile("assets/Freesentation-5Medium.ttf")) {
            throw runtime_error("폰트를 로드할 수 없습니다!");
        }
        srand(static_cast<unsigned>(time(0)));
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
    bool skipTurn = false;  // 상대방의 턴을 건너뛰는 상태를 관리

    void createBullets() {
        bullets.clear();
        int bulletCount = rand() % 5 + 4; // 총알의 개수는 최소 4개에서 최대 8개 사이
        bool hasLive = false, hasEmpty = false;

        for (int i = 0; i < bulletCount; ++i) {
            Bullet::Type type = (rand() % 2 == 0 && !hasEmpty) ? Bullet::EMPTY : Bullet::LIVE;
            if (type == Bullet::LIVE) hasLive = true;
            else hasEmpty = true;

            bullets.push_back(Bullet(type));
        }

        // 최소 1개의 실탄과 빈탄을 보장
        if (!hasLive) {
            bullets[rand() % bulletCount] = Bullet(Bullet::LIVE);
        }
        if (!hasEmpty) {
            bullets[rand() % bulletCount] = Bullet(Bullet::EMPTY);
        }
    }

    void createItems() {
        items.clear();
        // 아이템 4개 종류로 구성: 체력회복, 돋보기, 코인, 금지
        for (int i = 0; i < 3; ++i) {
            int itemType = rand() % 4;
            items.push_back(Item(static_cast<Item::Type>(itemType)));
        }
    }

    void handleEvents() {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Up && player1.isTurnActive() && !skipTurn) {
                    player1.shootBullet(bullets);
                    player1.useItem(bullets);
                    player1.toggleTurn();
                    player2.toggleTurn();
                    skipTurn = false;  // 턴을 넘겼으므로 skipTurn 초기화
                }
                if (event.key.code == Keyboard::Down && player2.isTurnActive() && !skipTurn) {
                    player2.shootBullet(bullets);
                    player2.useItem(bullets);
                    player2.toggleTurn();
                    player1.toggleTurn();
                    skipTurn = false;  // 턴을 넘겼으므로 skipTurn 초기화
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
            if (b.getType() == Bullet::EMPTY) {
                emptyCount++;
            }
            else {
                liveCount++;
            }
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

    // 상대방의 턴을 건너뛰는 기능 추가
    void skipOpponentTurn() {
        skipTurn = true;
        cout << "Opponent's turn is skipped!" << endl;
    }
};

