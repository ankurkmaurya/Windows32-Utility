
#include <winsock2.h> 
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <iostream>
#include <ping.h>


#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")



void Ping::checkIPConnectivity(const char* host) {

    HANDLE hIcmpFile;
    DWORD dwRetVal = 0;
    char SendData[] = "Data Buffer";
    BYTE ReplyBuffer[1024];

    // Convert IP string → DWORD using InetPtonA
    in_addr addr;
    if (InetPtonA(AF_INET, host, &addr) != 1) {
        std::cerr << "Error : Invalid IP address format" << std::endl;
        return;
    }
    DWORD ip = addr.S_un.S_addr;

    hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error : Unable to open ICMP handle." << std::endl;
        return;
    }

    dwRetVal = IcmpSendEcho(
        hIcmpFile,
        ip,
        SendData,
        sizeof(SendData),
        nullptr,
        ReplyBuffer,
        sizeof(ReplyBuffer),
        1000
    );

    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
        std::cout << "Reply from " << host
            << " bytes=" << pEchoReply->DataSize
            << " time=" << pEchoReply->RoundTripTime << "ms"
            << " TTL=" << (int)pEchoReply->Options.Ttl
            << std::endl;
    } 
    else {
        std::cout << "Error : Request timed out." << std::endl;
    }

    IcmpCloseHandle(hIcmpFile);
    return;
}


