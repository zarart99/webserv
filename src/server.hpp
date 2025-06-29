#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>
#include "client.hpp"
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"

class Server
{
public:
    Server(ConfigParser &parser);
    ~Server();

    void run();

private:
    std::vector<struct pollfd> fds;                // Все fd: listen + клиенты
    std::map<int, Client*>     clients;            // fd -> Client*
    std::map<int, ServerConfig> listenConfigs;     // listen_fd -> ServerConfig
    std::map<int, time_t> clientLastActivity; // Track last activity time for each client
    const int CLIENT_TIMEOUT = 30; // Timeout in seconds

    void initListeners(const std::vector<ServerConfig>& configs);
    void acceptNewClient(int listen_fd);
    void removeClient(int client_fd);
    void handlePollEvents();
    

    // Запрещаем копирование
    Server(const Server&);
    Server& operator=(const Server&);
};
#endif // SERVER_HPP