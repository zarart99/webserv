#include "RequestHandler.hpp"
#include "HttpResponse.hpp"
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
#include <map>
#include <vector>
#include <limits.h>

std::vector<MultipartPart> parseMultipart(const std::string &rawBody, const std::string &boundary)
{
    std::vector<MultipartPart> parts;
    std::string delim = "--" + boundary;
    size_t pos = rawBody.find(delim);
    if (pos == std::string::npos)
        throw std::runtime_error("no boundary");
    while (true)
    {
        if (rawBody.compare(pos, delim.size(), delim) != 0)
            throw std::runtime_error("bad boundary");
        pos += delim.size();
        if (rawBody.compare(pos, 2, "--") == 0)
            break; // reached closing boundary
        if (rawBody.compare(pos, 2, "\r\n") == 0)
            pos += 2;

        size_t header_end = rawBody.find("\r\n\r\n", pos);
        if (header_end == std::string::npos)
            throw std::runtime_error("bad headers");
        std::string header_block = rawBody.substr(pos, header_end - pos);
        pos = header_end + 4;

        MultipartPart part;
        part.name = "";
        part.filename = "";

        std::stringstream hs(header_block);
        std::string line;
        bool cd_found = false;
        while (std::getline(hs, line))
        {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            size_t colon = line.find(':');
            if (colon == std::string::npos)
                throw std::runtime_error("bad header line");
            std::string key = toLower(trim(line.substr(0, colon)));
            std::string value = trim(line.substr(colon + 1));
            if (key == "content-disposition")
            {
                cd_found = true;
                size_t name_pos = value.find("name=");
                if (name_pos != std::string::npos)
                {
                    name_pos += 5;
                    if (value[name_pos] == '"')
                    {
                        ++name_pos;
                        size_t end = value.find('"', name_pos);
                        if (end == std::string::npos)
                            throw std::runtime_error("bad name");
                        part.name = value.substr(name_pos, end - name_pos);
                    }
                }
                size_t fn_pos = value.find("filename=");
                if (fn_pos != std::string::npos)
                {
                    fn_pos += 9;
                    if (value[fn_pos] == '"')
                    {
                        ++fn_pos;
                        size_t end = value.find('"', fn_pos);
                        if (end == std::string::npos)
                            throw std::runtime_error("bad filename");
                        part.filename = value.substr(fn_pos, end - fn_pos);
                    }
                }
            }
        }
        if (!cd_found || part.name.empty())
            throw std::runtime_error("no content-disposition");

        size_t next = rawBody.find(delim, pos);
        if (next == std::string::npos)
            throw std::runtime_error("no closing boundary");
        size_t body_len = next - pos;
        if (body_len >= 2 && rawBody.compare(next - 2, 2, "\r\n") == 0)
            body_len -= 2;
        part.body = rawBody.substr(pos, body_len);
        parts.push_back(part);

        pos = next;
    }
    return parts;
}

RequestHandler::RequestHandler() : _config(NULL) {}

RequestHandler::RequestHandler(const ConfigParser &config) : _config(&config) {}

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

HttpResponse RequestHandler::handleRequest(const HttpRequest &request, int serverPort, const std::string &serverIp, const std::string &serverHost)
{

    const ServerConfig *server_config = NULL;
    const LocationStruct *location_config = NULL;

    try
    {
        server_config = _findServerConfig(serverPort, serverIp, serverHost);

        // Проверяем, 400
        if (!server_config)
        {
            return _createErrorResponse(400, NULL, NULL, NULL);
        }

        location_config = _findLocationFor(*server_config, request.getUri());

        // Проверяем, 404

        if (!location_config)
        {
            return _createErrorResponse(404, server_config, NULL);
        }

        // После поиска подходящего location — проверяем, нужно ли делать редирект
        if (!location_config->redir.empty())
        {
            std::map<int, std::string>::const_iterator it = location_config->redir.begin();
            int code = it->first;                // код редиректа или ошибки
            const std::string &url = it->second; // URL или пустая строка

            if (!url.empty())
            {
                // Это редирект на полный URL
                HttpResponse res;
                res.setStatusCode(code);        // 301, 302 и т.д.
                res.addHeader("Location", url); // браузер перейдёт по этой ссылке
                return res;
            }
            else if (code >= 400 && code < 600)
            {
                // Только код ошибки → рисуем страницу через общий генератор
                return _createErrorResponse(code, server_config, NULL, location_config);
            }
        }

        // Проверяем, 413

        size_t limit_in_bytes = location_config->client_max_body_size;
        size_t content_length = request.getContentLength();
        if (limit_in_bytes > 0 && content_length > limit_in_bytes)
        {
            return _createErrorResponse(413, server_config, NULL, location_config);
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
                return _createErrorResponse(405, server_config, &allowed_methods, location_config);
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

        return _createErrorResponse(statusCode, server_config, NULL, location_config);
    }

    return _createErrorResponse(501, server_config, NULL, location_config); // Если метод не реализован, возвращаем 501 Not Implemented
}

HttpResponse RequestHandler::_handleGet(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    // Определяем корневую директорию: берем из location, если есть, иначе из server
    std::string root = !location.root.empty()
                           ? location.root
                           : server.rootDef;
    // Если путь относительный – приводим к абсолютному через getcwd()
    if (!root.empty())
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)))
            root = std::string(cwd) + root;
    }
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
        return _createErrorResponse(403, &server, NULL, &location); // на самом деле сюда не попадем, т.к. normalizeUri защищает от "../", но на всякий случай

    struct stat path_stat;
    if (stat(absPath.c_str(), &path_stat) != 0)
    {
        return _createErrorResponse(404, &server, NULL, &location); // Путь не существует
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
            return _createErrorResponse(403, &server, NULL, &location); // index нет и autoindex off — 403
        }
    }

    if (access(absPath.c_str(), R_OK) != 0)
    {
        return _createErrorResponse(403, &server, NULL, &location);
    }

    std::ifstream file(absPath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return _createErrorResponse(500, &server, NULL, &location); // Ошибка открытия файла
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
    std::map<std::string, std::string>::const_iterator it = request.getHeaders().find("content-type");
    if (it != request.getHeaders().end())
    {
        std::string ct_lower = toLower(it->second);
        if (ct_lower.find("multipart/form-data") == 0 && ct_lower.find("boundary=") != std::string::npos)
            return _handleMultipart(request, location, server);
    }
    return _handleSimplePost(request, location, server);
}

std::string RequestHandler::_getStorageDir(const LocationStruct &location, const ServerConfig &server) const
{
    std::string dir;
    if (!location.upload_path.empty())
        dir = location.upload_path;
    else if (!location.root.empty())
        dir = location.root;
    else
        dir = server.rootDef;

    if (dir.empty())
        return "";

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)))
        return std::string(cwd) + dir;
    return "";
}

HttpResponse RequestHandler::_handleSimplePost(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    std::string storageDir = _getStorageDir(location, server);
    if (storageDir.empty())
        return _createErrorResponse(500, &server, NULL, &location);

    std::string prefix = location.prefix;
    if (!prefix.empty() && prefix[prefix.size() - 1] == '/')
        prefix.erase(prefix.size() - 1);

    std::string rel = request.getUri();
    if (rel.rfind(prefix, 0) == 0)
        rel = rel.substr(prefix.length());
    if (!rel.empty() && rel[0] == '/')
        rel.erase(0, 1);

    std::string filename = saveBodyToFile(request.getBody(), rel, storageDir);
    if (filename.empty())
        return _createErrorResponse(500, &server, NULL, &location);

    HttpResponse response;
    response.setStatusCode(201);
    response.addHeader("Location", prefix + "/" + filename);
    return response;
}

HttpResponse RequestHandler::_handleMultipart(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    std::string storageDir = _getStorageDir(location, server);
    if (storageDir.empty())
        return _createErrorResponse(500, &server, NULL, &location);
    std::map<std::string, std::string>::const_iterator it = request.getHeaders().find("content-type");
    if (it == request.getHeaders().end())
        return _createErrorResponse(400, &server, NULL, &location); // Если нет content-type, возвращаем 400 Bad Request

    std::string ct = it->second;
    std::string lower = toLower(ct);
    size_t bpos = lower.find("boundary="); // Извлекаем параметр boundary из заголовка
    if (bpos == std::string::npos)
        return _createErrorResponse(400, &server, NULL, &location); // Если нет boundary, возвращаем 400 Bad Request
    std::string boundary = ct.substr(bpos + 9);
    boundary = trim(boundary);
    if (!boundary.empty() && boundary[0] == '"') // — если boundary в кавычках, убираем их
    {
        boundary.erase(0, 1);
        size_t q = boundary.find('"');
        if (q != std::string::npos)
            boundary = boundary.substr(0, q);
    }
    else // — если после boundary идут дополнительные параметры, отсекаем всё после ';'
    {
        size_t sc = boundary.find(';');
        if (sc != std::string::npos)
            boundary = boundary.substr(0, sc);
    }
    if (boundary.empty())
        return _createErrorResponse(400, &server, NULL, &location); // Если boundary пустое, возвращаем 400 Bad Request

    std::vector<MultipartPart> parts; // Разбираем всё тело на отдельные части по найденному boundary
    try
    {
        parts = parseMultipart(request.getBody(), boundary);
    }
    catch (const std::exception &)
    {
        return _createErrorResponse(400, &server, NULL, &location); // Если ошибка при разборе multipart, возвращаем 400 Bad Request
    }

    std::vector<std::string> uploadedFiles; // Контейнеры: для сохранённых файлов и для полей формы
    std::map<std::string, std::string> formFields;

    for (size_t i = 0; i < parts.size(); ++i) // Обрабатываем каждую часть multipart

    {
        if (!parts[i].filename.empty()) // Часть с filename — значит, это файл. Сохраняем его.
        {
            std::string fname = saveBodyToFile(parts[i].body, parts[i].filename, storageDir);
            if (fname.empty())
                return _createErrorResponse(500, &server, NULL, &location); // Ошибка сохранения файла
            std::string prefix = location.prefix;                           // Формируем URL доступа (удаляем хвостовой '/')

            if (!prefix.empty() && prefix[prefix.size() - 1] == '/')
                prefix.erase(prefix.size() - 1);
            uploadedFiles.push_back(prefix + "/" + fname);
        }
        else // Часть без filename — текстовое поле формы. Сохраняем в map
        {
            formFields[parts[i].name] = parts[i].body;
        }
    }

    HttpResponse response;
    if (uploadedFiles.size() == 1 && formFields.empty()) // Если только один файл и нет полей формы, возвращаем 201 с Location
    {
        response.setStatusCode(201);
        response.addHeader("Location", uploadedFiles[0]);
        return response;
    }

    response.setStatusCode(200); // Если есть и файлы, и поля формы, возвращаем 200 с JSON-ответом
    response.addHeader("Content-Type", "application/json; charset=utf-8");
    std::stringstream body;
    body << "{\n  \"files\": [";
    for (size_t i = 0; i < uploadedFiles.size(); ++i)
    {
        if (i)
            body << ", ";
        body << "\"" << uploadedFiles[i] << "\"";
    }
    body << "],\n  \"fields\": {";
    for (std::map<std::string, std::string>::const_iterator mit = formFields.begin(); mit != formFields.end();)
    {
        body << "\"" << mit->first << "\": \"" << mit->second << "\"";
        if (++mit != formFields.end())
            body << ", ";
    }
    body << "}\n}";
    response.setBody(body.str());
    return response;
}

std::string RequestHandler::saveBodyToFile(const std::string &body, const std::string &suggestedName,
                                           const std::string &storageDir)
{
    std::string dir = storageDir;
    if (dir.empty())
        return "";

    std::string filtered;
    for (size_t i = 0; i < suggestedName.size(); ++i)
    {
        char c = suggestedName[i];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-')
            filtered += c;
    }

    std::string filename = filtered;
    std::string ext;
    size_t dot = suggestedName.find_last_of('.');
    if (dot != std::string::npos)
        ext = suggestedName.substr(dot);

    if (filename.empty())
    {
        static bool seeded = false;
        if (!seeded)
        {
            std::srand(std::time(NULL));
            seeded = true;
        }
        std::ostringstream oss;
        oss << std::time(NULL) << "_" << std::rand();
        if (!ext.empty())
            oss << ext;
        else
            oss << ".bin";
        filename = oss.str();
    }

    std::string path = dir;
    if (!path.empty() && path[path.size() - 1] != '/')
        path += "/";
    path += filename;

    if (access(path.c_str(), F_OK) == 0)
    {
        std::string base = filename;
        std::string extension;
        size_t d = filename.find_last_of('.');
        if (d != std::string::npos)
        {
            base = filename.substr(0, d);
            extension = filename.substr(d);
        }
        int counter = 1;
        do
        {
            std::ostringstream oss;
            oss << base << "_" << counter << extension;
            filename = oss.str();
            path = dir;
            if (!path.empty() && path[path.size() - 1] != '/')
                path += "/";
            path += filename;
            ++counter;
        } while (access(path.c_str(), F_OK) == 0);
    }

    std::ofstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    file.write(body.c_str(), body.size());
    if (file.fail())
    {
        file.close();
        return "";
    }
    file.close();
    return filename;
}

HttpResponse RequestHandler::_handleDelete(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server)
{
    std::string storageDir = _getStorageDir(location, server);
    if (storageDir.empty())
        return _createErrorResponse(500, &server, NULL, &location);
    // 2) Обрезаем префикс локации, чтобы не дублировать /uploads
    std::string prefix = location.prefix;
    if (!prefix.empty() && prefix[prefix.size() - 1] == '/')
        prefix.erase(prefix.size() - 1);
    std::string rel = request.getUri();  // "/uploads/test.png"
    if (rel.rfind(prefix, 0) == 0)       // если начинается с "/uploads"
        rel = rel.substr(prefix.size()); // → "/test.png"
    if (!rel.empty() && rel[0] == '/')
        rel.erase(0, 1); // → "test.png"

    // 3) Собираем настоящий полный путь
    std::string path = storageDir;
    if (!path.empty() && path[path.size() - 1] != '/')
        path += "/";
    path += rel; // "/www/html/uploads/test.png"

    if (access(path.c_str(), F_OK) == -1)
    {
        return _createErrorResponse(404, &server, NULL, &location); // Путь не существует
    }
    if (access(path.c_str(), W_OK) == -1)
    {
        return _createErrorResponse(403, &server, NULL, &location); // Нет прав на запись/удаление
    }

    if (std::remove(path.c_str()) != 0)
    {
        return _createErrorResponse(500, &server, NULL, &location); // Системная ошибка при удалении
    }

    HttpResponse response;
    response.setStatusCode(204); // No Content
    return response;
}

HttpResponse RequestHandler::_createErrorResponse(int statusCode,
                                                  const ServerConfig *server,
                                                  const std::vector<std::string> *allowed_methods,
                                                  const LocationStruct *location)
{
    HttpResponse response;
    response.setStatusCode(statusCode);
    response.addHeader("Content-Type", "text/html; charset=utf-8");
    // Для 405 – заголовок Allow
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

    // 1) Пытаемся найти кастомную страницу в location.error_page
    std::string pagePath;
    if (location)
    {
        std::map<int, std::string>::const_iterator itLoc =
            location->error_page.find(statusCode);
        if (itLoc != location->error_page.end())
        {
            pagePath = itLoc->second;
        }
    }

    // 2) Если не нашли в локации — смотрим server->error_pageDef
    if (pagePath.empty() && server)
    {
        std::map<int, std::string>::const_iterator itSrv =
            server->error_pageDef.find(statusCode);
        if (itSrv != server->error_pageDef.end())
        {
            pagePath = itSrv->second;
        }
    }

    // 3) Если есть путь к HTML — пробуем его отдать
    if (!pagePath.empty())
    {
        // собираем тот же root, что и в GET
        std::string root = (location && !location->root.empty())
                               ? location->root
                               : server->rootDef;
        if (!root.empty())
        {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)))
                fullPath = std::string(cwd) + root;
        }
        std::string fullPath = root + pagePath; // <-- теперь это "<cwd>/www/html/error/404.html"

        std::ifstream file(fullPath.c_str());
        if (file.is_open())
        {
            std::string body((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
            response.setBody(body);
            return response;
        }
    }
    // 4) Генерим простую HTML-страницу, беря текст из HttpResponse::getStatusMessages()
    const std::map<int, std::string> &statusMsgs = HttpResponse::getStatusMessages();
    std::map<int, std::string>::const_iterator itStatus =
        statusMsgs.find(statusCode);
    std::string reason;
    if (itStatus != statusMsgs.end())
        reason = itStatus->second;
    else
        reason = "";

    std::ostringstream oss;
    oss << "<html><head><title>"
        << statusCode << " " << reason
        << "</title></head><body><h1>"
        << statusCode << " " << reason
        << "</h1><hr><em>webserv - Karandashi </em></body></html>";

    response.setBody(oss.str());
    return response;
}

const ServerConfig *RequestHandler::_findServerConfig(int port, const std::string &ip, const std::string &host) const
{
    if (!_config)
        return NULL;
    const std::vector<ServerConfig> &servers = _config->getServers();
    const ServerConfig *default_server_for_port = NULL;
    bool isDefault = false;

    for (std::vector<ServerConfig>::const_iterator it = servers.begin(); it != servers.end(); it++)
    {
        for (size_t i = 0; i < it->listen.size(); i++)
        {
            if (it->listen[i].port == port) // Сначала сравниваем по port
            {
                if (it->listen[i].ip == ip || it->listen[i].ip == "0.0.0.0") // Если Ip одинаковые либо в листен прописан 0.0.0.0 то все ок идем дальше
                {
                    for (size_t i_2 = 0; i_2 < it->server_name.size(); i_2++) // Ищем похожиее имя домена
                    {
                        if (it->server_name[i_2] == host) // Если находим то это наш сервер
                            return &(*it);
                    }
                    if (!isDefault)
                    {
                        default_server_for_port = &(*it); // Первый сервер с совпадением по IP/PORT становиться дефолтным, отправляем его если нет совпадений по имени домена
                        isDefault = true;
                    }
                    break;
                }
            }
        }
    }
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
