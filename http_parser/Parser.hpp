// Parser.hpp
#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

// Разбирает «сырое» содержимое HTTP-запроса и возвращает структуру HttpRequest.
// Пока возвращает пустой HttpRequest.
HttpRequest parseRequest(const std::string& raw);

// Генерирует HTTP-ответ с кодом 400 Bad Request.
HttpResponse makeResponse(const HttpRequest& req);

#endif // HTTP_PARSER_HPP
