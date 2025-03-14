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
void listenServer(bool& isWaitingScreen, bool& isGameStart, bool& isCountdown, int& countdownTime, string& bulletData, bool& isPlayerTurn) {
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
        : turnText("", font, 30), bulletInfo("", font, 24) {
        turnText.setFillColor(Color::Black);
        turnText.setPosition(width / 2 - 50, height / 10);

        bulletInfo.setFillColor(Color::Red);
        bulletInfo.setPosition(10, 10);
    }

    // 업데이트 함수 정의 (인수: bool, const string&)
    void update(bool isPlayerTurn, const string& bulletData) {
        turnText.setString(isPlayerTurn ? "Turn: You" : "Turn: River");
        bulletInfo.setString("Bullets: " + bulletData);
    }

    void render(RenderWindow& window) {
        window.draw(turnText);
        window.draw(bulletInfo);
    }

private:
    Text turnText;   // 현재 턴 텍스트
    Text bulletInfo; // 탄환 정보 텍스트
};

// int main() 함수 정의
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
    GameScreen gameScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);

    // 상태 플래그 초기화
    bool isTitleScreen = true;
    bool isChoiceRoom = false;
    bool isCreateRoom = false;
    bool isJoinRoom = false;
    bool isWaitingScreen = false;
    bool isCountdown = false;
    bool isGameStart = false;
    bool isPlayerTurn = true; // 초기 플레이어 턴 여부
    string bulletData = "Empty: 3, Live: 2"; // 초기 탄환 데이터
    int countdownTime = 0;

    // WinSock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup 실패!" << endl;
        return -1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "소켓 생성 실패!" << endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "서버 연결 실패!" << endl;
        cleanupClient();
        return -1;
    }

    // 서버 응답 수신 쓰레드 실행
    thread serverListener(listenServer, ref(isWaitingScreen), ref(isGameStart), ref(isCountdown), ref(countdownTime), ref(bulletData), ref(isPlayerTurn));

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            Vector2i mousePos = Mouse::getPosition(window);

            // 키보드 입력 처리
            if (isCreateRoom) {
                createRoomScreen.handleInput(event); // CreateRoomScreen에 키보드 입력 전달
            }
            else if (isJoinRoom) {
                joinRoomScreen.handleInput(event); // JoinRoomScreen에 키보드 입력 전달
            }

            // 마우스 클릭 이벤트 처리
            if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                if (isTitleScreen && titleScreen.isStartButtonClicked(mousePos)) {
                    isTitleScreen = false;
                    isChoiceRoom = true;

                    // 화면 전환 시 입력 필드 초기화
                    createRoomScreen.resetInput();
                    joinRoomScreen.resetInput();
                }
                else if (isChoiceRoom) {
                    if (choiceRoomScreen.isCreateButtonClicked(mousePos)) {
                        isChoiceRoom = false;
                        isCreateRoom = true;

                        // 화면 전환 시 입력 필드 초기화
                        createRoomScreen.resetInput();
                    }
                    else if (choiceRoomScreen.isJoinButtonClicked(mousePos)) {
                        isChoiceRoom = false;
                        isJoinRoom = true;

                        // 화면 전환 시 입력 필드 초기화
                        joinRoomScreen.resetInput();
                    }
                }
                else if (isCreateRoom) {
                    if (createRoomScreen.isCreateButtonClicked(mousePos) && createRoomScreen.isPasswordEntered()) {
                        string message = "CREATE " + createRoomScreen.getPassword() + "\n";
                        send(clientSocket, message.c_str(), message.size(), 0);
                        isCreateRoom = false;
                        isWaitingScreen = true;
                    }
                }
                else if (isJoinRoom) {
                    if (joinRoomScreen.isJoinButtonClicked(mousePos) && joinRoomScreen.isPasswordEntered()) {
                        string message = "JOIN " + joinRoomScreen.getPassword() + "\n";
                        send(clientSocket, message.c_str(), message.size(), 0);

                        // 서버 응답 대기 및 처리
                        char buffer[1024];
                        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

                        if (bytesReceived > 0) {
                            string serverMessage(buffer);
                            if (serverMessage == "WAIT\n") {
                                // 대기 화면으로 전환
                                isJoinRoom = false;
                                isWaitingScreen = true;
                            }
                            else if (serverMessage == "FULL\n") {
                                // 방이 꽉 찼을 때 처리
                                cout << "방이 꽉 찼습니다. 다시 시도하세요." << endl;
                            }
                            else {
                                cout << "알 수 없는 서버 응답: " << serverMessage << endl;
                            }
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
            gameScreen.update(isPlayerTurn, bulletData); // 올바른 인수 전달
            gameScreen.render(window);
        }

        window.display();
    }


    serverListener.join();
    cleanupClient();
    return 0;
}