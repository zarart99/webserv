#include "client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include <cstdlib>

Client::Client(int fd, ServerConfig *config, const std::string& client_ip, int client_port,
            const std::string& server_ip, int server_port)
            : finished(false), fd(fd), listen_fd(0), clientPort(client_port),
            clientIP(client_ip), serverIP(server_ip), serverPort(server_port),
            config(config), readBuffer(""), writeBuffer("") {}

Client::~Client() { close(fd); }

void Client::handleRead()
{
    char buf[4096];
    int n = recv(fd, buf, sizeof(buf), 0);
    (void)config; // TODO убрать заглушку

    if (n > 0)
    {
        readBuffer.append(buf, n);
        static const size_t MAX_HEADER_SIZE = 8192;
        if (readBuffer.find("\r\n\r\n") == std::string::npos &&
            readBuffer.size() > MAX_HEADER_SIZE && writeBuffer.empty())
        {
            HttpResponse err = RequestHandler::createErrorResponse(431);
            writeBuffer = err.buildResponse();
            return;
        }
        if (isRequestReady() && writeBuffer.empty())
        {
            std::cout << "Processing HTTP request: \n"
                      << readBuffer << std::endl;
            //HttpResponse httpResponse = handleHttpRequest(readBuffer);
            //std::string response = httpResponse.buildResponse();
            //setResponse(response);
        }
    }
    else if (n == 0)
    {
        finished = true; // клиент закрыл соединение
    }
    else if (n < 0)
    {
        finished = true;
        std::cerr << "Read error on fd " << fd << std::endl;
    }
}

void Client::handleWrite()
{
    if (writeBuffer.empty())
        return;
    int n = send(fd, writeBuffer.c_str(), writeBuffer.size(), 0);
    
    if (n > 0) {
        writeBuffer.erase(0, n);
        if (writeBuffer.empty()) {
            std::cout << "Response sent and connection closing (fd=" << fd << ")\n";
            finished = true; // если отправлено всё
        }
    } 
    else if (n == 0) {
        // Zero bytes sent (unusual but possible)
        std::cout << "Zero bytes sent, possible connection issue (fd=" << fd << ")\n";
    }
    else if (n < 0) {
        // Error occurred
        std::cerr << "Write error on fd " << fd << std::endl;
        finished = true; // Mark as finished due to error
    }
}

std::string &Client::getRequestBuffer() { return readBuffer; }
bool Client::isRequestReady()
{
    size_t headEnd = readBuffer.find("\r\n\r\n");
    if (headEnd == std::string::npos)
        return false;
    std::string headers = readBuffer.substr(0, headEnd + 4);
    if (headers.find("Transfer-Encoding: chunked") != std::string::npos)
    {
        if (readBuffer.find("\r\n0\r\n\r\n", headEnd + 4) != std::string::npos) // ждем конец чанков
            return true;
        return false;
    }
    size_t pos = headers.find("Content-Length:");
    if (pos != std::string::npos)
    {
        pos += 15;
        while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t'))
            ++pos;
        long len = std::strtol(headers.c_str() + pos, NULL, 10);
        if (len >= 0 && readBuffer.size() >= headEnd + 4 + static_cast<size_t>(len))
            return true;
        return false;
    }
    return true;
}
void Client::setResponse(const std::string &resp) { writeBuffer = resp; }
bool Client::isDone() { return finished; }

std::string &Client::getReadBuffer() { return readBuffer; }
std::string &Client::getWriteBuffer() { return writeBuffer; }
