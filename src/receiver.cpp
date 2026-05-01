#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include "protocol.h"
#include "socket_utils.h"
#include "checksum_helper.h"

const size_t CHUNK_SIZE = 64 * 1024;

int main(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == 0){
        perror("Socket Failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8082);

    if(bind(server_fd, (struct sockaddr*)&address,  addrlen) < 0){
        perror("Bind Failed");
        return -1;
    }

    if(listen(server_fd, 3)<0){
        perror("Listen error");
        return -1;
    }

    std::cout << "Server listening on port 8082...\n";

    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if(new_socket < 0){
        perror("Accept error");
        return -1;
    }

    std::cout << "Connection accepted from client.\n";

    Header start_header{};
    if(recv_all(new_socket, &start_header, sizeof(start_header)) <= 0){
        std::cerr << "Error receiving start header or connection closed by client.\n";
        return -1;
    }
    if(start_header.type != START){
        std::cerr << "Expected START message, but received type: " << start_header.type << "\n";
        return -1;
    }

    uint32_t name_len;
    size_t file_size;
    if(recv_all(new_socket, &file_size, sizeof(file_size)) <= 0){
        std::cerr << "Error receiving file size or connection closed by client.\n";
        return -1;
    }
    if(recv_all(new_socket, &name_len, sizeof(name_len)) <= 0){
        std::cerr << "Error receiving filename length or connection closed by client.\n";
        return -1;
    }
    char filename_buffer[256];
    if(name_len >= sizeof(filename_buffer)){
        std::cerr << "Filename too long: " << name_len << " bytes\n";
        return -1;
    }
    if(recv_all(new_socket, filename_buffer, name_len) <= 0){
        std::cerr << "Error receiving filename or connection closed by client.\n";
        return -1;
    }
    filename_buffer[name_len] = '\0'; //For safety, null-terminate the filename
    std::string filename(filename_buffer);
    filename += "_received"; // Append .received to avoid overwriting existing files
    std::ofstream output_file(filename, std::ios::binary);
    if(!output_file){
        std::cerr << "Could not open output file: " << filename << "\n";
        return -1;
    }
    std::cout << "Receiving file: " << filename << " of size: " << file_size << " bytes\n";

    while(true){
        Header header{};
        if(recv_all(new_socket, &header, sizeof(header)) <= 0){
            std::cerr << "Error receiving header or connection closed by client.\n";
            break;
        }
        if(header.type == DATA){
            char data_buffer[CHUNK_SIZE];
            if(recv_all(new_socket, data_buffer, header.data_size) <= 0){
                std::cerr << "Error receiving data or connection closed by client.\n";
                break;
            }
            // Process the received chunk (e.g., write to file)
           
            output_file.write(data_buffer, header.data_size);
            output_file.close();

            // Send ACK
            Header ack_header{ACK, header.chunk_id, 0};
            send_all(new_socket, &ack_header, sizeof(ack_header));

        } else if(header.type == END){
            std::array<unsigned char, SHA256_DIGEST_LENGTH> recv_hash;
            if(recv_all(new_socket, recv_hash.data(), header.data_size) <= 0){
                std::cerr << "Error receiving hash or connection closed by client.\n";
                break;
            }
            auto computed_hash = compute_sha256(filename);
            if(recv_hash == computed_hash){
                std::cout << "SHA-256 hash matches. File integrity verified.\n";
            } else {
                std::cerr << "SHA-256 hash mismatch! File may be corrupted.\n";
                break;
            }
            std::cout << "Received END message. File transfer complete.\n";
            break;
        } else {
            std::cerr << "Received unknown message type: " << header.type << "\n";
        }
    }

    close(new_socket);
    close(server_fd);

    return 0;
}