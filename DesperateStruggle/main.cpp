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

SOCKET clientSocket;

// 클라이언트 종료 처리 함수
void cleanupClient() {
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        WSACleanup();
        cout << "Client shut down and cleaned up." << endl;
    }
}

// 서버 응답 처리 함수
void listenServer(bool& isWaitingScreen, bool& isGameStart, bool& isCountdown, int& countdownTime, Game& game) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Disconnected from server." << endl;
            cleanupClient();
            return;
        }

        string message(buffer);
        if (message == "WAIT\n") {
            cout << "Waiting for another player..." << endl;
            isWaitingScreen = true;
        }
        else if (message == "COUNTDOWN\n") {
            cout << "Game will start in 10 seconds!" << endl;
            isCountdown = true;
            countdownTime = 10;
        }
        else if (message == "START\n") {
            cout << "Game Started!" << endl;
            isGameStart = true;
            break;
        }
        else if (message.rfind("SHOT\n", 0) == 0) {
            game.processShot(message);
        }
        else if (message.rfind("HEALED\n", 0) == 0) {
            game.processHeal(message);
        }
    }
}

// 버튼 클래스
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

// ChoiceRoomScreen 클래스
class ChoiceRoomScreen {
public:
    ChoiceRoomScreen(float width, float height, const Font& font)
        : createButton(L"게임 생성", font, width / 2 - 150, height / 2 - 50, 300, 50),
        joinButton(L"게임 참여", font, width / 2 - 150, height / 2 + 50, 300, 50) {
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

// TitleScreen 클래스
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

// CreateRoomScreen 클래스 수정
class CreateRoomScreen {
public:
    CreateRoomScreen(float width, float height, const Font& font)
        : createButton(L"게임 생성", font, width / 2 - 100, height / 2 + 100, 200, 50) {
        titleText.setFont(font);
        titleText.setString(L"게임 생성");
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
            if (event.text.unicode == '\b' && !password.empty()) {
                password.pop_back(); // 백스페이스 처리
            }
            else if (isalnum(event.text.unicode)) { // 영숫자 입력만 허용
                password += static_cast<char>(event.text.unicode);
            }
            passwordInput.setString(password);
        }
    }

    bool isCreateButtonClicked(Vector2i mousePos) {
        return createButton.isClicked(mousePos);
    }

    bool isPasswordEntered() const {
        return !password.empty(); // 비밀번호가 입력되었는지 확인
    }

    string getPassword() const {
        return password;
    }

private:
    Text titleText, passwordPrompt, passwordInput;
    Button createButton;
    string password;
};

// JoinRoomScreen 클래스
class JoinRoomScreen {
public:
    JoinRoomScreen(float width, float height, const Font& font)
        : joinButton(L"게임 참여하기", font, width / 2 - 100, height / 2 + 100, 200, 50) {
        titleText.setFont(font);
        titleText.setString(L"게임 참여");
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
    }

    void draw(RenderWindow& window) {
        window.draw(titleText);
        window.draw(passwordPrompt);
        window.draw(passwordInput);
        joinButton.draw(window);
    }

    void handleInput(Event event) {
        if (event.type == Event::TextEntered) {
            if (event.text.unicode == '\b' && !password.empty()) {
                password.pop_back(); // 백스페이스
            }
            else if (isdigit(event.text.unicode)) {
                password += static_cast<char>(event.text.unicode);
            }
            passwordInput.setString(password);
        }
    }

    bool isJoinButtonClicked(Vector2i mousePos) {
        return joinButton.isClicked(mousePos);
    }

    string getPassword() const {
        return password;
    }

private:
    Text titleText, passwordPrompt, passwordInput;
    Button joinButton;
    string password;
};

// WaitingScreen 클래스
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

    TitleScreen titleScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    ChoiceRoomScreen choiceRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    CreateRoomScreen createRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    JoinRoomScreen joinRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    WaitingScreen waitingScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    CountdownScreen countdownScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    Game game;

    bool isTitleScreen = true;
    bool isChoiceRoom = false;
    bool isCreateRoom = false;
    bool isJoinRoom = false;
    bool isWaitingScreen = false;
    bool isCountdown = false;
    bool isGameStart = false;
    int countdownTime = 0;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed!" << endl;
        return -1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Error creating socket!" << endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "Failed to connect to server!" << endl;
        cleanupClient();
        return -1;
    }

    thread serverListener(listenServer, ref(isWaitingScreen), ref(isGameStart), ref(isCountdown), ref(countdownTime), ref(game));

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            if (event.type == Event::MouseButtonPressed) {
                Vector2i mousePos = Mouse::getPosition(window);

                if (isTitleScreen) {
                    if (titleScreen.isStartButtonClicked(mousePos)) {
                        isTitleScreen = false;
                        isChoiceRoom = true;
                    }
                }
                else if (isChoiceRoom) {
                    if (choiceRoomScreen.isCreateButtonClicked(mousePos)) {
                        isChoiceRoom = false;
                        isCreateRoom = true;
                    }
                    else if (choiceRoomScreen.isJoinButtonClicked(mousePos)) {
                        isChoiceRoom = false;
                        isJoinRoom = true;
                    }
                }
                else if (isCreateRoom) {
                    createRoomScreen.handleInput(event);
                    if (createRoomScreen.isCreateButtonClicked(mousePos)) {
                        if (createRoomScreen.isPasswordEntered()) { // 비밀번호가 입력되었는지 확인
                            string message = "CREATE " + createRoomScreen.getPassword() + "\n";
                            send(clientSocket, message.c_str(), message.size(), 0);
                            isCreateRoom = false;
                            isWaitingScreen = true;
                        }
                        else {
                            cout << "비밀번호를 입력하세요!" << endl; // 디버그 메시지
                        }
                    }
                }
                else if (isJoinRoom) {
                    joinRoomScreen.handleInput(event);
                    if (joinRoomScreen.isJoinButtonClicked(mousePos)) {
                        if (!joinRoomScreen.getPassword().empty()) { // 비밀번호가 입력되었는지 확인
                            string message = "JOIN " + joinRoomScreen.getPassword() + "\n";
                            send(clientSocket, message.c_str(), message.size(), 0);
                            isJoinRoom = false;
                            isWaitingScreen = true;
                        }
                        else {
                            cout << "비밀번호를 입력하세요!" << endl; // 디버그 메시지
                        }
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
            game.run();
        }

        window.display();
    }

    serverListener.join();
    cleanupClient();
    return 0;
}
