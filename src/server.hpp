#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>
#include "client.hpp"
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"
#include "Cgi.hpp"

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
    const ServerConfig* findMatchingConfig(const std::vector<ServerConfig>& configs, std::string host);
    void processRequest(int fd);

    ConfigParser &cfg;                              
    const std::vector<ServerConfig> &server_configs;

    std::vector<struct pollfd> fds;
    std::map<int, Client *> clients;
    std::map<int, std::vector<ServerConfig> > listenConfigs;
};
#endif // SERVER_HPP