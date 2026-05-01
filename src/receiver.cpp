#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
const size_t CHUNK_SIZE = 64;

int main(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    size_t file_size;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == 0){
        perror("Socket Failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if(bind(server_fd, (struct sockaddr*)&address,  addrlen) < 0){
        perror("Bind Failed");
        return -1;
    }

    if(listen(server_fd, 3)<0){
        perror("Listen error");
        return -1;
    }

    std::cout << "Server listening on port 8080...\n";

    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if(new_socket < 0){
        perror("Accept error");
        return -1;
    }

    std::cout << "Connection accepted from client.\n";

    // char buffer[1024] = {0};
    // read(new_socket, buffer, 1024);
    // std::cout << "Received message: " << buffer << std::endl;

    read(new_socket, &file_size, sizeof(file_size));

    std::ofstream output_file("received_data.txt", std::ios::binary);

    char buffer[CHUNK_SIZE];

    size_t total_bytes_read = 0;
    while (total_bytes_read < file_size) {
        int bytes_read = read(new_socket, buffer, std::min(CHUNK_SIZE, file_size - total_bytes_read));
        if(bytes_read <= 0) {
            std::cerr << "Error reading from socket or connection closed by client.\n";
            break;
        }
        output_file.write(buffer, bytes_read);
        total_bytes_read += bytes_read;
    }

    output_file.close();
    std::cout << "File received and saved as received_data.txt\n";

    close(new_socket);
    close(server_fd);

    return 0;
}