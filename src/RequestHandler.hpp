#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ConfigParser.hpp"

class RequestHandler {
public:
    RequestHandler();
    RequestHandler(ConfigParser& config);
    RequestHandler(const RequestHandler& src);
    RequestHandler& operator=(const RequestHandler& src);
    ~RequestHandler();

    HttpResponse handleRequest(const HttpRequest& request, int serverPort, const std::string& serverHost);

private:
    ConfigParser* _config;

    HttpResponse _handleGet(const HttpRequest& request, const LocationStruct& location, const ServerConfig& server);
    HttpResponse _handlePost(const HttpRequest& request, const LocationStruct& location, const ServerConfig& server);
    HttpResponse _handleDelete(const HttpRequest& request, const LocationStruct& location, const ServerConfig& server);

    HttpResponse _createErrorResponse(int statusCode, const ServerConfig* server, const std::vector<std::string>* allowed_methods = NULL);
    const ServerConfig* _findServerConfig(int port, const std::string& host) const;
    const LocationStruct* _findLocationFor(const ServerConfig& server, const std::string& uri) const;
};

#endif
