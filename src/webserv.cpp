#include <iostream>
#include <vector>
#include "server.hpp"
#include <signal.h>
#include "ConfigParser.hpp"

int main(int argc, char **argv)
{

    std::string config_path = "webserv.conf";
    if (argc == 2)
        config_path = argv[1];
    else if (argc > 2)
    {
        std::cerr << "Fatal error: incorrect number of arguments" << std::endl;
        return 1;
    }

    try
    {
        /* 1. парсим конфиг */
        ConfigParser parser(config_path);

        /* 2. создаём сервер, передавая ЕМУ ссылку на парсер */
        Server server(parser);

        std::cout << "Webserv started. Listening …\n";
        server.run(); // never returns пока не Ctrl-C
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
