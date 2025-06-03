// Parser.cpp
#include "Parser.hpp"
#include <sstream>
#include <algorithm>

// Вспомогательная функция: преобразует строку вида "GET" в enum HttpMethod
static HttpMethod parseMethod(const std::string& s) {
    if (s == "GET")     return GET;
    if (s == "POST")    return POST;
    if (s == "DELETE")  return DELETE;
    return UNKNOWN;
}

// Вспомогательная функция: убирает пробелы слева и справа
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

HttpRequest parseRequest(const std::string& raw) {
    HttpRequest req;
    // Найдём конец заголовков ("\r\n\r\n")
    size_t headers_end = raw.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        // Ещё не полный запрос — оставляем UNKNOWN
        return req;
    }

    // 1. Разбор старт-строки (до "\r\n")
    size_t line_end = raw.find("\r\n");
    if (line_end == std::string::npos) {
        return req;
    }
    std::string start_line = raw.substr(0, line_end);
    std::istringstream iss(start_line);
    std::string method_str, uri, version;
    iss >> method_str >> uri >> version;
    req.method = parseMethod(method_str);
    req.uri = uri;
    req.version = version;

    // 2. Разбор заголовков (между первой "\r\n" и "\r\n\r\n")
    size_t pos = line_end + 2; // сразу после "\r\n"
    while (pos < headers_end) {
        size_t next_end = raw.find("\r\n", pos);
        if (next_end == std::string::npos || next_end == pos) {
            break;
        }
        std::string header_line = raw.substr(pos, next_end - pos);
        size_t colon = header_line.find(':');
        if (colon != std::string::npos) {
            std::string key = trim(header_line.substr(0, colon));
            std::string value = trim(header_line.substr(colon + 1));
            req.headers[key] = value;
        }
        pos = next_end + 2;
    }

    // 3. Тело (если есть Content-Length)
    std::string cl = "";
    if (req.headers.count("Content-Length")) {
        cl = req.headers["Content-Length"];
    }
    if (!cl.empty()) {
        int length = std::atoi(cl.c_str());
        size_t body_start = headers_end + 4;
        if (raw.size() >= body_start + static_cast<size_t>(length)) {
            req.body = raw.substr(body_start, length);
        }
    }

    return req;
}

HttpResponse makeResponse(const HttpRequest& req) {
    (void)req; 
    HttpResponse resp;
    resp.statusCode   = BadRequest;
    resp.reasonPhrase = "Bad Request";
    resp.headers["Content-Length"] = "0";
    resp.headers["Connection"]     = "close";
    return resp;
}
