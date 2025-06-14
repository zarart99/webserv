#include "HttpRequest.hpp"


static std::string trim(const std::string &s)
{
    const std::string WHITESPACE = " \t\r\n";
    size_t first = s.find_first_not_of(WHITESPACE);
    if (std::string::npos == first)
    {
        return "";
    }
    size_t last = s.find_last_not_of(WHITESPACE);
    return s.substr(first, (last - first + 1));
}

bool HttpRequest::isValid() const {
    return true;
}

// Приводит строку к нижнему регистру
static std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), static_cast<int (*)(int)>(std::tolower));
    return s;
}



HttpRequest::HttpRequest(const std::string &raw_request)
{

    _parse(raw_request);
}

HttpRequest::~HttpRequest() {}

void HttpRequest::_parse(const std::string &raw_request)
{
    std::stringstream request_stream(raw_request);

    _parseRequestLine(request_stream);

    _parseHeaders(request_stream);

    _parseBody(request_stream);
}


void HttpRequest::_parseRequestLine(std::stringstream &request_stream)
{
    std::string request_line;
    if (!std::getline(request_stream, request_line) || request_line.empty())
    {
        throw std::runtime_error("400 Bad Request: Empty request");
    }

    if (!request_line.empty() && request_line[request_line.size() - 1] == '\r')
    {
        request_line = request_line.substr(0, request_line.size() - 1);
    }

    std::stringstream line_stream(request_line);
    if (!(line_stream >> _method >> _uri >> _http_version))
    {
        throw std::runtime_error("400 Bad Request: Malformed request line. Expected 'METHOD URI VERSION'");
    }

    std::string extra;
    if (line_stream >> extra) {
        throw std::runtime_error("400 Bad Request");
    }

    if (_method != "GET" && _method != "POST" && _method != "DELETE")
    {
        throw std::runtime_error("501 Not Implemented: Unsupported method '" + _method + "'");
    }
    if (_http_version != "HTTP/1.1")
    {
        throw std::runtime_error("505 HTTP Version Not Supported: Server only accepts HTTP/1.1");
    }

    if (_uri.empty() || _uri[0] != '/') {
        throw std::runtime_error("400 Bad Request");
    }
}

void HttpRequest::_parseHeaders(std::stringstream &request_stream)
{
    std::string header_line;

    while (std::getline(request_stream, header_line) && !header_line.empty() && header_line != "\r")
    {
        if (header_line[header_line.size() - 1] == '\r')
        {
            header_line = header_line.substr(0, header_line.size() - 1);
        }

        std::string::size_type colon_pos = header_line.find(':');
        if (colon_pos == std::string::npos)
        {
            continue;
        }

        std::string key = header_line.substr(0, colon_pos);
        std::string value = header_line.substr(colon_pos + 1);

        _headers[toLower(trim(key))] = trim(value);
    }
}

void HttpRequest::_parseBody(std::stringstream &request_stream)
{
 
    if (!request_stream.eof())
    {
        _body.assign(std::istreambuf_iterator<char>(request_stream), std::istreambuf_iterator<char>());
    }
}

size_t HttpRequest::getContentLength() const {
    // Ищем заголовок "content-length" (ключи у нас уже в нижнем регистре)
    std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");

    // Если заголовок не найден, возвращаем 0
    if (it == _headers.end()) {
        return 0;
    }

    // Конвертируем значение заголовка из строки в число
    char* end;
    long length = std::strtol(it->second.c_str(), &end, 10);

    // Проверяем на ошибки конвертации или отрицательные значения
    if (*end != '\0' || length < 0) {
        return 0; // Некорректное значение
    }
    
    return static_cast<size_t>(length);
}


std::string HttpRequest::getMethod() const { return _method; }
std::string HttpRequest::getUri() const { return _uri; }
std::string HttpRequest::getHttpVersion() const { return _http_version; }
const std::map<std::string, std::string> &HttpRequest::getHeaders() const { return _headers; }
std::string HttpRequest::getBody() const { return _body; }
