#include "platform.h"
#include <atomic>
#include <csignal>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <set>
#include "handle_transfer.h"

std::atomic<bool> g_running(true);
int server_fd_global = -1;
std::queue<int> client_queue;
std::mutex queue_mutex;
std::condition_variable cv{};
std::set<int> active_clients;
std::mutex active_clients_mutex;
Config cfg{};

void handle_sigint(int)
{
    g_running = false;
    if (server_fd_global != -1)
        CLOSE_SOCKET(server_fd_global);
}

void worker()
{
    while (g_running)
    {
        int client = -1;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            cv.wait(lock, []
                    { return !client_queue.empty() || !g_running; });

            if (client_queue.empty())
                continue;

            client = client_queue.front();
            client_queue.pop();
            {
                std::unique_lock<std::mutex> active_lock(active_clients_mutex);
                active_clients.insert(client);
            }
        }

        handle_transfer(client, cfg);
        {
            std::unique_lock<std::mutex> active_lock(active_clients_mutex);
            active_clients.erase(client);
        }
        CLOSE_SOCKET(client);
    }
}

int main()
{

    std::signal(SIGINT, handle_sigint);

    cfg = load_cfg();
    init_sockets();

    int server_fd = -1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket Failed");
        cleanup_sockets();
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(cfg.port);

    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    server_fd_global = server_fd;

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
    {
        perror("Bind Failed");
        CLOSE_SOCKET(server_fd);
        server_fd_global = -1;
        cleanup_sockets();
        return -1;
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen error");
        CLOSE_SOCKET(server_fd);
        server_fd_global = -1;
        cleanup_sockets();
        return -1;
    }

    std::vector<std::thread> th_pool;
    for (int i = 0; i < cfg.thread_count; i++)
        th_pool.emplace_back(worker);

    std::cout << "Server listening on port " << cfg.port << "...\n";

    while (g_running)
    {
        int client = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client < 0)
        {
            if (!g_running)
                break;
            if (errno == EINTR)
                break;
            perror("Accept error");
            continue;
        }

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            client_queue.push(client);
        }
        cv.notify_one();
    }

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        while (!client_queue.empty())
        {
            CLOSE_SOCKET(client_queue.front());
            client_queue.pop();
        }
    }

    {
        std::unique_lock<std::mutex> active_lock(active_clients_mutex);
        for (int client : active_clients)
            shutdown(client, SHUTDOWN_BOTH);
    }

    cv.notify_all();

    for (auto &t : th_pool)
    {
        if (t.joinable())
            t.join();
    }

    CLOSE_SOCKET(server_fd);
    server_fd_global = -1;
    cleanup_sockets();

    return 0;
}
