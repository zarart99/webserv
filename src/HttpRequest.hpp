#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <stdexcept>
#include <sstream>
#include <algorithm> // для std::transform
#include <cctype>    // для ::tolower
#include <iostream>  // для вывода ошибок

class HttpRequest
{
public:
    HttpRequest(const std::string &raw_request);
    ~HttpRequest();

    std::string getMethod() const;
    std::string getUri() const;
    std::string getHttpVersion() const;
    const std::map<std::string, std::string> &getHeaders() const;
    std::string getBody() const;
    bool isValid() const;

private:
    std::string _method;
    std::string _uri;
    std::string _http_version;
    std::map<std::string, std::string> _headers;
    std::string _body;

    void _parse(const std::string &raw_request);
    void _parseRequestLine(std::stringstream &request_stream);
    void _parseHeaders(std::stringstream &request_stream);
    void _parseBody(std::stringstream &request_stream);
};

#endif // HTTPREQUEST_HPP
