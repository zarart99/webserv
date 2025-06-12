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

HttpResponse handle_http_request(const std::string& raw_request) {
    HttpRequest req(raw_request);
    HttpResponse res;

    if (!req.isValid()) {
        res.setStatusCode(400);
        res.setStatusMessage("Bad Request");
        res.setBody("Cannot parse request");
        return res;
    }

    if (req.getMethod() == "GET") {
        if (req.getUri() == "/") {
            res.setStatusCode(200);
            res.setStatusMessage("OK");
            res.setBody("Welcome to webserv!\n");
        } else {
            res.setStatusCode(404);
            res.setStatusMessage("Not Found");
            res.setBody("The requested resource was not found.");
        }
    } else if (req.getMethod() == "POST") {
        res.setStatusCode(200);
        res.setStatusMessage("OK");
        res.setBody("POST request received.");
    } else if (req.getMethod() == "DELETE") {
        res.setStatusCode(200);
        res.setStatusMessage("OK");
        res.setBody("DELETE acknowledged.");
    } else {
        res.setStatusCode(405);
        res.setStatusMessage("Method Not Allowed");
        res.setBody("Unsupported method.");
    }

    return res;
}


#endif
