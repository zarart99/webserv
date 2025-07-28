#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include "HttpRequest.hpp"

class HttpResponse
{
public:
    HttpResponse();
    HttpResponse(const HttpResponse &src);
    HttpResponse &operator=(const HttpResponse &src);
    ~HttpResponse();

    void setStatusCode(int code);
    void setStatusMessage(const std::string &message);
    void addHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &_body);
    static const std::map<int,std::string>& getStatusMessages();


    std::string buildResponse() const;

private:
    int _statusCode;
    std::string _statusMessage;
    static std::map<int, std::string> _statusMessages;
    std::map<std::string, std::string> _headers;
    std::string _body;
    static std::map<int,std::string> initStatusMessages();

};

HttpResponse handleHttpRequest(const std::string& request);


#endif
