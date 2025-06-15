#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <string>

struct ListenStruct {
    std::string ip;
    int port;
};

struct ServerConfig {
    std::vector<ListenStruct> listen;
    // ... остальные поля
};

#endif
