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
    int turn = 0;  // ���� ���� (0: �÷��̾� 1, 1: �÷��̾� 2)
    vector<int> playerLives = { 5, 5 };  // �� �÷��̾��� ��� (5�� �ʱ�ȭ)
    vector<string> playerItems[2];  // �� �÷��̾ ȹ���� ������
    vector<string> bullets = { "empty", "bullet", "empty", "bullet", "empty", "bullet", "bullet", "empty" };  // �Ѿ� ���� (empty: ��ź, bullet: ��ź)
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

                    // �� ���� �÷��̾ ��� �����ϸ� ī��Ʈ�ٿ� ����
                    if (rooms[password].players.size() == 2) {
                        // ī��Ʈ�ٿ��� �����ϴ� �޽��� ����
                        for (SOCKET player : rooms[password].players) {
                            send(player, "COUNTDOWN\n", 11, 0);
                        }

                        // 10�� ī��Ʈ�ٿ�
                        this_thread::sleep_for(chrono::seconds(10));

                        // ī��Ʈ�ٿ� ���� �� ���� ���� �޽��� ����
                        for (SOCKET player : rooms[password].players) {
                            send(player, "START\n", 6, 0);
                        }

                        // ���� ����
                        while (true) {
                            Room& room = rooms[password];

                            // ���� �Ͽ� �´� �÷��̾�� �� ���� ����
                            SOCKET currentPlayerSocket = room.players[room.turn];
                            SOCKET otherPlayerSocket = room.players[1 - room.turn];

                            send(currentPlayerSocket, "YOUR_TURN\n", 10, 0);  // �ڽ��� ��
                            send(otherPlayerSocket, "OPPONENT_TURN\n", 14, 0);  // ������ ��

                            // �Ѿ��� �Ҹ��ϰ� ���� �ѱ� ��
                            this_thread::sleep_for(chrono::seconds(5));  // 5�� ��� (�� ���� ���� �ð��� ����)

                            // �� ����
                            room.turn = 1 - room.turn;

                            // �÷��̾��� ����� Ȯ���Ͽ�, ����� 0�̸� ���� ����
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
        clientThread.detach(); // Ŭ���̾�Ʈ�� ���� ������ ����
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
