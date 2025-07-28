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
#include <poll.h>
#include <signal.h>
#include <cstdio>
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"
#include "HttpRequest.hpp"

struct ExecveEnvp
{
	std::string query;
	std::string path_info;

};

struct ExecveArgs
{

	std::string path_relative;
	std::string path_absolut;
	CgiStruct data_cgi;
	ExecveEnvp data_envp;
	std::vector<std::string> envs_strings;
	std::vector<char*> envs_ptrs;
	std::vector<std::string> argv_strings;
	std::vector<char*> argv_ptrs;
	
};

struct RequestServer
{
	std::string ip;
	std::string url;
	std::string ext;//Расширение скрипта если он есть 
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

	
	bool isCgi(void);

	//Создаем переменные окружения CGI
	void createCgiEnvp(void);
	std::string findScriptFilename(void);
	void findQuery(void);
	void findPathInfo(void);
	bool checkMethod(void);
	std::string checkPath(std::string& path);

	std::string cgiHandler(void);
	std::string composeErrorResponse(const std::string& error);
	std::string executeScript(void);
	void findArgsExecve(void);
	void printEnvpCgi(void);

	private:
	ConfigParser _config;
	RequestServer _data_rec;
	const ServerConfig* _server;
	const LocationStruct* _location;
	struct ExecveArgs _exec_args;
};

#endif