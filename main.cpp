#include <SFML/Graphics.hpp>
#include <winsock2.h>
#include <ws2tcpip.h> // inet_pton 포함
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib") // WinSock 라이브러리 링크

using namespace sf;
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

SOCKET clientSocket;

// 서버 응답 처리 함수
void listenServer(bool& isWaitingScreen, bool& isGameStart) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Disconnected from server." << endl;
            closesocket(clientSocket);
            return;
        }

        string message(buffer);
        if (message == "WAIT\n") {
            cout << "Waiting for another player..." << endl;
        }
        else if (message == "COUNTDOWN\n") {
            cout << "Game will start in 10 seconds!" << endl;
            isWaitingScreen = true; // 대기 화면 유지
            this_thread::sleep_for(chrono::seconds(10)); // 카운트다운
        }
        else if (message == "START\n") {
            cout << "Game Started!" << endl;
            isGameStart = true; // 게임 시작 플래그
            break;
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

// CreateRoomScreen 클래스
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
                password.pop_back(); // 백스페이스
            }
            else if (isdigit(event.text.unicode)) {
                password += static_cast<char>(event.text.unicode);
            }
            passwordInput.setString(password);
        }
    }

    bool isCreateButtonClicked(Vector2i mousePos) {
        return createButton.isClicked(mousePos);
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

// main 함수
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
    CreateRoomScreen createRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    JoinRoomScreen joinRoomScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);
    WaitingScreen waitingScreen(WINDOW_WIDTH, WINDOW_HEIGHT, font);

    bool isTitleScreen = true, isCreateRoom = false, isJoinRoom = false, isWaitingScreen = false, isGameStart = false;

    // WinSock 초기화
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // 최신 방식으로 IP 주소 변환
    serverAddr.sin_port = htons(12345);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed!" << endl;
        return -1;
    }

    cout << "Connected to server!" << endl;

    thread(listenServer, ref(isWaitingScreen), ref(isGameStart)).detach();

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (isTitleScreen) {
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    Vector2i mousePos = Mouse::getPosition(window);
                    if (titleScreen.isStartButtonClicked(mousePos)) {
                        isTitleScreen = false;
                        isCreateRoom = true;
                    }
                }
            }
            else if (isCreateRoom) {
                if (event.type == Event::TextEntered) {
                    createRoomScreen.handleInput(event);
                }
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    if (createRoomScreen.isCreateButtonClicked(Mouse::getPosition(window))) {
                        string message = "CREATE " + createRoomScreen.getPassword() + "\n";
                        send(clientSocket, message.c_str(), message.size(), 0);
                        isCreateRoom = false;
                        isWaitingScreen = true;
                    }
                }
            }
            else if (isJoinRoom) {
                if (event.type == Event::TextEntered) {
                    joinRoomScreen.handleInput(event);
                }
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    if (joinRoomScreen.isJoinButtonClicked(Mouse::getPosition(window))) {
                        string message = "JOIN " + joinRoomScreen.getPassword() + "\n";
                        send(clientSocket, message.c_str(), message.size(), 0);
                        isJoinRoom = false;
                        isWaitingScreen = true;
                    }
                }
            }

            if (event.type == Event::Closed) {
                window.close();
            }
        }

        window.clear(Color::White);

        if (isTitleScreen) {
            titleScreen.draw(window);
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

        window.display();
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
