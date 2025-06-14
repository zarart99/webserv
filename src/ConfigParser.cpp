#include "ConfigParser.hpp"
		
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

void ConfigParser::trimSemicolon(std::string& str)
{
	if (!str.empty() && str.back())
		str.erase(str.size() - 1);
}

void ConfigParser::parseConfigFile(std::string const & fileName )
{
	std::ifstream file(fileName.c_str());
	if (!file)
	{
		std::cout << "Error: can not open config file !" << std::endl;
		return ; //Заменить на catch
	}
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
		file.close();
	}
}
	ServerConfig ConfigParser::parseServer(std::vector<std::string>& strs)
	{
		ServerConfig serverData;
		std::vector<std::string> currentLocationBlock;
		bool isLocation = false;
		int braceNb = 0;

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
					serverData.error_pageDef = findErrorPage(line);
				else //Проверить оставить или нет
				{
					std::cout << "Error: Uknow directive!" << std::endl;
				}
			}
		}
		return serverData;
	}

ListenStruct ConfigParser::parseListen(std::string& str)
{
	ListenStruct listen;
	std::vector<std::string> firstSplit;
	std::vector<std::string> secondeSplit;
	std::string ip; //bonne format verif
	int port; //max_int // min_int verif
	firstSplit = split(str, ' ');//А если табуляция?
	secondeSplit = split(firstSplit[1], ':');
	if (secondeSplit.size() == 2)
	{
		trimSemicolon(secondeSplit[1]);
		listen.ip = secondeSplit[0];
		listen.port = std::atoi(secondeSplit[1].c_str());//Проверку Int_max/Int_min. c catch
	}
	else if (secondeSplit.size() == 1 )
	{
		trimSemicolon(secondeSplit[0]);
		listen.ip = "0.0.0.0";
		listen.port = std::atoi(secondeSplit[0].c_str());//Проверку Int_max/Int_min. c catch/а так же что бы не выходил за выдельныее лимиты для ports
	}//Сделать проверку что на синтакс в этом месте.
	return listen;
}

std::vector<std::string> ConfigParser::findServerName(std::string& str)
{
	std::vector<std::string> names;
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() < 2)
	{
		std::cout << "Error: uknowe directive!";
		return names;//Отправиться пустой контейнер? catch?
	}
	for (size_t i = 1; i < strs.size(); i++)
	{
		names.push_back(strs[i]);
	}
	trimSemicolon(names.back());
	return names;
}

std::string ConfigParser::findRoot(std::string& str)
{
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() != 2)
	{
		std::cout << "Error: uknowe directive!";
		return "";//Можно таким образом вернуть пустую строку?
	}
	trimSemicolon(strs[1]);
	return strs[1];
}

bool ConfigParser::findAutoindex(std::string& str)
{
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() != 2)
	{
		std::cout << "Error: uknowe directive!";
		return false;//catch?
	}
	trimSemicolon(strs[1]);
	if (strs[1] == "off")
		return false;
	else if (strs[1] == "on")
		return true;
	return false;
}

t_size ConfigParser::findMaxBody(std::string &str)
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
		if (suffix == 'B')//Мультипликатор для битов
			multiplier = 1;
		else if (suffix == 'K')//Мультипликатор для килабайтов
			multiplier = 1024;
		else if (suffix == 'M') //Мультипликатор для мегабайт
			multiplier = 1024 * 1024;
		else if (suffix == 'G')
			multiplier = 1024 * 1024 * 1024;
		else
			throw std::runtime_error("Invalid client_max_body_size");
	}
	strs[1].pop_back();
	try
	{
		value = std::stoul(str);
		if (pos != strs[1].size())
			throw std::runtime_error("Invalid client_max_body_size");
	}
	catch(const std::exception& e)
	{
		throw std::runtime_error("Invalid client_max_body_size");
	}
	if ((value * multiplier) > maxSize )
		throw std::runtime_error("Invalid client_max_body_size");
	return (value * multiplier);
}

std::vector<std::string> ConfigParser::findIndex(std::string& str)
{
	std::vector<std::string> index;
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() < 2)
	{
		std::cout << "Error: uknowe directive!";
		return index;//Отправиться пустой контейнер? catch?
	}
	for (size_t i = 1; i < strs.size(); i++)
	{
		index.push_back(strs[i]);
	}
	trimSemicolon(names.back());
	return index;
}

std::map<int, std::string> ConfigParser::findErrorPage(std::string& str);
LocationStruct ConfigParser::parseLocation(std::vector<std::string>& strs);

std::string ConfigParser::findPrefix(std::string& str)
{
	std::vector<std::string> strs;
	strs = split(str, ' ');
	if (strs.size() != 3)
	{
		std::cout << "Error: uknowe directive!";
		return "";//Можно таким образом вернуть пустую строку?
	}
	return strs[1];
}

std::vector<std::string> ConfigParser::findMethods(std::string& str);
std::map<int, std::string> ConfigParser::findRedir(std::string& str);

