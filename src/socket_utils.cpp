#include "socket_utils.h"
#include <unistd.h>
#include <sys/socket.h>

ssize_t send_all(int sock, const void *data, size_t len)
{
    size_t total_sent = 0;
    const char *ptr = static_cast<const char *>(data);
    while (total_sent < len)
    {
        ssize_t sent = send(sock, ptr + total_sent, len - total_sent, MSG_NOSIGNAL);
        if (sent <= 0)
            return sent;
        total_sent += sent;
    }
    return total_sent;
}

ssize_t recv_all(int sock, void *buffer, size_t len)
{
    size_t total_received = 0;
    char *ptr = static_cast<char *>(buffer);
    while (total_received < len)
    {
        ssize_t received = recv(sock, ptr + total_received, len - total_received, 0);
        if (received <= 0)
            return received;
        total_received += received;
    }
    return total_received;
}