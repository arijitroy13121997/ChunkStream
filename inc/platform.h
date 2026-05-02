#pragma once

#include <iostream>
#include <fstream>
#include <cstring>
#include <memory>

#include "protocol.h"
#include "socket_utils.h"
#include "checksum_helper.h"
#include "config_reader.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    #ifndef _SSIZE_T_DEFINED
    typedef int ssize_t;
    #endif
    typedef int socklen_t;

    #define CLOSE_SOCKET closesocket
    #define GET_ERROR WSAGetLastError()

    inline void init_sockets() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
    }

    inline void cleanup_sockets() {
        WSACleanup();
    }

    #define SEND_FLAGS 0   // no MSG_NOSIGNAL

#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    #define CLOSE_SOCKET close
    #define GET_ERROR errno
    #define SEND_FLAGS MSG_NOSIGNAL

    inline void init_sockets() {}
    inline void cleanup_sockets() {}

#endif