#include "ConfigParser.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <climits>

ConfigParser::ConfigParser(void) {}

ConfigParser::ConfigParser(ConfigParser const & src)
{
	*this = src;
}

ConfigParser::ConfigParser(std::string &str)
{
	parseConfigFile(str);
}

ConfigParser & ConfigParser::operator=(ConfigParser const & src)
{
	if (this != &src)
	{
		this->_configServ = src._configServ;
		this->_uniqueListen = src._uniqueListen;
	}
	return *this;
}

ConfigParser::~ConfigParser() {}

void trimmer(std::string & str)
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
std::vector<std::string> split(std::string &str, char delimiter)
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

std::vector<std::string> splitWhitespace(std::string &str)
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

void trimSemicolon(std::string& str)
{
	if (!str.empty() && str[str.size() - 1] == ';')
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
	if (!validateGlobalUniqueListen())
		throw std::runtime_error("Error: Conflicting listen between servers!");
	extractUniqueListen();
}

ServerConfig ConfigParser::parseServer(std::vector<std::string>& strs)
{
	ServerConfig serverData;
	std::vector<std::string> currentLocationBlock;
	bool isLocation = false;
	int braceNb = 0;
	serverData.autoindexDef = false; //Дефолтная настройка
	serverData.client_max_body_sizeDef = 1024 * 1024; //Дефолтная настройка
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
				if (checkDoubleListen(serverData.listen, listen))
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
			else
				throw std::runtime_error("Error: Unknown directive in server!");
		}
	}
	checkPort(serverData);
	return serverData;
}

int ConfigParser::findPort(std::string& str)
{
	char* endptr;
	errno = 0;
	long port = 0;
	port = std::strtol(str.c_str(), &endptr, 10);
	if (errno == ERANGE || *endptr != '\0' || port < 1 || port > 65535)
		throw std::runtime_error("Error: Invalide port!");
	return static_cast<int>(port);
}

bool ConfigParser::checkValideIP(std::string& str)
{
	std::vector<std::string> strs;
	strs = split(str , '.');

	if (strs.size() != 4)
		return false;

	for (size_t i = 0; i < strs.size();  i++)
	{
		if (strs[i].empty() || strs[i].size() > 3)
			return false;

		char* endptr;
		errno = 0;
		long value = std::strtol(strs[i].c_str(), &endptr, 10);
		if (value < 0 || value > 255 || errno == ERANGE || *endptr != '\0')
			return false;
	}
	return true;
}

ListenStruct ConfigParser::parseListen(std::string& str)
{
	ListenStruct listen;
	std::vector<std::string> firstSplit;
	std::vector<std::string> secondeSplit;

	firstSplit = splitWhitespace(str);
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
	strs = splitWhitespace(str);
	if (strs.size() < 2)
		throw std::runtime_error("Invalid server_name directiv!");
	trimSemicolon(strs[strs.size() - 1]);
	if (strs[strs.size() - 1].empty())
		throw std::runtime_error("Invalid server_name directiv!");
	for (size_t i = 1; i < strs.size(); i++)
	{
		if (validateServerName(strs[i]))
			names.push_back(strs[i]);
		else
			throw std::runtime_error("Invalid server_name directiv!");
	}
	return names;
}

std::string ConfigParser::findRoot(std::string& str)
{
	std::vector<std::string> strs;
	strs = splitWhitespace(str);
	if (strs.size() != 2)
		throw std::runtime_error("Invalid root directiv!");
	trimSemicolon(strs[1]);
	if (strs[1].empty())
		throw std::runtime_error("Invalid root directiv!");
	return strs[1];
}

bool ConfigParser::findAutoindex(std::string& str)
{
	std::vector<std::string> strs;
	strs = splitWhitespace(str);
	if (strs.size() != 2)
		throw std::runtime_error("Invalid Autoindex directiv!");
	trimSemicolon(strs[1]);
	if (strs[1].empty())
		throw std::runtime_error("Invalid Autoindex directiv!");
	if (strs[1] == "off")
		return false;
	else if (strs[1] == "on")
		return true;
	else
		throw std::runtime_error("Invalid Autoindex value!");
	return false;
}

size_t ConfigParser::findMaxBody(std::string &str)
{
	char suffix;
	size_t multiplier = 1;
	size_t value = 0;
	size_t maxSize = 2147483648;
	std::vector<std::string> strs;
	strs = splitWhitespace(str);
	if (strs.size() != 2)
		throw std::runtime_error("Invalid directiv client_max_body_size");
	trimSemicolon(strs[1]);
	if (strs[1].empty())
		throw std::runtime_error("Invalid directiv client_max_body_size");
	suffix = strs[1][strs[1].size() - 1];
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
		strs[1].erase(strs[1].size() - 1);
		if (strs[1].empty())
			throw std::runtime_error("Invalid directiv client_max_body_size");
	}
	char* endptr;
	errno = 0;
	unsigned long temp = std::strtoul(strs[1].c_str(), &endptr, 10);
	if (errno == ERANGE || *endptr != '\0')
		throw std::runtime_error("Invalid client_max_body_size");
	value = static_cast<size_t>(temp);
	if (value > (maxSize / multiplier))
		throw std::runtime_error("Invalid client_max_body_size");
	return (value * multiplier);
}

std::vector<std::string> ConfigParser::findIndex(std::string& str)
{
	std::vector<std::string> index;
	std::vector<std::string> strs;
	strs = splitWhitespace(str);
	if (strs.size() < 2)
		throw std::runtime_error("Invalid index directiv!");
	trimSemicolon(strs[strs.size() - 1]);
	if (strs[strs.size() - 1].empty())
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
	strs = splitWhitespace(str);
	size_t lastItem = strs.size() - 1;
	if (strs.size() < 3)
		throw std::runtime_error("Invalid error_page directiv!");
	trimSemicolon(strs[lastItem]);
	if (strs[lastItem].empty())
		throw std::runtime_error("Invalid error_page directiv!");
	std::string page = strs[lastItem];
	for (size_t i = 1; i < lastItem ; i++)
	{
		char* endptr;
		errno = 0;
		long code = std::strtol(strs[i].c_str(), &endptr, 10);
		if (code < 100 || code > 599 || *endptr != '\0' || errno == ERANGE)
			throw std::runtime_error("Invalid error code format!");
		errors[static_cast<int>(code)] = page;
	}
}

LocationStruct ConfigParser::parseLocation(std::vector<std::string>& strs)
{
	LocationStruct location;

	location.autoindex = false; //Дефолтная настройка
	location.client_max_body_size = 1024 * 1024; //Дефолтная настройка
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
		else if (line.find("cgi") == 0)
			parseCgi(line, location.cgi);
		else
			throw std::runtime_error("Error: Unknown directive in location!");
	}
	if (location.allow_methods.empty())
		defineDefaultMethods(location);
	return location;
}

std::string ConfigParser::findPrefix(std::string& str)
{
	std::vector<std::string> strs;
	strs = splitWhitespace(str);
	if (strs.size() != 3)
		throw std::runtime_error("Error: Invalid location format!");
	return strs[1];
}

std::vector<std::string> ConfigParser::findMethods(std::string& str)
{
	std::vector<std::string> strs;
	std::vector<std::string> methods;
	strs = splitWhitespace(str);
	if (strs.size() < 2)
		throw std::runtime_error("Error: Invalid allow_methods directiv!");
	trimSemicolon(strs[strs.size() - 1]);
	if (strs[strs.size() - 1].empty())
		throw std::runtime_error("Invalid allow_methods directiv!");
	for (size_t i = 1; i < strs.size(); i++)
	{
		for (size_t i_2 = 0; i_2 < strs[i].size(); i_2++)
		{
			strs[i][i_2] = std::toupper(strs[i][i_2]);
		}
		if (strs[i] == "GET" || strs[i] == "POST" || strs[i] == "DELETE")//Если будет прописанн другой метод то выйдем с ошибкой
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
	strs = splitWhitespace(str);
	if (strs.size() < 2 || strs.size() > 3)
		throw std::runtime_error("Error: Invalid redirect directiv!");
	trimSemicolon(strs[strs.size() - 1]);
	if (strs[strs.size() - 1].empty())
		throw std::runtime_error("Invalid redirect format!");
	{
		char* endptr;
		errno = 0;
		long code = std::strtol(strs[1].c_str(), &endptr, 10);
		if ( *endptr != '\0' || errno == ERANGE || code < 100 || code > 500)
			throw std::runtime_error("Invalid redirect code!");
		if (strs.size() == 3)
			redirect[static_cast<int>(code)] = strs[2];
		else
			redirect[static_cast<int>(code)] = "";//Если у нас только код то строка будет пустой
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

void ConfigParser::parseCgi(std::string &str, std::vector<CgiStruct> &cgi)//Парсер для формата cgi .php /usr/bin/php-cgi 50
{
	std::vector<std::string> strs;
	strs = splitWhitespace(str);

	if (strs.size() < 3 || strs.size() > 4)
		throw std::runtime_error("Invalid CGI directiv format!");
	trimSemicolon(strs[strs.size() - 1]);
	if (strs[strs.size() - 1].empty())
		throw std::runtime_error("Invalid CGI directiv !");
	CgiStruct cgiTemp;
	cgiTemp.extension = strs[1];
	cgiTemp.pathInterpreter = strs[2];
	if (cgiTemp.extension[0] != '.')//Проверка что расщирение указанно корректно
		throw std::runtime_error("Error: Invalid CGI extension!");
	if (access(cgiTemp.pathInterpreter.c_str(), X_OK) != 0)//Проверка на то что путь к интерпритатору существует
		throw std::runtime_error("Error: CGI interpreter not executable!");
	if (strs.size() == 4)
	{
		char* endptr;
		errno = 0;
		unsigned long temp = std::strtoul(strs[3].c_str(), &endptr, 10);
		if (*endptr != '\0' || errno == ERANGE)
			throw std::runtime_error("Invalide CGI timeout!");
		cgiTemp.timeout = static_cast<size_t>(temp);
	}
	else
		cgiTemp.timeout = 30;//Если скрипт незапустился то закончим процесс через указанное здесь время
	cgi.push_back(cgiTemp);
}

void ConfigParser::printConfig(void) //Небольшой дебаггер , вывод данных из атрибута конфигурационного файла 
{
	int i = 0;
	std::cout << "Config file" << std::endl;
	std::cout << "Number of server:" << getServerCount() << std::endl;

	if (_configServ.empty())
		throw std::runtime_error("Error: Empty config file!");
	for (std::vector<ServerConfig>::iterator it = _configServ.begin(); it != _configServ.end(); it++)
	{
		std::cout << "\nServer[" << i << "]" << std::endl;

		for (std::vector<ListenStruct>::iterator il = it->listen.begin() ; il != it->listen.end(); il++)
		{
			std::cout << "Listen: " << std::endl;
			std::cout << "-IP: " << il->ip << std::endl;
			std::cout << "-Port: " << il->port << std::endl;
		}
		if (!it->server_name.empty())
		{
			std::cout << "Server name: " << std::endl;
			for (std::vector<std::string>::iterator in = it->server_name.begin() ; in != it->server_name.end(); in++)
			{
				std::cout << "-" << *in << std::endl;
			}
		}
		if (!it->rootDef.empty())
			std::cout << "Default root: " << it->rootDef << std::endl;
		if (!it->indexDef.empty())
		{
			std::cout << "Default index: " << std::endl;
			for (std::vector<std::string>::iterator ii = it->indexDef.begin() ; ii != it->indexDef.end(); ii++)
			{
				std::cout << "-" << *ii << std::endl;
			}
		}
		if (it->autoindexDef)
			std::cout << "Default autoindex: " << "true" << std::endl;
		else
			std::cout << "Default autoindex: " << "false" << std::endl;
		std::cout << "Default client_max_body_size: " << it->client_max_body_sizeDef << std::endl;
		if (!it->error_pageDef.empty())
		{
			std::cout << "Default error page: " << std::endl;
			for (std::map<int, std::string>::iterator iep = it->error_pageDef.begin() ; iep != it->error_pageDef.end(); iep++)
			{
				std::cout << "-" << iep->first << " " << iep->second << std::endl;
			}
		}
		for (std::vector<LocationStruct>::iterator iloc = it->location.begin() ; iloc != it->location.end(); iloc++)
		{
			std::cout << "\nLocation: "<< std::endl;
			if (!iloc->prefix.empty())
				std::cout << "Prefix: " << iloc->prefix << std::endl;
			if (!iloc->root.empty())
				std::cout << "Root: " << iloc->root << std::endl;
			if (!iloc->index.empty())
			{
				std::cout << "Index page: " << std::endl;
				for (std::vector<std::string>::iterator ii = iloc->index.begin() ; ii != iloc->index.end(); ii++)
				{
					std::cout << "-" << *ii << std::endl;
				}
			}
			if (iloc->autoindex)
				std::cout << "Autoindex: " << "true" << std::endl;
			else
				std::cout << "Autoindex: " << "false" << std::endl;
			std::cout << "Client_max_body_size: " << iloc->client_max_body_size << std::endl;
			if (!iloc->error_page.empty())
			{
				std::cout << "Error page: " << std::endl;
				for (std::map<int, std::string>::iterator iep = iloc->error_page.begin() ; iep != iloc->error_page.end(); iep++)
				{
					std::cout << "-" << iep->first << " " << iep->second << std::endl;
				}
			}
			if (!iloc->allow_methods.empty())
			{
				std::cout << "Allow methods: " << std::endl;
				for (std::vector<std::string>::iterator iam = iloc->allow_methods.begin() ; iam != iloc->allow_methods.end(); iam++)
				{
					std::cout << "-" << *iam << std::endl;
				}
			}
			if (!iloc->redir.empty())
			{
				std::cout << "Redir: " << std::endl;
				for (std::map<int, std::string>::iterator ir = iloc->redir.begin() ; ir != iloc->redir.end(); ir++)
				{
					std::cout << "-" << ir->first << " " << ir->second << std::endl;
				}
			}
			if (!iloc->cgi.empty())
			{
				std::cout << "CGI: " << std::endl;
				for (std::vector<CgiStruct>::iterator ic = iloc->cgi.begin() ; ic != iloc->cgi.end(); ic++)
				{
					std::cout << "Extension: " << ic->extension << std::endl;//Расширение для запуска скрипта
					std::cout << "PathInterpreter: " << ic->pathInterpreter << std::endl;//Путь к интерпритатору
					std::cout << "Timeout: " << ic->timeout << std::endl; 
				}
			}
		}
		i++;
	}
	std::cout << "IP/PORT" << std::endl;
	for(size_t i = 0; i < _uniqueListen.size(); i++)
	{
		std::cout << "IP == " <<  _uniqueListen[i].ip << " PORT == " << _uniqueListen[i].port << std::endl;
	}
}


bool ConfigParser::checkDoubleListen(std::vector<ListenStruct>& Servlisten, ListenStruct& listen)//Функция проверяет директиву listen в сервере. Не должно быть одинаковых IP/port
{
	if (!Servlisten.empty())
	{
		for (std::vector<ListenStruct>::iterator it = Servlisten.begin(); it != Servlisten.end(); it++)
		{
			if (it->port == listen.port)
			{
				if (it->ip == listen.ip || it->ip == "0.0.0.0" || listen.ip == "0.0.0.0")
					return false;
			}
		}
	}
	return true;
}

bool ConfigParser::validateGlobalUniqueListen(void)//Функция сравнивает директивы listen между серверами. Не должно быть одинаковых IP/port
{
	for (std::vector<ServerConfig>::iterator it = _configServ.begin(); it != _configServ.end(); it++)
	{
		for (std::vector<ServerConfig>::iterator it_2 = it + 1; it_2 != _configServ.end(); it_2++)
		{
			for (std::vector<ListenStruct>::iterator it_l = it->listen.begin(); it_l != it->listen.end(); it_l++)
			{
				for (std::vector<ListenStruct>::iterator it_l2 = it_2->listen.begin(); it_l2 != it_2->listen.end(); it_l2++)
				{
					if (it_l->port == it_l2->port)
					{
						if (it_l->ip == it_l2->ip || it_l->ip == "0.0.0.0" || it_l2->ip == "0.0.0.0")
						{
					
							if (it->server_name.empty() && it_2->server_name.empty())
								return false;
							for (size_t i = 0; i < it->server_name.size(); i++)
							{
								for (size_t i_2 = 0; i_2 < it_2->server_name.size(); i_2++)
								{
									if (it->server_name[i] == it_2->server_name[i_2])
										return false;
								}
							}
						}
					}
				}

			}
		}
	}
	return true;
}

void ConfigParser::extractUniqueListen(void)
{
	for (std::vector<ServerConfig>::iterator it = _configServ.begin(); it != _configServ.end(); it++)//Идем по каждому серверу
	{
		if (_uniqueListen.empty())//Если список _uniqueListen пустой скидываем ему сразу все listen первого сервера.
		{
			_uniqueListen = it->listen;
			continue;
		}
		for(size_t i = 0; i < it->listen.size(); i++)//Идем по listen текущего сервера
		{
			bool isUnique = true;
			for (std::vector<ListenStruct>::iterator it_u = _uniqueListen.begin(); it_u != _uniqueListen.end();) //сравниваем с каждым элементом в _uniqueListen
			{
				if (it_u->port == it->listen[i].port)//Если port в listen совпадает с тем что находиться в _uniqueListen
				{
					if (it_u->ip == it->listen[i].ip || it_u->ip == "0.0.0.0")//Если у них совпадают Ip или ip в _uniqueListen перекрывает новый listen  
					{
						isUnique = false;//Отмечаем что текущий Listen не уникальный
						break;//Прерываем цикл 
					}
					if (it->listen[i].ip == "0.0.0.0")//Если текущий listen имет перекрывающий IP
					{
						it_u = _uniqueListen.erase(it_u);//Удаляем более направленый ip из _uniqueListen
						continue;//прододжаем проверку
					}
				}
				it_u++;
			}
			if (isUnique)//Если текущий listen уникальный вставляем его в контейнер
				_uniqueListen.push_back(it->listen[i]);
		}
	}
}

void ConfigParser::checkPort(ServerConfig& serverData)//Проверяем что не используем привелигированный порт
{
	if (serverData.listen.empty())
	{
		defineDefaultListen(serverData);//Если listen пустой то ставим дефолтный
		return;
	}
	for (std::vector<ListenStruct>::iterator it = serverData.listen.begin(); it != serverData.listen.end(); it++)
	{
		if (it->port < 1024 && it->port != 80 && it->port != 443)
			throw std::runtime_error("Error: wrong port!");
	}
}

void ConfigParser::defineDefaultListen(ServerConfig& serverData)//Функция назначает дефодный listen
{
	ListenStruct temp;
	temp.ip = "0.0.0.0";//Все IP
	temp.port = 80;//port hppt
	serverData.listen.push_back(temp);
}

void ConfigParser::defineDefaultMethods(LocationStruct& location)//Функция добавляет в allow_methods все 3 метода как разрещенные , если в location allow_methods не был прописан
{
	location.allow_methods.push_back("GET");
	location.allow_methods.push_back("POST");
	location.allow_methods.push_back("DELETE");
}

bool ConfigParser::validateServerName(std::string& name)//Проверяет имя домена на соответствие RFC
{
	if (name.empty())
		return false;
	if (name[0] == '.' || name[name.size() - 1 ] == '.')
		return false;
	for (size_t i = 0; i < name.size(); i++)
	{
		if (!std::isalpha(name[i]) && name[i] != '.' && name[i] != '-' && name[i] != '_')
			return false;
	}
	return true;
}