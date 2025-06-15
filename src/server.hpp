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
    void initListeners(const std::vector<ServerConfig> &);
    void handlePollEvents();
    void acceptNewClient(int listen_fd);
    void removeClient(int client_fd);

    ConfigParser &cfg;                              
    const std::vector<ServerConfig> &server_configs;

    std::vector<struct pollfd> fds;
    std::map<int, ServerConfig> listenConfigs;
    std::map<int, Client *> clients;
};
#endif // SERVER_HPP