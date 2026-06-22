
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <telnet.h>

#pragma comment(lib, "ws2_32.lib")


void Telnet::checkIPAndPortConnectivity(const char* host, const char* port) {
    // Initialize WinSock
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "Error : WSAStartup Failed - " << wsaResult << std::endl;
        return;
    }

    // Resolve host and port
    addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    int res = getaddrinfo(host, port, &hints, &result);
    if (res != 0) {
        std::cerr << "Error : Getaddrinfo Failed - " << gai_strerrorA(res) << std::endl;
        WSACleanup();
        return;
    }

    // Create socket
    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error : Socket Creation Failed - " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return;
    }


    std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
    // Attempt connection
    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Error : Connection Failed - " << WSAGetLastError() << std::endl;
        closesocket(sock);
        freeaddrinfo(result);
        WSACleanup();
        return;
    }
    std::cout << "Connection Successful" << std::endl;


    // Cleanup
    closesocket(sock);
    freeaddrinfo(result);
    WSACleanup();
    return;
}




