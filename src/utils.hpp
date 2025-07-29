#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

std::string normalizeUri(const std::string& uri);
std::string generateAutoindex(const std::string& absPath, const std::string& uri);
std::string trim(const std::string& s);
std::string toLower(std::string s);


#endif
