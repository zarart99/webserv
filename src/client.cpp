#include "client.hpp"
#include <unistd.h>
#include <sys/socket.h>

Client::Client(int fd, ServerConfig* config) : _fd(fd), _config(config), _finished(false) {}

Client::~Client() { close(_fd); }

void Client::handle_read() {
    char buf[4096];
    int n = recv(_fd, buf, sizeof(buf), 0);
    (void)_config; // заглушка
    if (n > 0) {
        _read_buffer.append(buf, n);
        // можно сигнализировать парсеру HTTP о новых данных
    } else if (n == 0) {
        _finished = true;
    }
}

void Client::handle_write() {
    if (_write_buffer.empty()) return;
    int n = send(_fd, _write_buffer.c_str(), _write_buffer.size(), 0);
    if (n > 0) _write_buffer.erase(0, n);
    if (_write_buffer.empty()) _finished = true; // если отправлено всё
}

std::string& Client::get_request_buffer() { return _read_buffer; }
bool Client::is_request_ready() {
    return (_read_buffer.find("\r\n\r\n") != std::string::npos); // очень просто
}
void Client::set_response(const std::string& resp) { _write_buffer = resp; }
bool Client::is_done() { return _finished; }
