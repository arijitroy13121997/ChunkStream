#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
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
    serv_addr.sin_port = htons(8080);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        perror("Invalid address or address not supported");
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // const char *message = "Hello from client!";
    // send(sock, message, strlen(message), 0);
    // std::cout << "Message sent to server: " << message << std::endl;

    std::ifstream file("data.txt", std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file data.txt" << std::endl;
        return -1;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    send(sock, &file_size, sizeof(file_size), MSG_NOSIGNAL);
    std::cout << "Sending file of size: " << file_size << " bytes" << std::endl;

    char buffer[CHUNK_SIZE];
    while(!file.eof()) {
        file.read(buffer, CHUNK_SIZE);
        std::streamsize bytes_read = file.gcount();

        if(bytes_read > 0)
            send(sock, buffer, bytes_read, MSG_NOSIGNAL);
    }
    std::cout << "Number of chunks sent: " << (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE << std::endl;
    std::cout << "File data sent to server in chunks." << std::endl;

    file.close();
    close(sock);

    return 0;
}