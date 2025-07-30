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
#include <sys/stat.h>

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
    std::string upload_path;
    std::vector<std::string> index;
    bool autoindex;
    size_t client_max_body_size;
	std::map<int, std::string> error_page;
	std::vector<std::string> allow_methods;
	std::map<int, std::string> redir;
	std::vector<CgiStruct> cgi;
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
		ConfigParser(std::string &str);
		ConfigParser & operator=(ConfigParser const & src);
		~ConfigParser(void);

		void printConfig(void);
		const std::vector<ServerConfig>& getServers(void) const;
		size_t getServerCount(void);
		std::vector<ListenStruct> getUniqueListen(void) const;

	private:
		std::vector<ServerConfig> _configServ;
		std::vector<ListenStruct> _uniqueListen;

		void parseConfigFile(std::string const & fileName );
		ServerConfig parseServer(std::vector<std::string>& strs);
		LocationStruct parseLocation(std::vector<std::string>& strs);
		ListenStruct parseListen(std::string& str);
		int findPort(std::string& str);
		bool checkValideIP(std::string& str);
		std::vector<std::string> findServerName(std::string& str);
        std::string findRoot(std::string& str);
        std::string findUploadPath(std::string& str);
        std::vector<std::string> findIndex(std::string& str);
		bool findAutoindex(std::string& str);
		size_t findMaxBody(std::string &str);
		void appendErrorPage(std::string& str, std::map<int, std::string> &errors);
		std::string findPrefix(std::string& str);
		std::vector<std::string> findMethods(std::string& str);
		std::map<int, std::string> findRedir(std::string& str);		
		void parseCgi(std::string &str, std::vector<CgiStruct> &cgi);

		bool checkDoubleListen(std::vector<ListenStruct>& Servlisten, ListenStruct& listen);
		bool validateGlobalUniqueListen(void);
		bool validateServerName(std::string& name);
		void checkPort(ServerConfig& serverData);
		void defineDefaultListen(ServerConfig& serverData);
		void defineDefaultMethods(LocationStruct& location);
		void extractUniqueListen(void);

		void validateGlobalConfig(void);
		void validateServer(const ServerConfig& server, size_t serverIndex);
		void validateLocation(const LocationStruct& location, const ServerConfig& server, size_t serverIndex, size_t locationIndex);
};

#endif
