#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
//#include "config.hpp"
#include "ConfigParser.hpp"


class Client {
public:
    Client(int fd, ServerConfig* config);
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


private:
    int           fd;
    ServerConfig* config;
    std::string   readBuffer;
    std::string   writeBuffer;
    bool          finished;
};

#endif
