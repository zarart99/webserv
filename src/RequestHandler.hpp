#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ConfigParser.hpp"
#include "utils.hpp"

struct MultipartPart
{
    std::string name;
    std::string filename;
    std::string body;
};

class RequestHandler
{
public:
    RequestHandler();
    RequestHandler(const ConfigParser& config);
    RequestHandler(const RequestHandler& src);
    RequestHandler& operator=(const RequestHandler& src);
    ~RequestHandler();

    HttpResponse handleRequest(const HttpRequest &request, int serverPort, const std::string &serverIp, const std::string &serverHost);

protected:
    const ServerConfig* _findServerConfig(int port, const std::string& ip, const std::string& host) const;
    const LocationStruct* _findLocationFor(const ServerConfig& server, const std::string& uri) const;

private:
    const ConfigParser* _config;

    HttpResponse _handleGet(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server);
    HttpResponse _handlePost(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server);
    HttpResponse _handleSimplePost(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server);
    HttpResponse _handleMultipart(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server);
    std::string saveBodyToFile(const std::string &body, const std::string &suggestedName,
                               const std::string &storageDir);
    HttpResponse _handleDelete(const HttpRequest &request, const LocationStruct &location, const ServerConfig &server);

    // Returns the absolute directory to use for file uploads or an empty string on error
    std::string _getStorageDir(const LocationStruct &location, const ServerConfig &server) const;

    HttpResponse _createErrorResponse(int statusCode, const ServerConfig *server, const std::vector<std::string> *allowed_methods, const LocationStruct *location = NULL);
};

#endif
