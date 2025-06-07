#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void setStatusCode(int code);
    void setStatusMessage(const std::string &message);
    void addHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);

    std::string buildResponse() const;

private:
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
};

#endif // HTTPRESPONSE_HPP
