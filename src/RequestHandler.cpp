#include "RequestHandler.hpp"
#include "ConfigParser.hpp"
#include "utils.hpp"
#include "MimeTypes.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <cctype>

RequestHandler::RequestHandler() : _config(NULL) {}

RequestHandler::RequestHandler(ConfigParser &config) : _config(&config) {}

RequestHandler::RequestHandler(const RequestHandler &src)
{
    *this = src;
}

RequestHandler &RequestHandler::operator=(const RequestHandler &src)
{
    if (this != &src)
    {
        _config = src._config;
    }
    return *this;
}

RequestHandler::~RequestHandler() {}

HttpResponse RequestHandler::handleRequest(const HttpRequest &request, int serverPort, const std::string &serverHost)
{

    const ServerConfig *server_config = NULL;
    const LocationStruct *location_config = NULL;

    try
    {
        server_config = _findServerConfig(serverPort, serverHost);

        // Проверяем, 400
        if (!server_config)
        {
            return _createErrorResponse(400, NULL);
        }

        location_config = _findLocationFor(*server_config, request.getUri());

        // Проверяем, 404

        if (!location_config)
        {
            return _createErrorResponse(404, server_config);
        }

        // Проверяем, 413

        size_t limit_in_bytes = location_config->client_max_body_size;
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
            // Если метод не разрешен, возвращаем 405
            if (!method_is_allowed)
            {
                return _createErrorResponse(405, server_config, &allowed_methods);
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

    return _createErrorResponse(501, server_config); // Если метод не реализован, возвращаем 501 Not Implemented
}

HttpResponse RequestHandler::_handleGet(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    // Определяем корневую директорию: берем из location, если есть, иначе из server
    std::string root;
    if (!location.upload_path.empty())
        root = location.upload_path;
    else if (!location.root.empty())
        root = location.root;
    else
        root = server.rootDef;                            // Собираем абсолютный путь: нормализуем URI и склеиваем с корнем
    std::string normUri = normalizeUri(request.getUri()); // защита от "../"
    //  Отрезаем префикс локации (alias-логика)
    const std::string &prefix = location.prefix;
    // Удостоверимся, что prefix заканчивается на “/”, иначе поставим его
    std::string realPrefix = (!prefix.empty() && prefix[prefix.length() - 1] == '/') ? prefix : prefix + "/";

    std::string relPath;
    if (normUri.compare(0, realPrefix.length(), realPrefix) == 0)
        // Если URI начинается с нашего префикса
        relPath = normUri.substr(realPrefix.length() - 1); // чтобы оставить ведущий “/”
    else
        relPath = normUri; // На всякий случай, если не совпало — считаем весь URI относительным
    //  Склеиваем с root
    std::string absPath = root + relPath;

    // проверяем, что мы не выходим за пределы root
    if (absPath.rfind(root + "/", 0) != 0)
        return _createErrorResponse(403, &server); // на самом деле сюда не попадем, т.к. normalizeUri защищает от "../", но на всякий случай

    struct stat path_stat;
    if (stat(absPath.c_str(), &path_stat) != 0)
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
            std::string temp_path = absPath + (absPath[absPath.length() - 1] == '/' ? "" : "/") + index_files[i];
            if (access(temp_path.c_str(), F_OK) == 0)
            {
                found_index_path = temp_path;
                break;
            }
        }
        if (!found_index_path.empty())
        {
            absPath = found_index_path; // Нашли индекс, будем отдавать его
        }
        else if (location.autoindex)
        {
            std::string listing = generateAutoindex(absPath, normUri); // Нет index, но включён autoindex on → генерируем HTML-листинг
            HttpResponse response;
            response.setStatusCode(200);
            response.addHeader("Content-Type", "text/html; charset=utf-8");
            response.setBody(listing);
            return response;
        }
        else
        {
            return _createErrorResponse(403, &server); // index нет и autoindex off — 403
        }
    }

    if (access(absPath.c_str(), R_OK) != 0)
    {
        return _createErrorResponse(403, &server);
    }

    std::ifstream file(absPath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return _createErrorResponse(500, &server);
    }
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    HttpResponse response;
    response.setStatusCode(200);
    size_t pos = absPath.find_last_of(".");
    std::string ext = (pos != std::string::npos) ? absPath.substr(pos) : "";
    const std::map<std::string, std::string> &mimes = getMimeTypes();
    std::string mime = "application/octet-stream";
    if (mimes.count(ext))
        mime = mimes.at(ext);
    response.addHeader("Content-Type", mime);
    response.setBody(body);
    return response;
}

HttpResponse RequestHandler::_handlePost(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    std::string upload_dir;
    if (!location.upload_path.empty())
        upload_dir = location.upload_path;
    else if (!location.root.empty())
        upload_dir = location.root;
    else
        upload_dir = server.rootDef;

    if (upload_dir.empty()) // Если нет ни одной валидной директории ⇒ внутренняя ошибка конфигурации
        return _createErrorResponse(500, &server);

    std::string prefix = location.prefix; //   Очищаем prefix от лишнего «/» в конце, чтобы избежать двойных слэшей при формировании Location

    if (!prefix.empty() && prefix[prefix.size() - 1] == '/')
        prefix.erase(prefix.size() - 1);

    std::string rel = request.getUri(); // Вычленяем из полного URI часть после префикса локации:/uploads/myfile.bin → myfile.bin
    if (rel.rfind(prefix, 0) == 0)
        rel = rel.substr(prefix.length());
    if (!rel.empty() && rel[0] == '/')
        rel.erase(0, 1);

    std::string filename;
    if (!rel.empty() && rel.find('/') == std::string::npos) // Если rel не содержит «/», это простое имя файла:оставляем только буквы, цифры, '.', '-' и '_'
    {
        for (size_t i = 0; i < rel.size(); ++i)
        {
            char c = rel[i];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.')
                filename += c;
        }
    }

    if (filename.empty()) // Если клиент не задал имя → генерируем уникальное
    {
        static bool seeded = false;
        if (!seeded)
        {
            std::srand(std::time(NULL));
            seeded = true;
        }
        std::ostringstream oss;
        oss << std::time(NULL) << "_" << std::rand() << ".bin";
        filename = oss.str();
    }

    std::string full_path = upload_dir;
    if (!full_path.empty() && full_path[full_path.size() - 1] != '/')
        full_path += "/";
    full_path += filename; // Собираем полный путь до файла

    if (access(full_path.c_str(), F_OK) == 0)
    {
        std::string base = filename;
        std::string ext;
        size_t dot = filename.find_last_of('.');
        if (dot != std::string::npos)
        {
            base = filename.substr(0, dot);
            ext = filename.substr(dot);
        }
        int counter = 1;
        do
        {
            std::ostringstream oss;
            oss << base << "_" << counter << ext; // Если дубликация, добавляем счетчик
            filename = oss.str();
            full_path = upload_dir;
            if (!full_path.empty() && full_path[full_path.size() - 1] != '/')
                full_path += "/";
            full_path += filename;
            ++counter;
        } while (access(full_path.c_str(), F_OK) == 0);
    }

    std::ofstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return _createErrorResponse(500, &server);
    }
    file.write(request.getBody().c_str(), request.getBody().length());
    if (file.fail())
    {
        file.close();
        return _createErrorResponse(500, &server);
    }
    file.close();

    HttpResponse response;
    response.setStatusCode(201);
    std::string loc_hdr = prefix + "/" + filename;
    response.addHeader("Location", loc_hdr);
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

HttpResponse RequestHandler::_createErrorResponse(int statusCode, const ServerConfig *server, const std::vector<std::string> *allowed_methods)
{
    HttpResponse response;
    response.setStatusCode(statusCode);
    response.addHeader("Content-Type", "text/html; charset=utf-8"); // тут нужно уточнить, какой тип контента возвращаем
    if (statusCode == 405 && allowed_methods && !allowed_methods->empty())
    {
        std::string allow_header;
        for (size_t i = 0; i < allowed_methods->size(); ++i)
        {
            if (i > 0)
                allow_header += ", ";
            allow_header += (*allowed_methods)[i];
        }
        response.addHeader("Allow", allow_header);
    }

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
    return response;
}

const ServerConfig *RequestHandler::_findServerConfig(int port, const std::string &host) const
{
    if (!_config)
        return NULL;
    const std::vector<ServerConfig> &servers = _config->getServers(); // Миш, мне тут нужен геттер типа return this->_configServ
    const ServerConfig *default_server_for_port = NULL;

    for (size_t i = 0; i < servers.size(); ++i)
    {
        bool port_match = false;
        // Проверяем, слушает ли текущий сервер нужный порт
        for (size_t j = 0; j < servers[i].listen.size(); ++j)
        {
            if (servers[i].listen[j].port == port)
            {
                port_match = true;
                // Если это первый сервер, который мы нашли для этого порта,
                // запоминаем его как дефолтный на случай, если не найдем точного совпадения по хосту.
                if (!default_server_for_port)
                {
                    default_server_for_port = &servers[i];
                }
                break;
            }
        }

        if (!port_match)
        {
            continue; // Этот сервер не на нашем порту, пропускаем
        }

        // Если порт совпал, проверяем server_name
        for (size_t j = 0; j < servers[i].server_name.size(); ++j)
        {
            if (servers[i].server_name[j] == host)
            {
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
const LocationStruct *RequestHandler::_findLocationFor(const ServerConfig &server, const std::string &uri) const
{
    const LocationStruct *best_match = NULL;
    size_t max_len = 0;

    for (size_t i = 0; i < server.location.size(); ++i)
    {
        const std::string &prefix = server.location[i].prefix;

        // Проверяем, что URI начинается с префикса локации.
        // rfind(prefix, 0) == 0 - это эффективный способ проверить "startsWith".
        if (uri.rfind(prefix, 0) == 0)
        {
            // Если длина текущего префикса больше, чем у лучшего найденного ранее,
            // то это новое лучшее совпадение.
            if (prefix.length() > max_len)
            {
                max_len = prefix.length();
                best_match = &server.location[i];
            }
        }
    }

    // Возвращаем указатель на локацию с самым длинным совпадением префикса.
    // Если ничего не совпало, вернется NULL.
    return best_match;
}
