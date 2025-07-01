#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"
#include "HttpRequest.hpp"

struct RequestServer
{
	int ip;
	int port;
	std::string host;
};

class Cgi : public RequestHandler
{
	public:

	Cgi(void);
	Cgi(const ConfigParser& config, int port, int ip, const std::string& host);
	Cgi(const Cgi& src);
	Cgi& operator=(const Cgi& src);
	~Cgi(void);

	bool isCgi(std::string url);
//	std::string getScript();
	void createCgiEnvp(HttpRequest rec);

	private:
	std::string _cgi_ext;
	RequestServer _data_rec;
	const ServerConfig *_server;
	char** _envs;

//	void cgiHandler(const HttpRequest& req, int port, std::string host);
};

#endif