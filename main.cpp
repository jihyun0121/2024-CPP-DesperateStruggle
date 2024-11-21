#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

using namespace sf;
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

// 버튼 클래스 정의
class Button {
public:
    Button(const wstring& text, const Font& font, float x, float y, float width, float height,
        Color bgColor = Color::White, Color textColor = Color::Black) {
        // 버튼 배경 설정
        buttonShape.setSize(Vector2f(width, height));
        buttonShape.setFillColor(bgColor);
        buttonShape.setPosition(x, y);

        // 버튼 텍스트 설정
        buttonText.setFont(font);
        buttonText.setString(text);
        buttonText.setCharacterSize(24);
        buttonText.setFillColor(textColor);
        buttonText.setPosition(
            x + (width - buttonText.getGlobalBounds().width) / 2,
            y + (height - buttonText.getGlobalBounds().height) / 2 - 5
        );
    }

    // 버튼 그리기
    void draw(RenderWindow& window) {
        window.draw(buttonShape);
        window.draw(buttonText);
    }

    // 버튼 클릭 여부 확인
    bool isClicked(Vector2i mousePos) {
        return buttonShape.getGlobalBounds().contains(static_cast<Vector2f>(mousePos));
    }

private:
    RectangleShape buttonShape;
    Text buttonText;
};

// TitleScreen 클래스 정의
class TitleScreen {
public:
    TitleScreen(float width, float height, const Font& font)
        : startButton(L"- Game Start -", font, width / 2 - 100, height / 2, 200, 50) {
        // 타이틀 텍스트
        titleText.setFont(font);
        titleText.setString("Desperate Struggle");
        titleText.setCharacterSize(48);
        titleText.setFillColor(Color::Black);
        titleText.setPosition(width / 2 - titleText.getGlobalBounds().width / 2, height / 3);
    }

    void draw(RenderWindow& window) {
        window.draw(titleText);
        startButton.draw(window);
    }

    bool isStartButtonClicked(Vector2i mousePos) {
        return startButton.isClicked(mousePos);
    }

private:
    Text titleText;
    Button startButton;
};

// GameChoice 클래스 정의
class GameChoice {
public:
    GameChoice(float width, float height, const Font& font)
        : createRoomButton(L"게임 생성", font, width / 2 - 125, height / 2 - 60, 250, 50, Color(217, 217, 217)),
        joinRoomButton(L"게임 참여", font, width / 2 - 125, height / 2 + 20, 250, 50, Color(217, 217, 217)) {
        // "게임 방 선택" 텍스트
        choiceText.setFont(font);
        choiceText.setString(L"게임 선택");
        choiceText.setCharacterSize(36);
        choiceText.setFillColor(Color::Black);
        choiceText.setPosition(width / 2 - choiceText.getGlobalBounds().width / 2, height / 4);
    }

    void draw(RenderWindow& window) {
        window.draw(choiceText);
        createRoomButton.draw(window);
        joinRoomButton.draw(window);
    }

    bool isCreateRoomClicked(Vector2i mousePos) {
        return createRoomButton.isClicked(mousePos);
    }

    bool isJoinRoomClicked(Vector2i mousePos) {
        return joinRoomButton.isClicked(mousePos);
    }

private:
    Text choiceText;
    Button createRoomButton;
    Button joinRoomButton;
};

// main 함수
int main() {
    RenderWindow window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Russian Roulette");
    Font font;

    if (!font.loadFromFile("assets/Freesentation-5Medium.ttf")) {
        cerr << "폰트 로드 실패, 기본 폰트 사용" << endl;
        font.loadFromFile("C:\\Windows\\Fonts\\Arial.TTF");
    }

    TitleScreen titleScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    GameChoice GameChoice(WINDOW_WIDTH, WINDOW_HEIGHT, font);

    bool isTitleScreen = true;

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            if (isTitleScreen) {
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    Vector2i mousePos = Mouse::getPosition(window);
                    if (titleScreen.isStartButtonClicked(mousePos)) {
                        isTitleScreen = false; // 선택 화면으로 전환
                    }
                }
            }
            else {
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    Vector2i mousePos = Mouse::getPosition(window);
                    if (GameChoice.isCreateRoomClicked(mousePos)) {
                        cout << L"게임 방 만들기 선택!" << endl;
                        // 게임 방 만들기 로직 추가
                    }
                    else if (GameChoice.isJoinRoomClicked(mousePos)) {
                        cout << L"게임 방 입장 선택!" << endl;
                        // 게임 방 입장 로직 추가
                    }
                }
            }
        }

        window.clear(Color::White);
        if (isTitleScreen) {
            titleScreen.draw(window);
        }
        else {
            GameChoice.draw(window);
        }
        window.display();
    }

    return 0;
}
