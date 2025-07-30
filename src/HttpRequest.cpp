#include "HttpRequest.hpp"
#include <cstdlib>
#include <sstream>


bool HttpRequest::isValid() const
{
    return true;
}


// разбираем чанки
static std::string decodeChunked(const std::string &in)
{
    std::string out;
    size_t pos = 0;
    while (true)
    {
        size_t eol = in.find("\r\n", pos);
        if (eol == std::string::npos)
            throw std::runtime_error("incomplete chunk");
        size_t len = std::strtol(in.substr(pos, eol - pos).c_str(), NULL, 16); // hex размер
        pos = eol + 2;
        if (len == 0)
            break; // конец чанков
        if (in.size() < pos + len + 2)
            throw std::runtime_error("incomplete data");
        out.append(in, pos, len);
        pos += len + 2;
    }
    return out;
}

HttpRequest::HttpRequest() {}

HttpRequest::HttpRequest(const HttpRequest &src)
{
    *this = src;
}

HttpRequest::HttpRequest(const std::string &raw_request)
{

    _parse(raw_request);
}

HttpRequest &HttpRequest::operator=(const HttpRequest &src)
{
    if (this != &src)
    {
        _method = src._method;
        _uri = src._uri;
        _http_version = src._http_version;
        _headers = src._headers;
        _body = src._body;
    }
    return *this;
}

HttpRequest::~HttpRequest() {}

void HttpRequest::_parse(const std::string &raw_request)
{
    static const size_t MAX_HEADER_SIZE = 8192;
    size_t header_end = raw_request.find("\r\n\r\n");
    if (header_end == std::string::npos)
        throw std::runtime_error("400 Bad Request");
    if (header_end > MAX_HEADER_SIZE)
        throw std::runtime_error("431 Request Header Fields Too Large");

    std::stringstream request_stream(raw_request);

    _parseRequestLine(request_stream);

    _parseHeaders(request_stream);

    _parseBody(request_stream);
}

void HttpRequest::_parseRequestLine(std::stringstream &request_stream)
{
    static const size_t MAX_URI_LENGTH = 8192;
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
    std::string rawUri;
    if (!(line_stream >> _method >> rawUri >> _http_version))
    {
        throw std::runtime_error("400 Bad Request: Malformed request line. Expected 'METHOD URI VERSION'");
    }

    if (rawUri.size() > MAX_URI_LENGTH)
        throw std::runtime_error("414 URI Too Long");

    // Если клиент прислал абсолютный URI ("http://host[:port]/path"), обрезаем до "/path"
    if (rawUri.rfind("http://", 0) == 0 || rawUri.rfind("https://", 0) == 0)
    {
        // находим первый слэш после "http://"
        size_t slash = rawUri.find('/', rawUri.find("://") + 3);
        _uri = (slash != std::string::npos ? rawUri.substr(slash) : "/");
    }
    else         // иначе оставляем тот же относительный URI
        _uri = rawUri;

    std::string extra;
    if (line_stream >> extra)
    {
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

    if (_uri.empty() || _uri[0] != '/')
    {
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
            throw std::runtime_error("400 Bad Request: malformed header");
        }

        std::string key = trim(header_line.substr(0, colon_pos));
        std::string value = trim(header_line.substr(colon_pos + 1));

        // Проверяем корректность имени заголовка (только буквы, цифры и -)
        if (key.empty() || key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-") != std::string::npos)
        {
            throw std::runtime_error("400 Bad Request: invalid header name");
        }

        std::string lower_key = toLower(key);
        if (_headers.count(lower_key))
        {
            throw std::runtime_error("400 Bad Request: duplicate header");
        }

        _headers[lower_key] = value;
    }

    if (_http_version == "HTTP/1.1" && !_headers.count("host"))
    {
        throw std::runtime_error("400 Bad Request: Host header required");
    }
}

void HttpRequest::_parseBody(std::stringstream &request_stream)
{
    std::string raw((std::istreambuf_iterator<char>(request_stream)), std::istreambuf_iterator<char>());
    if (_headers.count("transfer-encoding") && _headers["transfer-encoding"] == "chunked")
    {
        _body = decodeChunked(raw);
        _headers.erase("transfer-encoding"); // убираем заголовок
        std::ostringstream ss;
        ss << _body.size();
        _headers["content-length"] = ss.str(); // новый размер
    }
    else
    {
        _body = raw;
        size_t declared_length = getContentLength();
        if (declared_length > 0 && declared_length != _body.size())
            throw std::runtime_error("400 Bad Request: Content-Length mismatch");
    }
}

size_t HttpRequest::getContentLength() const
{
    // Ищем заголовок "content-length" (ключи у нас уже в нижнем регистре)
    std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");

    // Если заголовок не найден, возвращаем 0
    if (it == _headers.end())
    {
        return 0;
    }

    // Конвертируем значение заголовка из строки в число
    char *end;
    long length = std::strtol(it->second.c_str(), &end, 10);

    // Проверяем на ошибки конвертации или отрицательные значения
    if (*end != '\0' || length < 0)
    {
        return 0; // Некорректное значение
    }

    return static_cast<size_t>(length);
}

std::string HttpRequest::getMethod() const { return _method; }
std::string HttpRequest::getUri() const { return _uri; }
std::string HttpRequest::getHttpVersion() const { return _http_version; }
const std::map<std::string, std::string> &HttpRequest::getHeaders() const { return _headers; }
std::string HttpRequest::getBody() const { return _body; }
