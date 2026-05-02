#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config_reader.h"
#include "protocol.h"
#include "socket_utils.h"
#include "checksum_helper.h"
#include "handle_transfer.h"
#include <thread>

int main()
{
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

TEST(TransferTest, BasicTransfer)
{
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    Config cfg{};
    cfg.chunk_size = 1024;

    std::thread t([&]()
                  {
        handle_transfer(sv[1], cfg);
        close(sv[1]); });

    std::string filename = "test.txt";
    std::string content = "hello";

    uint32_t name_len = filename.size();
    size_t file_size = content.size();

    std::cout << "SEND START\n";

    uint32_t meta_size = sizeof(file_size) + sizeof(name_len) + name_len;
    Header start{START, 0, meta_size};
    send_all(sv[0], &start, sizeof(start));
    send_all(sv[0], &file_size, sizeof(file_size));
    send_all(sv[0], &name_len, sizeof(name_len));
    send_all(sv[0], filename.c_str(), name_len);

    std::cout << "SEND DATA\n";

    Header data{DATA, 0, (uint32_t)content.size()};
    send_all(sv[0], &data, sizeof(data));
    send_all(sv[0], content.data(), content.size());

    std::cout << "WAIT ACK\n";

    Header ack{};
    int r = recv_all(sv[0], &ack, sizeof(ack));
    std::cout << "ACK RECV RESULT: " << r << "\n";

    EXPECT_GT(r, 0);
    EXPECT_EQ(ack.type, ACK);

    std::ofstream tmp("client_test.txt", std::ios::binary);
    tmp << content;
    tmp.close();

    auto hash = compute_sha256("client_test.txt");

    Header end{END, 0, (uint32_t)hash.size()};
    send_all(sv[0], &end, sizeof(end));
    send_all(sv[0], hash.data(), hash.size());

    // Receive final ACK
    Header final_ack{};
    int r1 = recv_all(sv[0], &final_ack, sizeof(final_ack));

    EXPECT_GT(r1, 0);
    EXPECT_EQ(final_ack.type, END);

    shutdown(sv[0], SHUT_WR);
    close(sv[0]);

    t.join();
}

TEST(TransferTest, HashMismatch)
{
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    Config cfg{};
    cfg.chunk_size = 1024;

    std::thread t([&]()
                  {
        handle_transfer(sv[1], cfg);
        close(sv[1]); });

    std::string filename = "test.txt";
    std::string content = "hello";

    uint32_t name_len = filename.size();
    uint64_t file_size = content.size();

    uint32_t meta_size = sizeof(file_size) + sizeof(name_len) + name_len;
    Header start{START, 0, meta_size};

    send_all(sv[0], &start, sizeof(start));
    send_all(sv[0], &file_size, sizeof(file_size));
    send_all(sv[0], &name_len, sizeof(name_len));
    send_all(sv[0], filename.c_str(), name_len);

    Header data{DATA, 0, (uint32_t)content.size()};
    send_all(sv[0], &data, sizeof(data));
    send_all(sv[0], content.data(), content.size());

    Header ack{};
    recv_all(sv[0], &ack, sizeof(ack));

    std::array<unsigned char, 32> wrong_hash{};
    Header end{END, 0, (uint32_t)wrong_hash.size()};
    send_all(sv[0], &end, sizeof(end));
    send_all(sv[0], wrong_hash.data(), wrong_hash.size());

    Header final_ack{};
    int r = recv_all(sv[0], &final_ack, sizeof(final_ack));

    EXPECT_LE(r, 0);

    close(sv[0]);
    t.join();
}

TEST(TransferTest, ClientDisconnect)
{
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    Config cfg{};
    cfg.chunk_size = 1024;

    std::thread t([&]() {
        handle_transfer(sv[1], cfg);
        close(sv[1]);
    });

    std::string filename = "partial.txt";
    std::string content = "hello world";

    uint32_t name_len = filename.size();
    uint64_t file_size = content.size();

    uint32_t meta_size = sizeof(file_size) + sizeof(name_len) + name_len;
    Header start{START, 0, meta_size};

    send_all(sv[0], &start, sizeof(start));
    send_all(sv[0], &file_size, sizeof(file_size));
    send_all(sv[0], &name_len, sizeof(name_len));
    send_all(sv[0], filename.c_str(), name_len);

    Header data{DATA, 0, 5};
    send_all(sv[0], &data, sizeof(data));
    send_all(sv[0], content.data(), 5);

    // disconnect
    close(sv[0]);

    t.join();

    SUCCEED(); // just ensure no crash
}

TEST(TransferTest, EmptyFileTransfer)
{
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    Config cfg{};
    cfg.chunk_size = 1024;

    std::thread t([&]() {
        handle_transfer(sv[1], cfg);
        close(sv[1]);
    });

    std::string filename = "empty.txt";
    std::string content = "";

    uint32_t name_len = filename.size();
    uint64_t file_size = 0;

    uint32_t meta_size = sizeof(file_size) + sizeof(name_len) + name_len;
    Header start{START, 0, meta_size};

    send_all(sv[0], &start, sizeof(start));
    send_all(sv[0], &file_size, sizeof(file_size));
    send_all(sv[0], &name_len, sizeof(name_len));
    send_all(sv[0], filename.c_str(), name_len);

    // No DATA packets

    std::ofstream tmp("client_empty.txt", std::ios::binary);
    tmp.close();

    auto hash = compute_sha256("client_empty.txt");

    Header end{END, 0, (uint32_t)hash.size()};
    send_all(sv[0], &end, sizeof(end));
    send_all(sv[0], hash.data(), hash.size());

    Header final_ack{};
    ASSERT_GT(recv_all(sv[0], &final_ack, sizeof(final_ack)), 0);
    EXPECT_EQ(final_ack.type, END);

    close(sv[0]);
    t.join();
}

TEST(TransferTest, MultiChunkTransfer)
{
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    Config cfg{};
    cfg.chunk_size = 1024;

    std::thread t([&]() {
        handle_transfer(sv[1], cfg);
        close(sv[1]);
    });

    std::string filename = "multi.txt";
    std::string content(5000, 'A'); // multiple chunks

    uint32_t name_len = filename.size();
    uint64_t file_size = content.size();

    uint32_t meta_size = sizeof(file_size) + sizeof(name_len) + name_len;
    Header start{START, 0, meta_size};

    send_all(sv[0], &start, sizeof(start));
    send_all(sv[0], &file_size, sizeof(file_size));
    send_all(sv[0], &name_len, sizeof(name_len));
    send_all(sv[0], filename.c_str(), name_len);

    // Send chunks
    uint32_t chunk_id = 0;
    for (size_t i = 0; i < content.size(); i += cfg.chunk_size)
    {
        size_t chunk_size = std::min((size_t)cfg.chunk_size, content.size() - i);

        Header data{DATA, chunk_id++, (uint32_t)chunk_size};
        send_all(sv[0], &data, sizeof(data));
        send_all(sv[0], content.data() + i, chunk_size);

        Header ack{};
        ASSERT_GT(recv_all(sv[0], &ack, sizeof(ack)), 0);
        ASSERT_EQ(ack.type, ACK);
    }

    // Compute sender hash
    std::ofstream tmp("client_multi.txt", std::ios::binary);
    tmp << content;
    tmp.close();

    auto hash = compute_sha256("client_multi.txt");

    Header end{END, 0, (uint32_t)hash.size()};
    send_all(sv[0], &end, sizeof(end));
    send_all(sv[0], hash.data(), hash.size());

    Header final_ack{};
    ASSERT_GT(recv_all(sv[0], &final_ack, sizeof(final_ack)), 0);
    EXPECT_EQ(final_ack.type, END);

    close(sv[0]);
    t.join();
}