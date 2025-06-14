#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>
#include "client.hpp"
#include "config.hpp" // Структура ServerConfig/LocationStruct (от Участника 3)

class Server {
public:
    Server(const std::vector<ServerConfig>& configs);
    ~Server();

    void run();

private:
    std::vector<struct pollfd> fds;                // Все fd: listen + клиенты
    std::map<int, Client*>     clients;            // fd -> Client*
    std::map<int, ServerConfig> listenConfigs;     // listen_fd -> ServerConfig

    void initListeners(const std::vector<ServerConfig>& configs);
    void acceptNewClient(int listen_fd);
    void removeClient(int client_fd);
    void handlePollEvents();

    // Запрещаем копирование
    Server(const Server&);
    Server& operator=(const Server&);
};

#endif
