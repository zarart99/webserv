#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse() : _statusCode(200), _statusMessage("OK") {}

HttpResponse::HttpResponse(const HttpResponse &src)
{
    *this = src;
}

HttpResponse &HttpResponse::operator=(const HttpResponse &src)
{
    if (this != &src)
    {
        _statusCode = src._statusCode;
        _statusMessage = src._statusMessage;
        _headers = src._headers;
        _body = src._body;
    }
    return *this;
}
HttpResponse::~HttpResponse() {}

void HttpResponse::setStatusMessage(const std::string &message) { _statusMessage = message; }
void HttpResponse::addHeader(const std::string &key, const std::string &value) { _headers[key] = value; }
void HttpResponse::setBody(const std::string &body) { _body = body; }

// определение статической функции класса
std::map<int, std::string> HttpResponse::initStatusMessages()
{
    std::map<int, std::string> m;
    m[200] = "OK";
    m[201] = "Created";
    m[204] = "No Content";
    m[301] = "Moved Permanently";
    m[302] = "Found";
    m[400] = "Bad Request";
    m[404] = "Not Found";
    m[405] = "Method Not Allowed";
    m[414] = "URI Too Long";
    m[431] = "Request Header Fields Too Large";
    m[413] = "Payload Too Large";
    m[500] = "Internal Server Error";
    m[501] = "Not Implemented";
    m[505] = "HTTP Version Not Supported";
    return m;
}

// инициализатор статического поля через метод класса
std::map<int, std::string> HttpResponse::_statusMessages = HttpResponse::initStatusMessages();

const std::map<int, std::string> &HttpResponse::getStatusMessages()
{
    return _statusMessages;
}

void HttpResponse::setStatusCode(int code)
{
    _statusCode = code;
    // автоматически подставляем текст
    std::map<int, std::string>::iterator it = _statusMessages.find(code);
    _statusMessage = (it != _statusMessages.end()) ? it->second : "Unknown";
}

std::string HttpResponse::buildResponse() const
{
    std::stringstream response_ss;

    response_ss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

    std::map<std::string, std::string> headers_copy = _headers;
    if (!_body.empty() && headers_copy.find("Content-Length") == headers_copy.end())
    {
        std::stringstream body_size_ss;
        body_size_ss << _body.length();
        headers_copy["Content-Length"] = body_size_ss.str();
    }

    for (std::map<std::string, std::string>::const_iterator it = headers_copy.begin(); it != headers_copy.end(); ++it)
    {
        response_ss << it->first << ": " << it->second << "\r\n";
    }

    response_ss << "\r\n";
    response_ss << _body;

    return response_ss.str();
}

// HttpResponse handleHttpRequest(const std::string& raw_request) {
//     HttpRequest req(raw_request);
//     HttpResponse res;

//    if (!req.isValid()) {
//        res.setStatusCode(400);
//        res.setStatusMessage("Bad Request");
//        res.setBody("Cannot parse request");
//        return res;
//    }

//    if (req.getMethod() == "GET") {
//        if (req.getUri() == "/") {
//            res.setStatusCode(200);
//            res.setStatusMessage("OK");
//            res.setBody("Welcome to webserv!\n");
//        } else {
//            res.setStatusCode(404);
//            res.setStatusMessage("Not Found");
//            res.setBody("The requested resource was not found.");
//        }
//    } else if (req.getMethod() == "POST") {
//        res.setStatusCode(200);
//        res.setStatusMessage("OK");
//        res.setBody("POST request received.");
//    } else if (req.getMethod() == "DELETE") {
//        res.setStatusCode(200);
//        res.setStatusMessage("OK");
//        res.setBody("DELETE acknowledged.");
//    } else {
//        res.setStatusCode(405);
//        res.setStatusMessage("Method Not Allowed");
//        res.setBody("Unsupported method.");
//    }

//    return res;
//}
