// HttpEnums.hpp
#ifndef HTTP_ENUMS_HPP
#define HTTP_ENUMS_HPP

// Методы HTTP-запроса
enum HttpMethod {
    GET,
    POST,
    DELETE,
    UNKNOWN
};

// Статусы HTTP-ответа
enum HttpStatus {
    OK = 200,
    Created = 201,
    NoContent = 204,
    BadRequest = 400,
    NotFound = 404,
    MethodNotAllowed = 405,
    InternalServerError = 500
};

#endif // HTTP_ENUMS_HPP
