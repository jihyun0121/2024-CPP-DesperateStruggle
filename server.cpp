#include <winsock2.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib") // WinSock 라이브러리 링크

using namespace std;

struct Room {
    string password;
    vector<SOCKET> players;
};

map<string, Room> rooms;

void handleClient(SOCKET clientSocket) {
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
            rooms[password] = { password, {clientSocket} };
            send(clientSocket, "WAIT\n", 5, 0);
            cout << "Room created with password: " << password << endl;
        }
        else if (message.rfind("JOIN", 0) == 0) {
            string password = message.substr(5, 4);

            if (rooms.find(password) != rooms.end() && rooms[password].players.size() < 2) {
                rooms[password].players.push_back(clientSocket);
                send(clientSocket, "WAIT\n", 5, 0);

                if (rooms[password].players.size() == 2) {
                    for (SOCKET player : rooms[password].players) {
                        send(player, "COUNTDOWN\n", 11, 0);
                    }
                    this_thread::sleep_for(chrono::seconds(10));
                    for (SOCKET player : rooms[password].players) {
                        send(player, "START\n", 6, 0);
                    }
                }
            }
            else {
                send(clientSocket, "FULL\n", 5, 0);
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
