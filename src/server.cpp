#include "server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include "HttpResponse.hpp"
#include "Cgi.hpp"

Server::Server(ConfigParser &parser)
    : cfg(parser),
      server_configs(parser.getServers())
{
    initListeners(server_configs);
}

Server::~Server()
{
    for (size_t i = 0; i < fds.size(); ++i)
        close(fds[i].fd);
    for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it)
        delete it->second;
}

void Server::initListeners(const std::vector<ServerConfig> &configs)
{
    for (size_t i = 0; i < configs.size(); ++i)
    {
        const std::vector<ListenStruct> &listenVec = configs[i].listen;
        for (size_t j = 0; j < listenVec.size(); ++j)
        {
            int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
            fcntl(listen_fd, F_SETFL, O_NONBLOCK);

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(listenVec[j].port);
            addr.sin_addr.s_addr = inet_addr(listenVec[j].ip.c_str());

            bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
            listen(listen_fd, 100);

            struct pollfd pfd;
            pfd.fd = listen_fd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            fds.push_back(pfd);

            listenConfigs[listen_fd] = configs[i];
        }
    }
}

void Server::run()
{
    while (true)
    {
        try
        {
            int ret = poll(&fds[0], fds.size(), 1000); // 1 сек
            if (ret < 0)
            {
                perror("poll");
                break;
            }
            handlePollEvents();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown error occurred" << std::endl;
        }
    }
}

void Server::handlePollEvents()
{
    for (size_t i = 0; i < fds.size(); ++i)
    {
        int fd = fds[i].fd;

        /* Новое соединение */
        if ((fds[i].revents & POLLIN) && listenConfigs.count(fd))
        {
            acceptNewClient(fd);
            continue;
        }

        /* Данные от существующего клиента */
        if ((fds[i].revents & POLLIN) && clients.count(fd))
        {
            clients[fd]->handleRead(); // ← вернули!

            if (clients[fd]->isRequestReady() &&
                clients[fd]->getWriteBuffer().empty())
            {
                /* 1. создаём HttpRequest */
                HttpRequest req(clients[fd]->getReadBuffer());

                /* 2. порт и Host */
                int port = 8001; //Заглушки
                std::string ip = "127.0.0.1"; //Заглушки
                std::string host;
                if (req.getHeaders().count("host"))
                    host = req.getHeaders().at("host");

                Cgi script(cfg, port, ip, host);
                RequestHandler handler(cfg);
                if (script.isCgi(req.getUri))
                {
                    script.cgiHandler()
                    clients[fd]->setResponse(resp.buildResponse());
                }
                else
                {
                    HttpResponse resp = handler.handleRequest(req, port, ip, host);
                    clients[fd]->setResponse(resp.buildResponse());
                }
            }
        }

        /* запись в сокет */
        if ((fds[i].revents & POLLOUT) && clients.count(fd))
            clients[fd]->handleWrite();
    }
    /* удаляем завершённых клиентов … */

    for (size_t i = 0; i < fds.size();)
    {
        int fd = fds[i].fd;
        if (clients.count(fd) && clients[fd]->isDone())
        {
            removeClient(fd);
        }
        else
        {
            ++i;
        }
    }
}

void Server::acceptNewClient(int listen_fd)
{
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0)
        return;
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    struct pollfd pfd = {client_fd, POLLIN | POLLOUT, 0};
    fds.push_back(pfd);

    ServerConfig *config = &listenConfigs[listen_fd];
    clients[client_fd] = new Client(client_fd, config);
}

void Server::removeClient(int client_fd)
{
    close(client_fd);
    delete clients[client_fd];
    clients.erase(client_fd);

    for (std::vector<struct pollfd>::iterator it = fds.begin(); it != fds.end(); ++it)
    {
        if (it->fd == client_fd)
        {
            fds.erase(it);
            break;
        }
    }
}

// #include <iostream>
// #include <vector>
// #include <cstring>
// #include <cerrno>
// #include <unistd.h>
// #include <fcntl.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <poll.h>
// #include "HttpResponse.hpp"
// #include "HttpRequest.hpp"

// #define PORT 8080
// #define BUFFER_SIZE 1024

// int make_socket_non_blocking(int fd) {
//     int flags = fcntl(fd, F_GETFL, 0);
//     if (flags < 0) return -1;
//     return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }

// int main() {
//     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd < 0) {
//         perror("socket");
//         return 1;
//     }

//     make_socket_non_blocking(server_fd);

//     int opt = 1;
//     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     struct sockaddr_in addr;
//     std::memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(PORT);
//     addr.sin_addr.s_addr = INADDR_ANY;

//     if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
//         perror("bind");
//         return 1;
//     }

//     if (listen(server_fd, 5) < 0) {
//         perror("listen");
//         return 1;
//     }

//     std::vector<struct pollfd> fds;
//     struct pollfd server_pollfd;
//     server_pollfd.fd = server_fd;
//     server_pollfd.events = POLLIN;
//     fds.push_back(server_pollfd);

//     std::cout << "Listening on port " << PORT << "...\n";

//     char buffer[BUFFER_SIZE];

//     while (true) {
//         int ready = poll(&fds[0], fds.size(), -1);
//         if (ready < 0) {
//             perror("poll");
//             break;
//         }

//         for (size_t i = 0; i < fds.size(); ++i) {
//             if (fds[i].revents & POLLIN) {
//                 if (fds[i].fd == server_fd) {
//                     int client_fd = accept(server_fd, NULL, NULL);
//                     if (client_fd >= 0) {
//                         make_socket_non_blocking(client_fd);
//                         struct pollfd client_pollfd;
//                         client_pollfd.fd = client_fd;
//                         client_pollfd.events = POLLIN;
//                         fds.push_back(client_pollfd);
//                         std::cout << "New client connected: fd " << client_fd << "\n";
//                     }
//                 } else {
//                     int client_fd = fds[i].fd;
//                     int len = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
//                     if (len <= 0) {
//                         std::cout << "Client disconnected: fd " << client_fd << "\n";
//                         close(client_fd);
//                         fds.erase(fds.begin() + i);
//                         --i;
//                         continue;
//                     }

//                     buffer[len] = '\0';
//                     std::string raw_request(buffer);
//                     HttpResponse res = handle_http_request(raw_request);
//                     std::string response = res.buildResponse();

//                     send(client_fd, response.c_str(), response.size(), 0);
//                     close(client_fd);  // закрываем, потому что Connection: close
//                     fds.erase(fds.begin() + i);
//                     --i;
//                 }
//             }
//         }
//     }

//     for (size_t i = 0; i < fds.size(); ++i)
//         close(fds[i].fd);

//     return 0;
// }
