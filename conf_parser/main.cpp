#include "ConfigParser.hpp"


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "Error: Wrong number of argument!" << std::endl;
		return 1;
	}
	ConfigParser config;
	try
	{
		config.parseConfigFile(argv[1]);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return (0);
}

