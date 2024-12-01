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
    vector<string> items;  // 아이템 상태 관리
    vector<int> bullets;   // 총알 상태: 0 (빈탄), 1 (실탄)
    bool gameStarted = false;
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
    Room* currentRoom = nullptr;

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
            rooms[clientPassword] = { clientPassword, {clientSocket}, {5}, {0}, {}, false };  // 초기화
            send(clientSocket, "WAIT\n", 5, 0);
            cout << "Room created with password: " << clientPassword << endl;
        }
        // 방에 참여
        else if (message.rfind("JOIN", 0) == 0) {
            clientPassword = message.substr(5, 4);

            if (rooms.find(clientPassword) != rooms.end() && rooms[clientPassword].players.size() < 2) {
                rooms[clientPassword].players.push_back(clientSocket);
                rooms[clientPassword].healths.push_back(5);  // 두 번째 플레이어의 체력 초기화
                rooms[clientPassword].turns.push_back(0);  // 첫 번째 턴은 0으로 설정
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
        if (currentRoom != nullptr && currentRoom->gameStarted) {
            if (message.rfind("SHOOT", 0) == 0) {
                int playerIndex = find(currentRoom->players.begin(), currentRoom->players.end(), clientSocket) - currentRoom->players.begin();
                int bulletIndex = currentRoom->turns[playerIndex];

                if (currentRoom->bullets[bulletIndex] == 1) {  // 실탄인 경우
                    currentRoom->healths[1 - playerIndex] -= 1; // 상대방 체력 감소
                    sendToAllInRoom(clientPassword, "SHOT\n");
                    cout << "Player " << playerIndex + 1 << " shot the bullet. Health: " << currentRoom->healths[1 - playerIndex] << endl;
                }
                currentRoom->turns[playerIndex] = (bulletIndex + 1) % 8;  // 턴 순서 변경
            }
            else if (message.rfind("USE_ITEM", 0) == 0) {
                int playerIndex = find(currentRoom->players.begin(), currentRoom->players.end(), clientSocket) - currentRoom->players.begin();
                currentRoom->healths[playerIndex] = min(currentRoom->healths[playerIndex] + 1, 8);  // 회복 아이템 사용
                sendToAllInRoom(clientPassword, "HEALED\n");
                cout << "Player " << playerIndex + 1 << " used a heal item. Health: " << currentRoom->healths[playerIndex] << endl;
            }

            // 게임 종료 처리
            if (currentRoom->healths[0] <= 0 || currentRoom->healths[1] <= 0) {
                string winner = currentRoom->healths[0] > 0 ? "Player 1" : "Player 2";
                sendToAllInRoom(clientPassword, (winner + " WINS!\n").c_str());
                currentRoom->gameStarted = false; // 게임 종료
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
