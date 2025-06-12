#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "config.hpp"

class Client {
public:
    Client(int fd, ServerConfig* config);
    ~Client();

    void handle_read();   // вызов при POLLIN
    void handle_write();  // вызов при POLLOUT

    // Взаимодействие с HTTP-парсером (от участника 2)
    std::string& get_request_buffer();
    bool         is_request_ready();          // найден \r\n\r\n?
    void         set_response(const std::string&); // HTTP-ответ
    bool         is_done();                   // можно удалять клиента

private:
    int          _fd;
    ServerConfig* _config;
    std::string  _read_buffer;
    std::string  _write_buffer;
    bool         _finished;
};

#endif
