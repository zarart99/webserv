#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
//#include "config.hpp"
#include "ConfigParser.hpp"


class Client {
public:
    Client(int fd, ServerConfig *config, const std::string& ip, int port);
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
    void updateConfig(ServerConfig* newConfig) { config = newConfig; }



private:
    int           fd;
    int           listen_fd;
    int           clientPort;
    bool          finished;
    ServerConfig* config;
    std::string   readBuffer;
    std::string   writeBuffer;
    std::string   clientIP;
};

#endif
