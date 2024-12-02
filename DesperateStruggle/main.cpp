#define _WINSOCK_DEPRECATED_NO_WARNINGS // 경고 비활성화
#include <SFML/Graphics.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include "game.cpp"

#pragma comment(lib, "ws2_32.lib") // WinSock 라이브러리 링크

using namespace sf;
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

SOCKET clientSocket; // 클라이언트 소켓

// 클라이언트 종료 처리 함수
void cleanupClient() {
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        WSACleanup();
        cout << "클라이언트 종료 및 정리 완료." << endl;
    }
}

// 서버 응답 처리 함수
void listenServer(bool& isWaitingScreen, bool& isGameStart, bool& isPlayerTurn,
    string& bulletData, string& lifeData, string& itemData) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "서버와의 연결이 끊어졌습니다." << endl;
            cleanupClient();
            return;
        }

        string message(buffer);
        if (message == "WAIT\n") {
            isWaitingScreen = true;
        }
        else if (message == "START\n") {
            isGameStart = true;
            break;
        }
        else if (message.rfind("BULLETS\n", 0) == 0) {
            bulletData = message.substr(8);
        }
        else if (message.rfind("LIVES\n", 0) == 0) {
            lifeData = message.substr(6);
        }
        else if (message.rfind("ITEMS\n", 0) == 0) {
            itemData = message.substr(6);
        }
        else if (message == "TURN:YOU\n") {
            isPlayerTurn = true;
        }
        else if (message == "TURN:RIVER\n") {
            isPlayerTurn = false;
        }
    }
}



// Button 클래스 정의
class Button {
public:
    Button(const wstring& text, const Font& font, float x, float y, float width, float height,
        Color bgColor = Color::White, Color textColor = Color::Black) {
        buttonShape.setSize(Vector2f(width, height));
        buttonShape.setFillColor(bgColor);
        buttonShape.setPosition(x, y);

        buttonText.setFont(font);
        buttonText.setString(text);
        buttonText.setCharacterSize(24);
        buttonText.setFillColor(textColor);
        buttonText.setPosition(
            x + (width - buttonText.getGlobalBounds().width) / 2,
            y + (height - buttonText.getGlobalBounds().height) / 2 - 5
        );
    }

    void draw(RenderWindow& window) {
        window.draw(buttonShape);
        window.draw(buttonText);
    }

    bool isClicked(Vector2i mousePos) {
        return buttonShape.getGlobalBounds().contains(static_cast<Vector2f>(mousePos));
    }

private:
    RectangleShape buttonShape;
    Text buttonText;
};

class TitleScreen {
public:
    TitleScreen(float width, float height, const Font& font)
        : startButton(L"- 게임 시작하기 -", font, width / 2 - 100, height / 2, 200, 50) {
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

class ChoiceRoomScreen {
public:
    ChoiceRoomScreen(float width, float height, const Font& font)
        : createButton(L"게임 생성", font, width / 2 - 200, height / 2, 150, 50),
        joinButton(L"게임 참여", font, width / 2 + 50, height / 2, 150, 50) {
        titleText.setFont(font);
        titleText.setString(L"방 선택");
        titleText.setCharacterSize(36);
        titleText.setFillColor(Color::Black);
        titleText.setPosition(width / 2 - titleText.getGlobalBounds().width / 2, height / 4);
    }

    void draw(RenderWindow& window) {
        window.draw(titleText);
        createButton.draw(window);
        joinButton.draw(window);
    }

    bool isCreateButtonClicked(Vector2i mousePos) {
        return createButton.isClicked(mousePos);
    }

    bool isJoinButtonClicked(Vector2i mousePos) {
        return joinButton.isClicked(mousePos);
    }

private:
    Text titleText;
    Button createButton;
    Button joinButton;
};

class CreateRoomScreen {
public:
    CreateRoomScreen(float width, float height, const Font& font)
        : createButton(L"방 생성", font, width / 2 - 100, height / 2 + 100, 200, 50) {
        titleText.setFont(font);
        titleText.setString(L"방 생성");
        titleText.setCharacterSize(36);
        titleText.setFillColor(Color::Black);
        titleText.setPosition(width / 2 - titleText.getGlobalBounds().width / 2, height / 4);

        passwordPrompt.setFont(font);
        passwordPrompt.setString(L"비밀번호:");
        passwordPrompt.setCharacterSize(24);
        passwordPrompt.setFillColor(Color::Black);
        passwordPrompt.setPosition(width / 2 - 200, height / 2 - 150);

        passwordInput.setFont(font);
        passwordInput.setCharacterSize(24);
        passwordInput.setFillColor(Color::Black);
        passwordInput.setPosition(width / 2 - 100, height / 2 - 150);
        passwordInput.setString(""); // 초기화
    }

    void draw(RenderWindow& window) {
        window.draw(titleText);
        window.draw(passwordPrompt);
        window.draw(passwordInput);
        createButton.draw(window);
    }

    void handleInput(Event event) {
        if (event.type == Event::TextEntered) {
            if (event.text.unicode == '\b' && !password.empty()) { // 백스페이스 처리
                password.pop_back();
            }
            else if ((event.text.unicode < 128 || (event.text.unicode >= 44032 && event.text.unicode <= 55203))) {
                // 영문자, 숫자, 한글 범위 허용
                password += static_cast<char>(event.text.unicode);
            }
            passwordInput.setString(password);
        }
    }

    void resetInput() {
        password.clear();
        passwordInput.setString("");
    }

    bool isCreateButtonClicked(Vector2i mousePos) {
        return createButton.isClicked(mousePos);
    }

    bool isPasswordEntered() const {
        return !password.empty();
    }

    string getPassword() const {
        return password;
    }

private:
    Text titleText, passwordPrompt, passwordInput;
    Button createButton;
    string password;
};

class JoinRoomScreen {
public:
    JoinRoomScreen(float width, float height, const Font& font)
        : joinButton(L"방 참여", font, width / 2 - 100, height / 2 + 100, 200, 50) {
        titleText.setFont(font);
        titleText.setString(L"방 참여");
        titleText.setCharacterSize(36);
        titleText.setFillColor(Color::Black);
        titleText.setPosition(width / 2 - titleText.getGlobalBounds().width / 2, height / 4);

        passwordPrompt.setFont(font);
        passwordPrompt.setString(L"비밀번호:");
        passwordPrompt.setCharacterSize(24);
        passwordPrompt.setFillColor(Color::Black);
        passwordPrompt.setPosition(width / 2 - 200, height / 2 - 150);

        passwordInput.setFont(font);
        passwordInput.setCharacterSize(24);
        passwordInput.setFillColor(Color::Black);
        passwordInput.setPosition(width / 2 - 100, height / 2 - 150);
        passwordInput.setString(""); // 초기화
    }

    void draw(RenderWindow& window) {
        window.draw(titleText);
        window.draw(passwordPrompt);
        window.draw(passwordInput);
        joinButton.draw(window);
    }

    void handleInput(Event event) {
        if (event.type == Event::TextEntered) {
            if (event.text.unicode == '\b' && !password.empty()) { // 백스페이스 처리
                password.pop_back();
            }
            else if (event.text.unicode < 128 && isalnum(event.text.unicode)) { // 영숫자 입력만 허용
                password += static_cast<char>(event.text.unicode);
            }
            passwordInput.setString(password);
        }
    }

    void resetInput() {
        password.clear();
        passwordInput.setString("");
    }

    bool isJoinButtonClicked(Vector2i mousePos) {
        return joinButton.isClicked(mousePos);
    }

    bool isPasswordEntered() const {
        return !password.empty();
    }

    string getPassword() const {
        return password;
    }

private:
    Text titleText, passwordPrompt, passwordInput;
    Button joinButton;
    string password;
};

// WaitingScreen 클래스 정의
class WaitingScreen {
public:
    WaitingScreen(float width, float height, const Font& font) {
        waitingText.setFont(font);
        waitingText.setString(L"대기 중...");
        waitingText.setCharacterSize(36);
        waitingText.setFillColor(Color::Black);
        waitingText.setPosition(width / 2 - waitingText.getGlobalBounds().width / 2, height / 2);
    }

    void draw(RenderWindow& window) {
        window.draw(waitingText);
    }

private:
    Text waitingText;
};

// CountdownScreen 클래스
class CountdownScreen {
public:
    CountdownScreen(float width, float height, const Font& font)
        : countdownText("10", font, 64) {
        countdownText.setFillColor(Color::Black);
        countdownText.setPosition(width / 2 - countdownText.getGlobalBounds().width / 2, height / 2);
    }

    void update(int secondsLeft) {
        countdownText.setString(to_string(secondsLeft));
    }

    void draw(RenderWindow& window) {
        window.draw(countdownText);
    }

private:
    Text countdownText;
};

class GameScreen {
public:
    GameScreen(float width, float height, const Font& font)
        : turnText("", font, 30), bulletInfo("", font, 24),
        lifeInfo("", font, 24), itemInfo("", font, 24) {

        turnText.setFillColor(Color::Black);
        turnText.setPosition(width / 2 - 50, height / 10);

        bulletInfo.setFillColor(Color::Red);
        bulletInfo.setPosition(10, 10);

        lifeInfo.setFillColor(Color::Green);
        lifeInfo.setPosition(10, 40);

        itemInfo.setFillColor(Color::Blue);
        itemInfo.setPosition(10, 70);
    }

    void update(bool isPlayerTurn, const string& bulletData, const string& lifeData, const string& itemData) {
        turnText.setString(isPlayerTurn ? "Turn: You" : "Turn: River");
        bulletInfo.setString("Bullets: " + bulletData);
        lifeInfo.setString("Lives: " + lifeData);
        itemInfo.setString("Items: " + itemData); // 아이템 정보 추가
    }

    void render(RenderWindow& window) {
        window.draw(turnText);
        window.draw(bulletInfo);
        window.draw(lifeInfo);
        window.draw(itemInfo);  // 아이템 정보 표시
    }

private:
    Text turnText;    // 현재 턴 텍스트
    Text bulletInfo;  // 탄환 정보 텍스트
    Text lifeInfo;    // 목숨 정보 텍스트
    Text itemInfo;    // 아이템 정보 텍스트
};


int main() {
    RenderWindow window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Russian Roulette");
    Font font;

    try {
        if (!font.loadFromFile("assets/Freesentation-5Medium.ttf")) {
            throw runtime_error("폰트를 로드할 수 없습니다. 경로를 확인하세요!");
        }
    }
    catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    // 화면 인스턴스 생성
    TitleScreen titleScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    ChoiceRoomScreen choiceRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    CreateRoomScreen createRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    JoinRoomScreen joinRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    WaitingScreen waitingScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    CountdownScreen countdownScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    GameScreen gameScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);

    // 상태 플래그 초기화
    bool isTitleScreen = true;
    bool isChoiceRoom = false;
    bool isCreateRoom = false;
    bool isJoinRoom = false;
    bool isWaitingScreen = false;
    bool isCountdown = false;
    bool isGameStart = false;

    // 게임 관련 데이터
    int countdownTime = 10;
    string bulletData = "";
    string lifeData = "";
    string itemData = "";
    bool isPlayerTurn = false;

    // WinSock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup 실패" << endl;
        return -1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "소켓 생성 실패" << endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "서버 연결 실패" << endl;
        cleanupClient();
        return -1;
    }

    thread serverListener(listenServer, ref(isWaitingScreen), ref(isGameStart), ref(isCountdown),
        ref(countdownTime), ref(bulletData), ref(isPlayerTurn), ref(lifeData), ref(itemData));
    serverListener.detach();

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                cleanupClient();
                window.close();
            }

            if (isTitleScreen) {
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    if (titleScreen.isStartButtonClicked(Mouse::getPosition(window))) {
                        isTitleScreen = false;
                        isChoiceRoom = true;
                    }
                }
            }
            else if (isChoiceRoom) {
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    Vector2i mousePos = Mouse::getPosition(window);
                    if (choiceRoomScreen.isCreateButtonClicked(mousePos)) {
                        isChoiceRoom = false;
                        isCreateRoom = true;
                    }
                    else if (choiceRoomScreen.isJoinButtonClicked(mousePos)) {
                        isChoiceRoom = false;
                        isJoinRoom = true;
                    }
                }
            }
            else if (isCreateRoom) {
                createRoomScreen.handleInput(event);
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    if (createRoomScreen.isCreateButtonClicked(Mouse::getPosition(window)) && createRoomScreen.isPasswordEntered()) {
                        string password = createRoomScreen.getPassword();
                        send(clientSocket, ("CREATE\n" + password + "\n").c_str(), password.size() + 8, 0);
                        createRoomScreen.resetInput();
                        isCreateRoom = false;
                        isWaitingScreen = true;
                    }
                }
            }
            else if (isJoinRoom) {
                joinRoomScreen.handleInput(event);
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    if (joinRoomScreen.isJoinButtonClicked(Mouse::getPosition(window)) && joinRoomScreen.isPasswordEntered()) {
                        string password = joinRoomScreen.getPassword();
                        send(clientSocket, ("JOIN\n" + password + "\n").c_str(), password.size() + 6, 0);
                        joinRoomScreen.resetInput();
                        isJoinRoom = false;
                        isWaitingScreen = true;
                    }
                }
            }
        }

        window.clear(Color::White);

        if (isTitleScreen) {
            titleScreen.draw(window);
        }
        else if (isChoiceRoom) {
            choiceRoomScreen.draw(window);
        }
        else if (isCreateRoom) {
            createRoomScreen.draw(window);
        }
        else if (isJoinRoom) {
            joinRoomScreen.draw(window);
        }
        else if (isWaitingScreen) {
            waitingScreen.draw(window);
        }
        else if (isCountdown) {
            countdownScreen.update(countdownTime);
            countdownScreen.draw(window);
        }
        else if (isGameStart) {
            gameScreen.update(isPlayerTurn, bulletData, lifeData, itemData);
            gameScreen.render(window);
        }

        window.display();
    }

    cleanupClient();
    return 0;
}
