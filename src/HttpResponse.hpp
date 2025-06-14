#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include "HttpRequest.hpp"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void setStatusCode(int code);
    void setStatusMessage(const std::string &message);
    void addHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &_body);

    std::string buildResponse() const;

private:
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
};

HttpResponse handleHttpRequest(const std::string& request);


#endif
