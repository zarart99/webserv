#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <stdexcept>


struct ListenStruct
{
	std::string ip;
	int port;
};

struct CgiStruct
{
	std::string extension;
	std::string pathInterpreter;
	size_t timeout;
};

struct LocationStruct
{
	std::string prefix;
	std::string root;
	std::vector<std::string> index;
	bool autoindex;
	size_t client_max_body_size;
	std::map<int, std::string> error_page;
	std::vector<std::string> allow_methods;
	std::string upload_dir;
	std::map<int, std::string> redir;
	std::map<std::string, CgiStruct> cgi;
};

struct ServerConfig
{
	std::vector<ListenStruct> listen;
	std::vector<std::string> server_name;
	std::string rootDef;
	std::vector<std::string> indexDef;
	bool autoindexDef;
	size_t client_max_body_sizeDef;
	std::map<int, std::string> error_pageDef;
	std::vector<LocationStruct> location;
	std::string upload_dirDef;
	std::map<std::string, CgiStruct> cgiDef; //Он может быть дефолтным?
};

class ConfigParser
{
	public:
		ConfigParser(void);
		ConfigParser(ConfigParser const &src);
		ConfigParser & operator=(ConfigParser const & src);
		~ConfigParser(void);

		void trimmer(std::string & str);
		std::vector<std::string> split(std::string &str, char delimiter);
		std::vector<std::string> splitWhitespace(std::string &str);
		void trimSemicolon(std::string& str);


		void parseConfigFile(std::string const & fileName );
		ServerConfig parseServer(std::vector<std::string>& strs);

		ListenStruct parseListen(std::string& str);
		int findPort(std::string& str);
		bool checkValideIP(std::string& str);

		std::vector<std::string> findServerName(std::string& str);
		std::string findRoot(std::string& str);
		bool findAutoindex(std::string& str);
		size_t findMaxBody(std::string &str);
		void appendErrorPage(std::string& str, std::map<int, std::string> &errors);
		std::vector<std::string> findIndex(std::string& str);
		std::string findUploadDir(std::string& str);
		void parseCgi(std::string &str, std::map<std::string, CgiStruct> &cgi);

		LocationStruct parseLocation(std::vector<std::string>& strs);
		std::string findPrefix(std::string& str);
		std::vector<std::string> findMethods(std::string& str);
		std::map<int, std::string> findRedir(std::string& str);		

//		void validateConfig(void);
//		bool checkPath(const std::string& path);
//		void validateServer(const ServerConfig& server);
//		void validateLocation(const LocationStruct& location);
//		void check port(void);
//		void printConfig(void);
		const std::vector<ServerConfig>& getServers(void) const;
		size_t getServerCount(void);
	private:
		std::vector<ServerConfig> _configServ;
};

#endif