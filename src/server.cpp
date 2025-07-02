#include "server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include "HttpResponse.hpp"

#define CLIENT_MAX_NUMBER 1000 

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

const ServerConfig* Server::findMatchingConfig(const std::vector<ServerConfig>& configs, std::string host)
{
    // Проверка на пустой вектор
    if (configs.empty())
        return NULL; // В C++98 используем NULL вместо nullptr
    
    // Удаляем порт из хоста, если он есть
    size_t pos = host.find(':');
    if (pos != std::string::npos)
        host = host.substr(0, pos);
    
    // Если имя хоста пустое, используем первую конфигурацию
    if (host.empty())
        return &configs[0];
    
    // Ищем совпадение по server_name
    for (size_t i = 0; i < configs.size(); ++i) {
        for (size_t j = 0; j < configs[i].server_name.size(); ++j) {
            if (configs[i].server_name[j] == host)
                return &configs[i];
        }
    }
    
    // Если совпадений нет, используем первую конфигурацию
    return &configs[0];
}

void Server::initListeners(const std::vector<ServerConfig> &configs)
{
    // Получаем уникальные листенеры из парсера
    std::vector<ListenStruct> uniqueListeners = cfg.getUniqueListen();
    
    for (size_t i = 0; i < uniqueListeners.size(); ++i)
    {
        const ListenStruct& listenInfo = uniqueListeners[i];
        
        // Создаем сокет для каждого уникального листенера
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
            std::cerr << "Ошибка при создании сокета: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Устанавливаем опцию повторного использования адреса
        int opt = 1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Ошибка setsockopt(SO_REUSEADDR): " << strerror(errno) << std::endl;
        }
        
        // Устанавливаем неблокирующий режим
        fcntl(listen_fd, F_SETFL, O_NONBLOCK);

        // Настраиваем адрес
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(listenInfo.port);
        addr.sin_addr.s_addr = inet_addr(listenInfo.ip.c_str());

        // Привязываем сокет к адресу и порту
        if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            std::cerr << "Ошибка bind на " << listenInfo.ip << ":" 
                      << listenInfo.port << ": " << strerror(errno) << std::endl;
            close(listen_fd);
            continue;
        }
        
        // Переводим сокет в режим прослушивания
        if (listen(listen_fd, 100) < 0) {
            std::cerr << "Ошибка listen на " << listenInfo.ip << ":" 
                      << listenInfo.port << ": " << strerror(errno) << std::endl;
            close(listen_fd);
            continue;
        }

        // Добавляем в структуру poll
        struct pollfd pfd;
        pfd.fd = listen_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        fds.push_back(pfd);
        
        std::cout << "Сервер слушает на " << listenInfo.ip << ":" << listenInfo.port << std::endl;

        // Находим все конфигурации серверов, которые слушают на этом IP:порт
        for (size_t j = 0; j < configs.size(); ++j) {
            for (size_t k = 0; k < configs[j].listen.size(); ++k) {
                if (configs[j].listen[k].ip == listenInfo.ip && 
                    configs[j].listen[k].port == listenInfo.port) {
                    // Связываем сокет с конфигурацией сервера
                    listenConfigs[listen_fd].push_back(configs[j]);
                    break; // Достаточно одного совпадения для этой конфигурации
                }
            }
        }
    }
    
    if (fds.empty()) {
        throw std::runtime_error("Не удалось инициализировать ни один слушающий сокет");
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
    for (size_t i = 0; i < fds.size(); ++i) {
        int fd = fds[i].fd;
        
        // Новое соединение
        if ((fds[i].revents & POLLIN) && listenConfigs.count(fd)) {
            acceptNewClient(fd);
            continue;
        }
        
        // Чтение от клиента
        if ((fds[i].revents & POLLIN) && clients.count(fd)) {
            clients[fd]->handleRead();
            
            // Если запрос готов и буфер записи пуст
            if (clients[fd]->isRequestReady() && clients[fd]->getWriteBuffer().empty()) {
                processRequest(fd);
            }
        }
        
        // Запись клиенту
        if ((fds[i].revents & POLLOUT) && clients.count(fd))
            clients[fd]->handleWrite();
    }
    
    // Удаляем завершенных клиентов
    for (size_t i = 0; i < fds.size();) {
        int fd = fds[i].fd;
        if (clients.count(fd) && clients[fd]->isDone())
            removeClient(fd);
        else
            ++i;
    }
}

void Server::acceptNewClient(int listen_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0)
        return;
        
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    
    // Получаем IP и порт клиента
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    // Добавляем в poll
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN | POLLOUT;
    pfd.revents = 0;
    fds.push_back(pfd);
    // Проверяем лимит клиентов - для C++98
    if (clients.size() >= CLIENT_MAX_NUMBER) {
        close(client_fd);
        std::cout << "Достигнут максимальный лимит клиентов" << std::endl;
        return;
    }    
    // Используем первую конфигурацию до получения Host
    if (!listenConfigs[listen_fd].empty()) {
        clients[client_fd] = new Client(client_fd, &listenConfigs[listen_fd][0], 
                                       client_ip, client_port);
        clients[client_fd]->setListenFd(listen_fd);
        std::cout << "Соединение: " << client_ip << ":" << client_port << std::endl;
    } else {
        close(client_fd);
        // Удаляем из fds
        for (std::vector<struct pollfd>::iterator it = fds.begin(); it != fds.end(); ++it) {
            if (it->fd == client_fd) {
                fds.erase(it);
                break;
            }
        }
    }
}

void Server::processRequest(int fd)
{
    HttpRequest req(clients[fd]->getReadBuffer());
    
    // Получаем host из заголовков
    std::string host;
    if (req.getHeaders().count("host"))
        host = req.getHeaders().at("host");
    
    // Получаем listen_fd и находим подходящую конфигурацию
    int listen_fd = clients[fd]->getListenFd();
    const ServerConfig* config = findMatchingConfig(listenConfigs[listen_fd], host);
    
    // Проверяем, что есть действительная конфигурация
    if (config != NULL) {
        // Используем const_cast, поскольку updateConfig ожидает не-константный указатель
        // Это безопасно, так как мы не изменяем саму конфигурацию внутри Client
        clients[fd]->updateConfig(const_cast<ServerConfig*>(config));
        
        // Обрабатываем запрос
        RequestHandler handler(cfg);
        HttpResponse resp = handler.handleRequest(req, config->listen[0].port, host);
        
        // Устанавливаем ответ
        clients[fd]->setResponse(resp.buildResponse());
    }
}

void Server::removeClient(int client_fd)
{
    std::cout << "Закрытие соединения: fd=" << client_fd 
    << " (" << clients[client_fd]->getClientIP() 
    << ":" << clients[client_fd]->getClientPort() << ")" << std::endl;

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
