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

struct LocationStruct
{
	std::string prefix;
	std::string root;
	std::vector<std::string> index;
	bool autoindex;
	size_t client_max_body_sizeDef;
	std::map<int, std::string> error_page;
	std::vector<std::string> allow_methods;
	std::map<int, std::string> redir;
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
		void trimSemicolon(std::string& str);


		void parseConfigFile(std::string const & fileName );
		ServerConfig parseServer(std::vector<std::string>& strs);

		ListenStruct parseListen(std::string& str);
		int findPort(std::string& str);
		bool checkValideIP(const std::string& str);
		
		std::vector<std::string> findServerName(std::string& str);
		std::string findRoot(std::string& str);
		bool findAutoindex(std::string& str);

		size_t findMaxBody(std::string &str);
		std::map<int, std::string> findErrorPage(std::string& str);
		std::vector<std::string> findIndex(std::string& str);
		LocationStruct parseLocation(std::vector<std::string>& strs);
		std::string findPrefix(std::string& str);
		std::vector<std::string> findMethods(std::string& str);
		std::map<int, std::string> findRedir(std::string& str);		

	private:
		std::vector<ServerConfig> _configServ;
};

#endif