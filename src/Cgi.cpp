#include "Cgi.hpp"

Cgi::Cgi(){}

Cgi::Cgi(const ConfigParser& config, int port, int ip, const std::string& host): RequestHandler(config)
{
    this->_server = _findServerConfig(port, ip, host);
    this->_data_rec.host = host;
    this->_data_rec.ip = ip;
    this->_data_rec.port = port;
}

Cgi::Cgi(const Cgi& src)
{
	*this = src
}
Cgi& Cgi::operator=(const Cgi& src)
{
    if (this != &src)
    {
        this->_cgi_ext = src._cgi_ext;
        this->_data_rec = src._data_rec;
        this->_server = src._server;
    }
    return *this;
}
Cgi::~Cgi(void){};

bool Cgi::isCgi(std::string url)
{
    size_t pos = url.find_last_of('.');
    if (pos != std::string::npos)
    {
        std::string ext = url.substr(pos);
        if (ext == ".php" || ext == ".py" || ext == ".pl")
        {
            _cgi_ext = ext;
        }
            return true;
    }
    return false;
}

char** createCgiEnvp(HttpRequest rec)
{
    std::vector<std::string> envs;

    envs.push_back("REQUEST_METHOD=" + rec.getMethod());
    envs.push_back("QUERY_STRING=" + rec.getHeaders()["query_string"]);//проверить
    envs.push_back("SCRIPT_FILENAME=");
    envs.push_back("PATH_INFO=");
    envs.push_back("CONTENT_LENGTH=" + rec.getHeaders()["content-length"]);//проверить
    envs.push_back("CONTENT_TYPE=" + rec.getHeaders()["content-type"]);//проверить
    envs.push_back("REDIRECT_STATUS=200");

}