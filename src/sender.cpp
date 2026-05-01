#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include "protocol.h"
#include "socket_utils.h"
const size_t CHUNK_SIZE = 64 * 1024;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8082);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        perror("Invalid address or address not supported");
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    std::ifstream file("data.txt", std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file data.txt" << std::endl;
        return -1;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string filename = "data.txt";
    uint32_t name_len = filename.size();

    Header start_header{START, 0, sizeof(file_size) + name_len + sizeof(name_len)}; // Include header size in data_size
    send_all(sock, &start_header, sizeof(start_header));

    send_all(sock, &file_size, sizeof(file_size));
    send_all(sock, &name_len, sizeof(name_len));
    send_all(sock, filename.c_str(), name_len);

    // send(sock, &file_size, sizeof(file_size), MSG_NOSIGNAL);
    // std::cout << "Sending file of size: " << file_size << " bytes" << std::endl;

    char buffer[CHUNK_SIZE];
    uint32_t chunk_id = 0;

    while(!file.eof()) {
        file.read(buffer, CHUNK_SIZE);
        std::streamsize bytes_read = file.gcount();

        if(bytes_read <= 0)
            break;
        Header header{DATA, chunk_id, static_cast<uint32_t>(bytes_read)};
        send_all(sock, &header, sizeof(header));
        send_all(sock, buffer, bytes_read);

        Header ack_header;
        recv_all(sock, &ack_header, sizeof(ack_header));
        if(ack_header.type != ACK || ack_header.chunk_id != chunk_id) {
            std::cerr << "Failed to receive ACK for chunk " << chunk_id << std::endl;
            file.seekg(chunk_id * CHUNK_SIZE, std::ios::beg); // Rewind to resend the chunk
            continue;
        }
        chunk_id++;
    }

    Header end_header{END, chunk_id, 0};
    send_all(sock, &end_header, sizeof(end_header));

    std::cout << "File transfer completed." << std::endl;

    file.close();
    close(sock);

    return 0;
}