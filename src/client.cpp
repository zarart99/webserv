#include "client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include "HttpResponse.hpp"

Client::Client(int fd, ServerConfig* config) : fd(fd), config(config), finished(false) {}

Client::~Client() { close(fd); }

void Client::handleRead() {
    char buf[4096];
    int n = recv(fd, buf, sizeof(buf), 0);
    (void)config; // TODO убрать заглушку

    if (n > 0) {
        readBuffer.append(buf, n);
        if (isRequestReady() && writeBuffer.empty()) {
            std::cout << "Processing HTTP request: \n" << readBuffer << std::endl;
            HttpResponse httpResponse = handleHttpRequest(readBuffer);
            std::string response = httpResponse.buildResponse();
            setResponse(response);
        }
    } else if (n == 0) {
        finished = true; // клиент закрыл соединение
    } else if (n < 0) {
        finished = true;
        std::cerr << "Read error on fd " << fd << std::endl;
    }
}

void Client::handleWrite() {
    if (writeBuffer.empty()) return;
    int n = send(fd, writeBuffer.c_str(), writeBuffer.size(), 0);
    std::cout << "Response sent and connection closing (fd=" << fd << ")\n";
    if (n > 0) writeBuffer.erase(0, n);
    if (writeBuffer.empty()) finished = true; // если отправлено всё
}

std::string& Client::getRequestBuffer() { return readBuffer; }
bool Client::isRequestReady() {
    return (readBuffer.find("\r\n\r\n") != std::string::npos);
}
void Client::setResponse(const std::string& resp) { writeBuffer = resp; }
bool Client::isDone() { return finished; }

std::string& Client::getReadBuffer() { return readBuffer; }
std::string& Client::getWriteBuffer() { return writeBuffer; }
