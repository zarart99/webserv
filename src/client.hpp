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

    void handleRead();   // POLLIN
    void handleWrite();  // POLLOUT

    // Request/Response buffer management
    std::string& getReadBuffer();
    void clearReadBuffer() { readBuffer.clear(); }
    std::string& getWriteBuffer();
    std::string& getRequestBuffer();
    bool isRequestReady();          // Check if complete request received (\r\n\r\n)
    void setResponse(const std::string&); // Set HTTP response

    // Client state management
    bool isDone();                  // Check if client can be removed
    
    // Client connection information
    std::string getClientIP() const { return clientIP; }
    int getClientPort() const { return clientPort; }
    std::string getServerIP() const { return serverIP; }
    int getServerPort() const { return serverPort; }
    
    // Server configuration
    int getListenFd() const { return listen_fd; }
    void setListenFd(int fd) { listen_fd = fd; }
    void updateConfig(ServerConfig* newConfig) { config = newConfig; }
    void markAsFinished() { finished = true; }
};

#endif
