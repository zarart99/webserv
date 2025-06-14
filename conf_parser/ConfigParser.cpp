#include "ConfigParser.hpp"
#include <sys/stat.h>
#include <unistd.h>
ConfigParser::ConfigParser(void) {}

ConfigParser::ConfigParser(ConfigParser const & src)
{
	*this = src;
}

ConfigParser & ConfigParser::operator=(ConfigParser const & src)
{
	if (this != &src)
	{
		this->_configServ = src._configServ;
	}
	return *this;
}

ConfigParser::~ConfigParser() {}

void ConfigParser::trimmer(std::string & str)
{
	std::string whitespace = " \t\n\r";
	size_t first = str.find_first_not_of(whitespace);
	if (first == std::string::npos)
	{
		str.clear();
		return;
	}
	size_t last = str.find_last_not_of(whitespace);
	str = str.substr(first, (last - first + 1));
}
std::vector<std::string> ConfigParser::split(std::string &str, char delimiter)
{
	std::vector<std::string> strs;
	std::string item;
	std::stringstream data(str);

	while (std::getline(data, item ,delimiter))
	{
		strs.push_back(item);
	}
	return strs;
}

std::vector<std::string> ConfigParser::splitWhitespace(std::string &str)
{
	std::vector<std::string> strs;
	std::string item;
	std::stringstream data(str);

	while (data >> item)//Точно?
	{
		strs.push_back(item);
	}
	return strs;
}

void ConfigParser::trimSemicolon(std::string& str)
{
	if (!str.empty() && str.back() == ';')
		str.erase(str.size() - 1);
}

void ConfigParser::parseConfigFile(std::string const & fileName )
{
	std::ifstream file(fileName.c_str());
	if (!file)
		throw std::runtime_error("Error: can not open config file !");
	else
	{
		std::string line;
		bool isServer = false;
		int braceNb = 0;
		std::vector<std::string> currentServer;

		while (std::getline(file, line))
		{
			trimmer(line);
			if (line.empty() || line[0] == '#')
				continue;
			if (line.find("server") == 0 && line.find('{') != std::string::npos)
			{
				isServer = true;
				braceNb++;
				continue;
			}
			if (isServer)
			{
				for (size_t i = 0; i < line.size(); i++)
				{
					if (line[i] == '{')
						braceNb++;
					if (line[i] == '}')
						braceNb--;
				}
				if (braceNb > 0)
					currentServer.push_back(line);
				else
				{
					ServerConfig dataServer = parseServer(currentServer);
					_configServ.push_back(dataServer);
					currentServer.clear();
					isServer = false;
				}
			}
		}
	if (braceNb != 0)
		throw std::runtime_error("Error: too mach brace on congig file!");
	}
	file.close();
}

ServerConfig ConfigParser::parseServer(std::vector<std::string>& strs)
{
	ServerConfig serverData;
	std::vector<std::string> currentLocationBlock;
	bool isLocation = false;
	int braceNb = 0;
	serverData.autoindexDef = false; //Дефолтная настройка
	serverData.client_max_body_sizeDef = 1024 * 1024; //Дефолтная настройка
	serverData.upload_dirDef = "/tmp/uplads"; //Дефолтная настройка, определиться потом с финальным расположением
	for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); it++)
	{
		std::string line = *it;
		if (line.find("location") == 0 && line.find('{') != std::string::npos)
		{
			currentLocationBlock.push_back(line);
			isLocation = true;
			braceNb++;
			continue;
		}
		if (isLocation)
		{
			for (size_t i = 0; i < line.size(); i++)
			{
				if (line[i] == '{')
					braceNb++;
				if (line[i] == '}')
					braceNb--;
			}
			if (braceNb > 0)
				currentLocationBlock.push_back(line);
			else
			{
				LocationStruct location = parseLocation(currentLocationBlock);
				serverData.location.push_back(location);
				currentLocationBlock.clear();
				isLocation = false;
			}
		}
		else
		{
			if (line.find("listen") == 0)
			{
				ListenStruct listen = parseListen(line);
				serverData.listen.push_back(listen);
			}
			else if (line.find("server_name") == 0)
				serverData.server_name = findServerName(line);
			else if (line.find("root") == 0)
				serverData.rootDef = findRoot(line);
			else if (line.find("index") == 0)
				serverData.indexDef = findIndex(line);
			else if (line.find("autoindex") == 0)
				serverData.autoindexDef = findAutoindex(line);
			else if (line.find("client_max_body_size") == 0)
				serverData.client_max_body_sizeDef = findMaxBody(line);
			else if (line.find("error_page") == 0)
				appendErrorPage(line, serverData.error_pageDef);
			else if (line.find("upload_dir") == 0)
				serverData.upload_dirDef = findUploadDir(line);
			else if (line.find("cgi") == 0)
				parseCgi(line, serverData.cgiDef);
			else
				throw std::runtime_error("Error: Unknown directive in server!");
		}
	}
	return serverData;
}

int ConfigParser::findPort(std::string& str)
{
	size_t pos = 0;
	int port = 0;
	try 
	{
		port = std::stoi(str, &pos);
		if (port < 1 || port > 65535)
			throw std::runtime_error("Error: Invalide port!");
		if (pos != str.size())
			throw std::runtime_error("Error: Invalide port!");
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error("Error: Invalide port!");
	}
	return port;
}

bool ConfigParser::checkValideIP(const std::string& str)
{
	std::vector<std::string> strs;
	size_t pos = 0;
	strs = split(str , '.');

	if (strs.size() != 4)
		return false;

	for (size_t i = 0; i < strs.size();  i++)
	{
		if (strs[i].empty() || strs[i].size() > 3)
			return false;

		if (strs[i].size() > 1 && strs[i][0] == '0') //Разве блоки не могут начинаться с 0?
			return false;

		try 
		{
			int value = std::stoi(strs[i], &pos);
			if (value < 0 || value > 255 || pos != strs[i].size())
				return false;
		}
		catch (const std::exception &e)
		{
			return false;
		}
	}
	return true;
}

ListenStruct ConfigParser::parseListen(std::string& str)
{
	ListenStruct listen;
	std::vector<std::string> firstSplit;
	std::vector<std::string> secondeSplit;

	firstSplit = split(str, ' ');//А если табуляция?
	if (firstSplit.size() != 2)
		throw std::runtime_error("Error: invalid listen directive format!");
	secondeSplit = split(firstSplit[1], ':');
	if (secondeSplit.size() == 2)
	{
		trimSemicolon(secondeSplit[1]);
		if (!checkValideIP(secondeSplit[0]))
			throw std::runtime_error("Error: invalid listen directive format!");
		listen.ip = secondeSplit[0];
		listen.port = findPort(secondeSplit[1]);
	}
	else if (secondeSplit.size() == 1 )
	{
		trimSemicolon(secondeSplit[0]);
		listen.ip = "0.0.0.0";
		listen.port = findPort(secondeSplit[0]);
	}
	else
		throw std::runtime_error("Error: Error: invalid listen directive format!");
	return listen;
}

std::vector<std::string> ConfigParser::findServerName(std::string& str)
{
	std::vector<std::string> names;
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() < 2)
		throw std::runtime_error("Invalid root directiv!");
	trimSemicolon(strs.back());
	if (strs.back().empty())
		throw std::runtime_error("Invalid root directiv!");
	for (size_t i = 1; i < strs.size(); i++)
	{
		names.push_back(strs[i]);
	}
	return names;
}

std::string ConfigParser::findRoot(std::string& str)
{
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() != 2)
		throw std::runtime_error("Invalid root directiv!");
	trimSemicolon(strs.back());
	if (strs.back().empty())
		throw std::runtime_error("Invalid root directiv!");
	return strs[1];
}

bool ConfigParser::findAutoindex(std::string& str)
{
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() != 2)
		throw std::runtime_error("Invalid Autoindex directiv!");
	trimSemicolon(strs.back());
	if (strs.back().empty())
		throw std::runtime_error("Invalid Autoindex directiv!");
	if (strs[1] == "off")
		return false;
	else if (strs[1] == "on")
		return true;
	return false;
}

size_t ConfigParser::findMaxBody(std::string &str)
{
	char suffix;
	size_t multiplier = 1;
	size_t value = 0;
	size_t pos = 0;
	size_t maxSize = 2147483648;
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() != 2)
		throw std::runtime_error("Invalid directiv client_max_body_size");
	trimSemicolon(strs[1]);
	if (strs[1].empty())
		throw std::runtime_error("Invalid directiv client_max_body_size");
	suffix = strs[1].back();
	if (!isdigit(suffix))
	{
		if (suffix == 'B' || suffix == 'b')//Мультипликатор для битов
			multiplier = 1;
		else if (suffix == 'K' || suffix == 'k')//Мультипликатор для килабайтов
			multiplier = 1024;
		else if (suffix == 'M' || suffix == 'm') //Мультипликатор для мегабайт
			multiplier = 1024 * 1024;
		else if (suffix == 'G' || suffix == 'g')
			multiplier = 1024 * 1024 * 1024;
		else
			throw std::runtime_error("Invalid client_max_body_size");
		strs[1].pop_back();
		if (strs[1].empty())
			throw std::runtime_error("Invalid directiv client_max_body_size");
	}
	try
	{
		value = std::stoul(strs[1], &pos);
		if (pos != strs[1].size())
			throw std::runtime_error("Invalid client_max_body_size");
	}
	catch(const std::exception& e)
	{
		throw std::runtime_error("Invalid client_max_body_size");
	}
	if (value > (maxSize / multiplier))
		throw std::runtime_error("Invalid client_max_body_size");
	return (value * multiplier);
}

std::vector<std::string> ConfigParser::findIndex(std::string& str)
{
	std::vector<std::string> index;
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() < 2)
		throw std::runtime_error("Invalid index directiv!");
	trimSemicolon(strs.back());
	if (strs.back().empty())
		throw std::runtime_error("Invalid index directiv!");
	for (size_t i = 1; i < strs.size(); i++)
	{
		index.push_back(strs[i]);
	}
	return index;
}

void ConfigParser::appendErrorPage(std::string& str, std::map<int, std::string> &errors)
{
	std::vector<std::string> strs;

	size_t pos = 0;
	strs = split(str, ' ');
	if (strs.size() < 3)
		throw std::runtime_error("Invalid error_page directiv!");
	trimSemicolon(strs.back());
	if (strs.back().empty())
		throw std::runtime_error("Invalid error_page directiv!");
	std::string page = strs.back();
	for (size_t i = 1; i < strs.size() - 1 ; i++)
	{
		try
		{
			int code = std::stoi(strs[i], &pos);
			if (code < 100 || code > 599 || pos != strs[i].size())
				throw std::runtime_error("Invalid error_page format!");
			errors[code] = page;
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Invalid error_page format!");
		}
	}
}

LocationStruct ConfigParser::parseLocation(std::vector<std::string>& strs)
{
	LocationStruct location;

	location.autondex = false; //Дефолтная настройка
	location.client_max_body_sizeDef = 1024 * 1024; //Дефолтная настройка

	if (!strs.empty())
		location.prefix = findPrefix(strs[0]);
	for (size_t i = 1; i < strs.size(); i++)
	{
		std::string line = strs[i];
		if (line.find("root") == 0)
			location.root = findRoot(line);
		else if (line.find("return") == 0)
			location.redir = findRedir(line);
		else if (line.find("index") == 0)
			location.index = findIndex(line);
		else if (line.find("autoindex") == 0)
			location.autoindex = findAutoindex(line);
		else if (line.find("allow_methods") == 0)
			location.allow_methods = findMethods(line);
		else if (line.find("client_max_body_size") == 0)
			location.client_max_body_size = findMaxBody(line);
		else if (line.find("error_page") == 0)
			appendErrorPage(line, location.error_page);
		else
			throw std::runtime_error("Error: Unknown directive in location!");
	}
	return location;
}

std::string ConfigParser::findPrefix(std::string& str)
{
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() != 3)
		throw std::runtime_error("Error: Invalid location format!");
	return strs[1];
}

std::vector<std::string> ConfigParser::findMethods(std::string& str)
{
	std::vector<std::string> strs;
	std::vector<std::string> methods;
	strs = split(str, ' ');
	if (strs.size() < 2)
		throw std::runtime_error("Error: Invalid allow_methods directiv!");
	trimSemicolon(strs.back());
	if (strs.back().empty())
		throw std::runtime_error("Invalid allow_methods directiv!");
	for (size_t i = 1; i < strs.size(); i++)
	{
		for (size_t i_2 = 0; i_2 < strs[i].size(); i_2++)
		{
			strs[i][i_2] = std::toupper(strs[i][i_2]);
		}
		if (strs[i] == "GET" || strs[i] == "POST" || strs[i] == "DELETE")
			methods.push_back(strs[i]);
		else
			throw std::runtime_error("Error: Invalid method in directiv!");
	}
	return methods;
}
std::map<int, std::string> ConfigParser::findRedir(std::string& str)
{
	std::vector<std::string> strs;
	std::map<int, std::string> redirect;
	size_t pos = 0;
	strs = split(str, ' ');
	if (strs.size() != 3)
		throw std::runtime_error("Error: Invalid redirect directiv!");
	trimSemicolon(strs.back());
	if (strs.back().empty())
		throw std::runtime_error("Invalid redirect format!");
	{
		try
		{
			int code = std::stoi(strs[1], &pos);
			if (code < 300 || code > 399 || pos != strs[1].size())
				throw std::runtime_error("Invalid redirect code!");
			redirect[code] = strs[2];
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Invalid redirect format!");
		}
	}
	return redirect;
}

const std::vector<ServerConfig>& ConfigParser::getServers(void) const
{
	return _configServ;
}

size_t ConfigParser::getServerCount(void)
{
	return _configServ.size();
}

std::string ConfigParser::findUploadDir(std::string& str)
{

}

void ConfigParser::parseCgi(std::string &str, std::map<std::string, CgiStruct> &cgi)
{

}

//		void validateConfig(void);
//		bool checkPath(const std::string& path);
//		void validateServer(const ServerConfig& server);
//		void validateLocation(const LocationStruct& location);
//		void check port(void);
//		void printCongig(void);