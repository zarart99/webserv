#include "utils.hpp"
#include <sstream>
#include <vector>
#include <dirent.h>

static std::vector<std::string> split(const std::string &path, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, delim))
        elems.push_back(item);
    return elems;
}

//Убираем "." и корректно обрабатываем ".." в URI.
// Это защищает от выхода за пределы корневой папки сервера.
std::string normalizeUri(const std::string &uri)
{
    std::vector<std::string> parts = split(uri, '/');
    std::vector<std::string> out;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (parts[i].empty() || parts[i] == ".")
            continue;
        if (parts[i] == "..")
        {
            if (!out.empty())
                out.pop_back();  // поднимаемся на уровень выше
        }
        else
            out.push_back(parts[i]);
    }
    // Собираем обратно в строку с ведущим "/"
    std::string result = "/";
    for (size_t i = 0; i < out.size(); ++i)
    {
        result += out[i];
        if (i + 1 < out.size())
            result += "/";
    }
     // Сохраняем завершающий "/" если он был в исходном URI
    if (!out.empty() && uri[uri.length() - 1] == '/')
        result += "/";
    if (out.empty() && uri[uri.length() - 1] == '/')
        result = "/";
    return result;
}

// Открываем директорию absPath и пробегаемся по её записям.
// Формируем HTML-список ссылок на каждый файл/папку.
std::string generateAutoindex(const std::string &absPath, const std::string &uri)
{
    DIR *dir = opendir(absPath.c_str());
    if (!dir)
        return "<html><body>Directory listing error</body></html>";

    std::string html = "<html><body><ul>";
    struct dirent *entry;
    std::string base = uri;
    if (base.empty())
        base = "/";
    if (base[base.size() - 1] != '/')
        base += "/";
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        std::string href = base + name;
        if (entry->d_type == DT_DIR)
            href += "/";
        html += "<li><a href=\"" + href + "\">" + name + "</a></li>";
    }
    closedir(dir);
    html += "</ul></body></html>";
    return html;
}
