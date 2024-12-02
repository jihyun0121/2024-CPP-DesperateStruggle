#include <winsock2.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

struct Room {
    string password;
    vector<SOCKET> players;
    int turn = 0;  // 턴을 관리 (0: 플레이어 1, 1: 플레이어 2)
    vector<int> playerLives = { 5, 5 };  // 각 플레이어의 목숨 (5로 초기화)
    vector<string> playerItems[2];  // 각 플레이어가 획득한 아이템
    vector<string> bullets = { "empty", "bullet", "empty", "bullet", "empty", "bullet", "bullet", "empty" };  // 총알 상태 (empty: 빈탄, bullet: 실탄)
};

map<string, Room> rooms;

void handleClient(SOCKET clientSocket) {
    try {
        char buffer[1024];
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (bytesReceived <= 0) {
                cout << "Client disconnected." << endl;
                closesocket(clientSocket);
                return;
            }

            string message(buffer);

            if (message.rfind("CREATE", 0) == 0) {
                string password = message.substr(7, 4);
                if (rooms.find(password) == rooms.end()) {
                    rooms[password] = { password, {clientSocket} };
                    send(clientSocket, "WAIT\n", 5, 0);
                    cout << "Room created: " << password << endl;
                }
                else {
                    send(clientSocket, "EXIST\n", 6, 0);
                }
            }
            else if (message.rfind("JOIN", 0) == 0) {
                string password = message.substr(5, 4);
                if (rooms.find(password) != rooms.end() && rooms[password].players.size() < 2) {
                    rooms[password].players.push_back(clientSocket);
                    send(clientSocket, "WAIT\n", 5, 0);

                    // 두 명의 플레이어가 모두 입장하면 카운트다운 시작
                    if (rooms[password].players.size() == 2) {
                        // 카운트다운을 시작하는 메시지 전송
                        for (SOCKET player : rooms[password].players) {
                            send(player, "COUNTDOWN\n", 11, 0);
                        }

                        // 10초 카운트다운
                        this_thread::sleep_for(chrono::seconds(10));

                        // 카운트다운 종료 후 게임 시작 메시지 전송
                        for (SOCKET player : rooms[password].players) {
                            send(player, "START\n", 6, 0);
                        }

                        // 게임 진행
                        while (true) {
                            Room& room = rooms[password];

                            // 현재 턴에 맞는 플레이어에게 턴 정보 전송
                            SOCKET currentPlayerSocket = room.players[room.turn];
                            SOCKET otherPlayerSocket = room.players[1 - room.turn];

                            send(currentPlayerSocket, "YOUR_TURN\n", 10, 0);  // 자신의 턴
                            send(otherPlayerSocket, "OPPONENT_TURN\n", 14, 0);  // 상대방의 턴

                            // 총알을 소모하고 턴을 넘길 때
                            this_thread::sleep_for(chrono::seconds(5));  // 5초 대기 (각 턴의 진행 시간을 설정)

                            // 턴 변경
                            room.turn = 1 - room.turn;

                            // 플레이어의 목숨을 확인하여, 목숨이 0이면 게임 종료
                            if (room.playerLives[0] == 0 || room.playerLives[1] == 0) {
                                string result = (room.playerLives[0] == 0) ? "Player 2 wins!" : "Player 1 wins!";
                                for (SOCKET player : room.players) {
                                    send(player, result.c_str(), result.length(), 0);
                                }
                                break;
                            }
                        }
                    }
                }
                else {
                    send(clientSocket, "FULL\n", 5, 0);
                }
            }
        }
    }
    catch (const exception& e) {
        cerr << "Error in client handler: " << e.what() << endl;
        closesocket(clientSocket);
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed!" << endl;
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Error creating socket!" << endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    cout << "Server is running on port 12345..." << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Failed to accept connection!" << endl;
            continue;
        }

        cout << "Client connected." << endl;
        thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // 클라이언트별 독립 스레드 실행
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
