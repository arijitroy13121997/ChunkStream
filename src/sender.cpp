#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include "protocol.h"
#include "socket_utils.h"
#include "checksum_helper.h"
#include "config_reader.h"
#include <filesystem>

int main(int argc, char *argv[])
{
    Config cfg = load_cfg();

    if (argc > 1)
    {
        cfg.file = argv[1];
    }

    std::string filename = std::filesystem::path(cfg.file).filename().string();
    std::cout << "Using file: " << filename << "\n";

    int sock = 0;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(cfg.port);

    if (inet_pton(AF_INET, cfg.ip.c_str(), &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address or address not supported");
        return -1;
    }
    std::cout << "Connecting to " << cfg.ip << ":" << cfg.port << "...\n";

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        return -1;
    }

    std::ifstream file(cfg.file, std::ios::binary);
    if (!file)
    {
        std::cerr << "Could not open file " << cfg.file << std::endl;
        return -1;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    uint32_t name_len = filename.size();

    uint32_t meta_size = sizeof(file_size) + sizeof(name_len) + name_len;
    Header start_header{START, 0, meta_size};
    send_all(sock, &start_header, sizeof(start_header));

    send_all(sock, &file_size, sizeof(file_size));
    send_all(sock, &name_len, sizeof(name_len));
    send_all(sock, filename.c_str(), name_len);

    // send(sock, &file_size, sizeof(file_size), MSG_NOSIGNAL);
    // std::cout << "Sending file of size: " << file_size << " bytes" << std::endl;

    char buffer[cfg.chunk_size];
    uint32_t chunk_id = 0;

    while (true)
    {
        file.read(buffer, cfg.chunk_size);
        std::streamsize bytes_read = file.gcount();
        if(bytes_read <= 0)
            break;

        bool success = false;
        int retries = 0;
        while (!success && retries < 3)
        {
            Header header{DATA, chunk_id, static_cast<uint32_t>(bytes_read)};
            if (send_all(sock, &header, sizeof(header)) < 0 || 
                send_all(sock, buffer, bytes_read) < 0)
            {
                retries++;
                std::cout << "Failed to send chunk " << chunk_id << ", retrying (" << retries << "/3)..." << std::endl;
                continue;
            }

            Header ack_header;
            if(recv_all(sock, &ack_header, sizeof(ack_header)) < 0 || 
                ack_header.type != ACK || 
                ack_header.chunk_id != chunk_id)
            {
                retries++;
                std::cout << "Failed to receive ACK for chunk " << chunk_id << ", retrying (" << retries << "/3)..." << std::endl;
                continue;
            }
            success = true;
            std::cout << "Chunk " << chunk_id << " sent and ACK received." << std::endl;
        }

        if(!success){
            std::cerr << "Failed to send chunk " << chunk_id << " after 3 attempts. Aborting transfer." << std::endl;
            file.close();
            close(sock);
            return -1;
        }
        chunk_id++;
    }

        // Header end_header{END, chunk_id, 0};
        // send_all(sock, &end_header, sizeof(end_header));

        auto hash = compute_sha256(cfg.file);
        Header end_sha256_header{END, chunk_id, hash.size()};
        send_all(sock, &end_sha256_header, sizeof(end_sha256_header));
        send_all(sock, hash.data(), hash.size());

        if(recv_all(sock, buffer, sizeof(Header)) <= 0){
            std::cerr << "Failed to receive final ACK from server. Aborting." << std::endl;
            file.close();
            close(sock);
            return -1;
        }

        std::cout << "File transfer completed & final ACK received." << std::endl;

        file.close();
        close(sock);

        return 0;
    }