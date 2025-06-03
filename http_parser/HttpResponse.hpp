// HttpResponse.hpp
#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>
#include <vector>
#include "HttpEnums.hpp"

struct HttpResponse {
    HttpStatus statusCode;
    std::string reasonPhrase;
    std::map<std::string, std::string> headers;
    std::vector<char> bodyBuffer;

    // По умолчанию — 500 InternalServerError, пустые остальные поля
    HttpResponse()
        : statusCode(InternalServerError), reasonPhrase("") {}
};

#endif // HTTP_RESPONSE_HPP
