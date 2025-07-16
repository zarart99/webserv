#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"
#include "HttpRequest.hpp"

struct ExecveArgs
{
	std::string cgi_ext;
	std::string path_script;
	std::vector<std::string> envs_strings;
	std::vector<std::string> argv_strings;

	std::vector<char*> envs_ptrs;
	std::vector<char*> argv_ptrs;
	std::string interpreter;
};

struct RequestServer
{
	std::string ip;
	std::string url;
	int port;
	std::string host;
	HttpRequest req;
};

class Cgi : public RequestHandler
{
	public:

	Cgi(void);
	Cgi(ConfigParser& config, HttpRequest request, int port, std::string ip, const std::string& host);
	Cgi(const Cgi& src);
	Cgi& operator=(const Cgi& src);
	~Cgi(void);

	
	bool isCgi(std::string url);

	//Создаем переменные окружения CGI
	void createCgiEnvp(void);
	std::string findScriptFilename(void);
	std::string findQuery(void);
	std::string findPathInfo(void);


	std::string cgiHandler(void);
	std::string composeErrorResponse(const std::string& error);
	std::string executeScript(void);
	void findArgsExecve(void);
	void printEnvpCgi(void);
	char** getEnvp(void);

	private:
	ConfigParser _config;
	RequestServer _data_rec;
	const ServerConfig* _server;
	const LocationStruct* _location;
	struct ExecveArgs _exec_args;
};

#endif