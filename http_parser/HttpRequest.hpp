// HttpRequest.hpp
#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include "HttpEnums.hpp"

struct HttpRequest {
    HttpMethod method;
    std::string uri;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    // По умолчанию — UNKNOWN-метод, пустые остальные поля
    HttpRequest()
        : method(UNKNOWN), uri(""), version(""), body("") {}
};

#endif // HTTP_REQUEST_HPP
