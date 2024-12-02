#include <winsock2.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib") // WinSock ���̺귯�� ��ũ

using namespace std;

struct Room {
    string password;
    vector<SOCKET> players;
    vector<int> healths;   // �÷��̾��� ü���� ����
    vector<int> turns;     // �� ������ ����
    vector<int> bullets;   // �Ѿ� ����: 0 (��ź), 1 (��ź)
    bool gameStarted;

    // �⺻ ������ �߰�
    Room() : password(""), gameStarted(false) {}

    // ������ �߰�
    Room(const string& pwd)
        : password(pwd), gameStarted(false) {
        players.clear();
        healths = { 5, 5 };    // �� �÷��̾��� �⺻ ü��
        turns = { 0, 0 };      // �� �÷��̾��� �� �ʱ�ȭ
        bullets.resize(8, 0);  // �⺻������ 8���� ��ź���� �ʱ�ȭ
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

        // �� ����
        if (message.rfind("CREATE", 0) == 0) {
            clientPassword = message.substr(7, 4);
            rooms[clientPassword] = Room(clientPassword);  // Room �����ڸ� ���
            rooms[clientPassword].players.push_back(clientSocket); // �÷��̾� �߰�
            send(clientSocket, "WAIT\n", 5, 0);
            cout << "Room created with password: " << clientPassword << endl;
        }
        // �濡 ����
        else if (message.rfind("JOIN", 0) == 0) {
            clientPassword = message.substr(5, 4);

            if (rooms.find(clientPassword) != rooms.end() && rooms[clientPassword].players.size() < 2) {
                rooms[clientPassword].players.push_back(clientSocket);
                send(clientSocket, "WAIT\n", 5, 0);

                if (rooms[clientPassword].players.size() == 2) {
                    // �� ���� ���濡 ������ ī��Ʈ�ٿ� ����
                    for (SOCKET player : rooms[clientPassword].players) {
                        send(player, "COUNTDOWN\n", 11, 0);
                    }

                    // ī��Ʈ�ٿ� 10��
                    this_thread::sleep_for(chrono::seconds(10));

                    // 10�� �� "START" �޽��� ����
                    sendToAllInRoom(clientPassword, "START\n");
                    cout << "Game started in room: " << clientPassword << endl;
                    rooms[clientPassword].gameStarted = true;
                }
            }
            else {
                send(clientSocket, "FULL\n", 5, 0); // ���� �� á���� �ź�
            }
        }

        // ���� ���� ó��
        if (rooms[clientPassword].gameStarted) {
            if (message.rfind("SHOOT", 0) == 0) {
                int playerIndex = find(rooms[clientPassword].players.begin(), rooms[clientPassword].players.end(), clientSocket) - rooms[clientPassword].players.begin();
                int bulletIndex = rooms[clientPassword].turns[playerIndex];

                if (rooms[clientPassword].bullets[bulletIndex] == 1) {  // ��ź�� ���
                    rooms[clientPassword].healths[1 - playerIndex] -= 1; // ���� ü�� ����
                    sendToAllInRoom(clientPassword, "SHOT\n");
                    cout << "Player " << playerIndex + 1 << " shot the bullet. Health: " << rooms[clientPassword].healths[1 - playerIndex] << endl;
                }
                rooms[clientPassword].turns[playerIndex] = (bulletIndex + 1) % 8;  // �� ���� ����
            }
            else if (message.rfind("USE_ITEM", 0) == 0) {
                int playerIndex = find(rooms[clientPassword].players.begin(), rooms[clientPassword].players.end(), clientSocket) - rooms[clientPassword].players.begin();
                rooms[clientPassword].healths[playerIndex] = min(rooms[clientPassword].healths[playerIndex] + 1, 8);  // ȸ�� ������ ���
                sendToAllInRoom(clientPassword, "HEALED\n");
                cout << "Player " << playerIndex + 1 << " used a heal item. Health: " << rooms[clientPassword].healths[playerIndex] << endl;
            }

            // ���� ���� ó��
            if (rooms[clientPassword].healths[0] <= 0 || rooms[clientPassword].healths[1] <= 0) {
                string winner = rooms[clientPassword].healths[0] > 0 ? "Player 1" : "Player 2";
                sendToAllInRoom(clientPassword, (winner + " WINS!\n").c_str());
                rooms[clientPassword].gameStarted = false; // ���� ����
                break;  // ���� �� Ŭ���̾�Ʈ���� ���� ����
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