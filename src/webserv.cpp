#include <iostream>
#include <vector>
#include "server.hpp"
#include "config.hpp"

// Заглушка парсера конфигурации
std::vector<ServerConfig> parse_config(const std::string& path) {
    std::vector<ServerConfig> configs;
    // TODO: вызвать настоящий парсер (участник 3)
    // Пример для запуска (пока 127.0.0.1:8080)
    ServerConfig conf;
    ListenStruct lst;
    (void)path; // TODO: убрать, когда будет настоящий парсер
    lst.ip = "127.0.0.1";
    lst.port = 8080;
    conf.listen.push_back(lst);
    configs.push_back(conf);
    return configs;
}

int main(int argc, char** argv) {
    std::string config_path = "webserv.conf";
    if (argc == 2)
        config_path = argv[1];

    std::vector<ServerConfig> configs;
    try {
        configs = parse_config(config_path);
        if (configs.empty()) {
            std::cerr << "No servers in config. Exiting." << std::endl;
            return 1;
        }
    } catch (...) {
        std::cerr << "Failed to parse config!" << std::endl;
        return 1;
    }

    try {
        Server server(configs);
        std::cout << "Webserv started. Listening..." << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
