#include <winsock2.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib") // WinSock 라이브러리 링크

using namespace std;

struct Room {
    string password;
    vector<SOCKET> players;
    vector<int> healths;   // 플레이어의 체력을 저장
    vector<int> turns;     // 턴 순서를 관리
    vector<int> bullets;   // 총알 상태: 0 (빈탄), 1 (실탄)
    bool gameStarted;

    // 기본 생성자 추가
    Room() : password(""), gameStarted(false) {}

    // 생성자 추가
    Room(const string& pwd)
        : password(pwd), gameStarted(false) {
        players.clear();
        healths = { 5, 5 };    // 두 플레이어의 기본 체력
        turns = { 0, 0 };      // 각 플레이어의 턴 초기화
        bullets.resize(8, 0);  // 기본적으로 8개의 빈탄으로 초기화
    }
};

map<string, Room> rooms;

void sendToAllInRoom(const string& password, const string& message) {
    for (SOCKET player : rooms[password].players) {
        send(player, message.c_str(), message.length(), 0);
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    string clientPassword;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Client disconnected." << endl;
            closesocket(clientSocket);
            return;
        }

        string message(buffer);

        // 방 생성
        if (message.rfind("CREATE", 0) == 0) {
            clientPassword = message.substr(7, 4);
            rooms[clientPassword] = Room(clientPassword);  // Room 생성자를 사용
            rooms[clientPassword].players.push_back(clientSocket); // 플레이어 추가
            send(clientSocket, "WAIT\n", 5, 0);
            cout << "Room created with password: " << clientPassword << endl;
        }
        // 방에 참여
        else if (message.rfind("JOIN", 0) == 0) {
            clientPassword = message.substr(5, 4);

            if (rooms.find(clientPassword) != rooms.end() && rooms[clientPassword].players.size() < 2) {
                rooms[clientPassword].players.push_back(clientSocket);
                send(clientSocket, "WAIT\n", 5, 0);

                if (rooms[clientPassword].players.size() == 2) {
                    // 두 명이 대기방에 들어오면 카운트다운 시작
                    for (SOCKET player : rooms[clientPassword].players) {
                        send(player, "COUNTDOWN\n", 11, 0);
                    }

                    // 카운트다운 10초
                    this_thread::sleep_for(chrono::seconds(10));

                    // 10초 후 "START" 메시지 전송
                    sendToAllInRoom(clientPassword, "START\n");
                    cout << "Game started in room: " << clientPassword << endl;
                    rooms[clientPassword].gameStarted = true;
                }
            }
            else {
                send(clientSocket, "FULL\n", 5, 0); // 방이 꽉 찼으면 거부
            }
        }

        // 게임 진행 처리
        if (rooms[clientPassword].gameStarted) {
            if (message.rfind("SHOOT", 0) == 0) {
                int playerIndex = find(rooms[clientPassword].players.begin(), rooms[clientPassword].players.end(), clientSocket) - rooms[clientPassword].players.begin();
                int bulletIndex = rooms[clientPassword].turns[playerIndex];

                if (rooms[clientPassword].bullets[bulletIndex] == 1) {  // 실탄인 경우
                    rooms[clientPassword].healths[1 - playerIndex] -= 1; // 상대방 체력 감소
                    sendToAllInRoom(clientPassword, "SHOT\n");
                    cout << "Player " << playerIndex + 1 << " shot the bullet. Health: " << rooms[clientPassword].healths[1 - playerIndex] << endl;
                }
                rooms[clientPassword].turns[playerIndex] = (bulletIndex + 1) % 8;  // 턴 순서 변경
            }
            else if (message.rfind("USE_ITEM", 0) == 0) {
                int playerIndex = find(rooms[clientPassword].players.begin(), rooms[clientPassword].players.end(), clientSocket) - rooms[clientPassword].players.begin();
                rooms[clientPassword].healths[playerIndex] = min(rooms[clientPassword].healths[playerIndex] + 1, 8);  // 회복 아이템 사용
                sendToAllInRoom(clientPassword, "HEALED\n");
                cout << "Player " << playerIndex + 1 << " used a heal item. Health: " << rooms[clientPassword].healths[playerIndex] << endl;
            }

            // 게임 종료 처리
            if (rooms[clientPassword].healths[0] <= 0 || rooms[clientPassword].healths[1] <= 0) {
                string winner = rooms[clientPassword].healths[0] > 0 ? "Player 1" : "Player 2";
                sendToAllInRoom(clientPassword, (winner + " WINS!\n").c_str());
                rooms[clientPassword].gameStarted = false; // 게임 종료
                break;  // 종료 후 클라이언트와의 연결 종료
            }
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    cout << "Server is listening on port 12345..." << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        cout << "Client connected." << endl;
        thread(handleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}