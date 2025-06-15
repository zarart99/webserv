#include "RequestHandler.hpp"
#include "../conf_parser/ConfigParser.hpp"
#include <sys/stat.h>
#include <unistd.h>

RequestHandler::RequestHandler(ConfigParser &config) : _config(config) {}
RequestHandler::~RequestHandler() {}

HttpResponse RequestHandler::handleRequest(const HttpRequest &request, int serverPort, const std::string &serverHost)
{

    const ServerConfig *server_config = NULL;
    const LocationStruct *location_config = NULL;

    try
    {
        server_config = _findServerConfig(serverPort, serverHost);

        if (!server_config)
        {
            return _createErrorResponse(400, NULL);
        }

        location_config = _findLocationFor(*server_config, request.getUri());

        if (!location_config)
        {
            return _createErrorResponse(404, server_config);
        }

        size_t limit_in_bytes = server_config->client_max_body_sizeDef;
        size_t content_length = request.getContentLength();
        if (limit_in_bytes > 0 && content_length > limit_in_bytes)
        {
            return _createErrorResponse(413, server_config);
        }

        const std::vector<std::string> &allowed_methods = location_config->allow_methods;
        if (!allowed_methods.empty())
        {
            bool method_is_allowed = false;
            for (size_t i = 0; i < allowed_methods.size(); ++i)
            {
                if (request.getMethod() == allowed_methods[i])
                {
                    method_is_allowed = true;
                    break;
                }
            }
            if (!method_is_allowed)
            {
                return _createErrorResponse(405, server_config);
            }
        }

        if (request.getMethod() == "GET")
        {
            return _handleGet(request, *location_config, *server_config);
        }
        else if (request.getMethod() == "POST")
        {
            return _handlePost(request, *location_config, *server_config);
        }
        else if (request.getMethod() == "DELETE")
        {
            return _handleDelete(request, *location_config, *server_config);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    
        int statusCode = 500;

        std::string error_message = e.what();
        std::stringstream ss(error_message);
   
        ss >> statusCode; // Если в начале строки не число, statusCode останется 500
    
        return _createErrorResponse(statusCode, server_config);
    
    }

    return _createErrorResponse(501, server_config);

}

HttpResponse RequestHandler::_handleGet(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    // Определяем корневую директорию: берем из location, если есть, иначе из server
    std::string root = !location.root.empty() ? location.root : server.rootDef;
    std::string path = root + request.getUri();

    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0)
    {
        return _createErrorResponse(404, &server); // Путь не существует
    }

    // Если URI указывает на директорию
    if (S_ISDIR(path_stat.st_mode))
    {
        // Определяем список индексных файлов
        const std::vector<std::string> &index_files = !location.index.empty() ? location.index : server.indexDef;
        std::string found_index_path = "";
        for (size_t i = 0; i < index_files.size(); ++i)
        {
            std::string temp_path = path + (path[path.length() - 1] == '/' ? "" : "/") + index_files[i];
            if (access(temp_path.c_str(), F_OK) == 0)
            {
                found_index_path = temp_path;
                break;
            }
        }
        if (!found_index_path.empty())
        {
            path = found_index_path; // Нашли индекс, будем отдавать его
        }
        else
        {
            // TODO: Проверить autoindex. Пока просто возвращаем ошибку.
            return _createErrorResponse(403, &server);
        }
    }

    if (access(path.c_str(), R_OK) != 0)
    {
        return _createErrorResponse(403, &server);
    }

    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return _createErrorResponse(500, &server);
    }
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    
    HttpResponse response;
    response.setStatusCode(200);
    // TODO: 
    response.addHeader("Content-Type", "text/plain"); // Миш, я тут хз как определять MIME-тип. Пока хардкод
    response.setBody(body);
    return response;
}

HttpResponse RequestHandler::_handlePost(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    // Миша, где хранится путь для загрузок (upload_path) ?
    // Пока использую root.
    std::string upload_dir = !location.root.empty() ? location.root : server.rootDef;
    std::string path = upload_dir + "/uploaded_file.tmp";

    std::ofstream file(path.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        return _createErrorResponse(500, &server);
    }
    file.write(request.getBody().c_str(), request.getBody().length());
    if (file.fail())
    {
        file.close();
        return _createErrorResponse(500, &server); // Ошибка записи на диск
    }
    file.close();

    HttpResponse response;
    response.setStatusCode(201);// Created
    response.addHeader("Location", path);
    return response;
}

HttpResponse RequestHandler::_handleDelete(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    std::string root = !location.root.empty() ? location.root : server.rootDef;
    std::string path = root + request.getUri();

    if (access(path.c_str(), F_OK) == -1)
    {
        return _createErrorResponse(404, &server);
    }
    if (access(path.c_str(), W_OK) == -1)
    {
        return _createErrorResponse(403, &server); // Нет прав на запись/удаление
    }

    if (std::remove(path.c_str()) != 0)
    {
        return _createErrorResponse(500, &server); // Системная ошибка при удалении
    }

    HttpResponse response;
    response.setStatusCode(204); // No Content
    return response;
}

HttpResponse RequestHandler::_createErrorResponse(int statusCode, const ServerConfig *server)
{
    HttpResponse response;
    response.setStatusCode(statusCode);
    response.addHeader("Content-Type", "text/html; charset=utf-8"); // тут нужно уточнить, какой тип контента возвращаем

    std::string body;
    // Пытаемся найти кастомную страницу ошибки, если конфиг сервера доступен
    if (server)
    {
        std::map<int, std::string>::const_iterator it = server->error_pageDef.find(statusCode);
        if (it != server->error_pageDef.end())
        {
            std::ifstream file(it->second.c_str());
            if (file.is_open())
            {
                body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                response.setBody(body);
                return response;
            }
        }
    }
}

const ServerConfig* RequestHandler::_findServerConfig(int port, const std::string& host) const {
    const std::vector<ServerConfig>& servers = _config.getServers(); //Миш, мне тут нужен геттер типа return this->_configServ
    const ServerConfig* default_server_for_port = NULL;

    for (size_t i = 0; i < servers.size(); ++i) {
        bool port_match = false;
        // Проверяем, слушает ли текущий сервер нужный порт
        for (size_t j = 0; j < servers[i].listen.size(); ++j) {
            if (servers[i].listen[j].port == port) {
                port_match = true;
                // Если это первый сервер, который мы нашли для этого порта,
                // запоминаем его как дефолтный на случай, если не найдем точного совпадения по хосту.
                if (!default_server_for_port) {
                    default_server_for_port = &servers[i];
                }
                break;
            }
        }

        if (!port_match) {
            continue; // Этот сервер не на нашем порту, пропускаем
        }

        // Если порт совпал, проверяем server_name
        for (size_t j = 0; j < servers[i].server_name.size(); ++j) {
            if (servers[i].server_name[j] == host) {
                return &servers[i]; // Нашли точное совпадение! Возвращаем его.
            }
        }
    }

    // Если мы прошли весь цикл и не нашли точного совпадения по хосту,
    // возвращаем дефолтный сервер для этого порта (или NULL, если и такого не было).
    return default_server_for_port;
}

// Этот метод ищет наиболее подходящий location блок внутри данного сервера.
// Логика:
// Ищем локацию, чей префикс длиннее всего совпадает с началом URI.
// Например, для URI "/images/cat/avatar.jpg", префикс "/images/cat/" лучше, чем "/images/".
const LocationStruct* RequestHandler::_findLocationFor(const ServerConfig& server, const std::string& uri) const {
    const LocationStruct* best_match = NULL;
    size_t max_len = 0;

    for (size_t i = 0; i < server.location.size(); ++i) {
        const std::string& prefix = server.location[i].prefix;
        
        // Проверяем, что URI начинается с префикса локации.
        // rfind(prefix, 0) == 0 - это эффективный способ проверить "startsWith".
        if (uri.rfind(prefix, 0) == 0) {
            // Если длина текущего префикса больше, чем у лучшего найденного ранее,
            // то это новое лучшее совпадение.
            if (prefix.length() > max_len) {
                max_len = prefix.length();
                best_match = &server.location[i];
            }
        }
    }

    // Возвращаем указатель на локацию с самым длинным совпадением префикса.
    // Если ничего не совпало, вернется NULL.
    return best_match;
}
