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
    std::vector<struct pollfd> _fds;                // Все fd: listen + клиенты
    std::map<int, Client*>     _clients;            // fd -> Client*
    std::map<int, ServerConfig> _listenConfigs;     // listen_fd -> ServerConfig

    void init_listeners(const std::vector<ServerConfig>& configs);
    void accept_new_client(int listen_fd);
    void remove_client(int client_fd);
    void handle_poll_events();

    // Запрещаем копирование
    Server(const Server&);
    Server& operator=(const Server&);
};

#endif
