#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include "protocol.h"
#include "socket_utils.h"
#include "checksum_helper.h"
#include "config_reader.h"
#include <atomic>
#include <csignal>


std::atomic<bool> g_running(true);

void handle_sigint(int) {
    g_running = false;
}

int handle_transfer(int new_socket, const Config &cfg)
{

    std::cout << "Connection accepted from client.\n";

    Header start_header{};
    if (recv_all(new_socket, &start_header, sizeof(start_header)) <= 0)
    {
        std::cerr << "Error receiving start header or connection closed by client.\n";
        return -1;
    }
    if (start_header.type != START)
    {
        std::cerr << "Expected START message, but received type: " << start_header.type << "\n";
        return -1;
    }

    uint32_t name_len;
    size_t file_size;
    if (recv_all(new_socket, &file_size, sizeof(file_size)) <= 0)
    {
        std::cerr << "Error receiving file size or connection closed by client.\n";
        return -1;
    }
    if (recv_all(new_socket, &name_len, sizeof(name_len)) <= 0)
    {
        std::cerr << "Error receiving filename length or connection closed by client.\n";
        return -1;
    }
    char filename_buffer[256];
    if (name_len >= sizeof(filename_buffer))
    {
        std::cerr << "Filename too long: " << name_len << " bytes\n";
        return -1;
    }
    if (recv_all(new_socket, filename_buffer, name_len) <= 0)
    {
        std::cerr << "Error receiving filename or connection closed by client.\n";
        return -1;
    }
    filename_buffer[name_len] = '\0'; // For safety, null-terminate the filename
    std::string filename(filename_buffer);
    filename = "received_" + filename;
    std::string temp_filename = filename + ".tmp";
    std::ofstream output_file(temp_filename, std::ios::binary);
    if (!output_file)
    {
        std::cerr << "Could not open output file: " << temp_filename << "\n";
        output_file.close();
        std::remove(temp_filename.c_str());
        return -1;
    }
    std::cout << "Receiving file: " << temp_filename << " of size: " << file_size << " bytes\n";
    size_t received_bytes = 0;
    while (true)
    {
        Header header{};
        if (recv_all(new_socket, &header, sizeof(header)) <= 0)
        {
            std::cerr << "Error receiving header or connection closed by client.\n";
            output_file.close();
            std::remove(temp_filename.c_str());
            break;
        }
        if (header.type == DATA)
        {
            char data_buffer[cfg.chunk_size];
            if (header.data_size == 0 || header.data_size > cfg.chunk_size)
            {
                std::cerr << "Chunk underflow or overflow detected\n";
                std::remove(temp_filename.c_str());
                output_file.close();
                return -1;
            }
            if (recv_all(new_socket, data_buffer, header.data_size) <= 0)
            {
                std::cerr << "Error receiving data or connection closed by client.\n";
                output_file.close();
                std::remove(temp_filename.c_str());
                break;
            }

            output_file.write(data_buffer, header.data_size);
            received_bytes += header.data_size;

            // Send ACK
            Header ack_header{ACK, header.chunk_id, 0};
            send_all(new_socket, &ack_header, sizeof(ack_header));
        }
        else if (header.type == END)
        {
            std::array<unsigned char, SHA256_DIGEST_LENGTH> recv_hash;
            if (recv_all(new_socket, recv_hash.data(), header.data_size) <= 0)
            {
                std::cerr << "Error receiving hash or connection closed by client.\n";
                std::remove(temp_filename.c_str());
                output_file.close();
                break;
            }
            output_file.close();
            auto computed_hash = compute_sha256(temp_filename);
            if (recv_hash != computed_hash)
            {
                std::remove(temp_filename.c_str());
                std::cerr << "SHA-256 hash mismatch! File may be corrupted.\n";
                break;
            }

            if(received_bytes != file_size)
            {
                std::remove(temp_filename.c_str());
                std::cerr << "File size mismatch! Expected " << file_size << " bytes but received " << received_bytes << " bytes.\n";
                break;
            }

            std::cout << "SHA-256 hash matches. File integrity verified.\n";
            std::cout << "File Size Matched\n";
            std::rename(temp_filename.c_str(), filename.c_str());

            std::cout << "Received END message. File transfer complete.\n";
            Header end_ack{END, 0, 0};
            send_all(new_socket, &end_ack, sizeof(end_ack));
            break;
        }
        else
        {
            output_file.close();
            std::remove(temp_filename.c_str());
            std::cerr << "Received unknown message type: " << header.type << "\n";
        }
    }

    return 0;
}

int main()
{

    std::signal(SIGINT, handle_sigint);

    Config cfg = load_cfg();
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);


    if (server_fd == 0)
    {
        perror("Socket Failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(cfg.port);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
    {
        perror("Bind Failed");
        return -1;
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen error");
        return -1;
    }

    std::cout << "Server listening on port " << cfg.port << "...\n";

    while (g_running)
    {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0)
        {
            perror("Accept error");
            continue;
        }
        int result = handle_transfer(new_socket, cfg);
        if (result < 0)
            std::cerr << "Error during file transfer with client.\n";

        close(new_socket);
    }
    close(server_fd);

    return 0;
}