#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "ConfigParser.hpp"


class Client {
private:
    bool          finished;
    int           fd;
    int           listen_fd;
    int           clientPort;
    std::string   clientIP;
    std::string   serverIP;
    int           serverPort;
    ServerConfig* config;
    std::string   readBuffer;
    std::string   writeBuffer;

public:
    Client(int fd, ServerConfig *config, const std::string& client_ip, int client_port,
        const std::string& server_ip, int server_port);
    ServerConfig* getConfig() const { return config; }   
    ~Client();

    void handleRead();   // вызов при POLLIN
    void handleWrite();  // вызов при POLLOUT

    // Взаимодействие с HTTP-парсером (от участника 2)
    std::string& getRequestBuffer();
    std::string& getReadBuffer();
    std::string& getWriteBuffer();
    bool         isRequestReady();          // найден \r\n\r\n?
    void         setResponse(const std::string&); // HTTP-ответ
    bool         isDone();                   // можно удалять клиента
    std::string getClientIP() const { return clientIP; }
    int getClientPort() const { return clientPort; }
    void setListenFd(int fd) { listen_fd = fd; }
    int getListenFd() const { return listen_fd; }
    std::string getServerIP() const { return serverIP; }
    int getServerPort() const { return serverPort; }
    void updateConfig(ServerConfig* newConfig) { config = newConfig; }
};

#endif
