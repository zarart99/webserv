#include "Cgi.hpp"


Cgi::Cgi(): RequestHandler() {}

Cgi::Cgi(ConfigParser& config, HttpRequest request, int port, std::string ip, const std::string& host): RequestHandler(config), _config(config)
{
    this->_server = _findServerConfig(port, ip, host);
    if (this->_server == NULL)
        throw std::runtime_error("500 Internal Server Error");
    this->_data_rec.req = request;
    this->_data_rec.host = host;
    this->_data_rec.ip = ip;
    this->_data_rec.port = port;

}

Cgi::Cgi(const Cgi& src): RequestHandler(src._config)
{
	*this = src;
}
Cgi& Cgi::operator=(const Cgi& src)
{
    if (this != &src)
    {
        this->_data_rec = src._data_rec;
        this->_server = src._server;
	    this->_location = src._location;
        this->_config = src._config;
        this->_exec_args.cgi_ext = src._exec_args.cgi_ext;
        this->_exec_args.path_script = src._exec_args.path_script;
        this->_exec_args.envs_ptrs = src._exec_args.envs_ptrs;
        this->_exec_args.envs_strings = src._exec_args.envs_strings;
        this->_exec_args.interpreter = src._exec_args.interpreter;
        this->_exec_args.argv_ptrs = src._exec_args.argv_ptrs;
    }
    return *this;
}

Cgi::~Cgi(void){}

std::string to_string_98(int val)
{
    std::ostringstream str;
    str << val;
    return str.str();
}

bool Cgi::isCgi(std::string url)
{
    size_t pos = url.find_last_of('?');//Проверяем на наличие query string
    if (pos != std::string::npos)
        url = url.substr(0, pos);//Если нашли то обрезаем

    pos = url.find_last_of('.');
    size_t pos_2 = url.find_last_of('/');//Проверяем на наличие path_info
    if (pos < pos_2)
        url = url.substr(0, pos_2);//Если нашли то обрезаем
    if (pos != std::string::npos)
    {
        std::string ext = url.substr(pos);
        if (ext == ".php" || ext == ".py" || ext == ".cgi")
        {
            _exec_args.cgi_ext = ext;
            return true;
        }
    }
    return false;
}

std::string Cgi::findScriptFilename(void)//Функция находит абсолютный путь к файлу
{
    const LocationStruct *curLocation = _findLocationFor(*_server, _data_rec.url);//Ищем location запроса
    if (curLocation == NULL)
        throw std::runtime_error("404 Not Found//location");//как здесь выйти , найти файл ошибки или сгенерить свою
    _location = curLocation;

    /*Обрезаем лишние от абсолютного пути*/
    
    std::string path = _data_rec.url.substr(curLocation->prefix.length());//Удаляем префикс

    size_t pos = path.find_first_of('.');
    if (pos != std::string::npos)
    {
        size_t query = path.find_first_of('?');//Удаляем query если есть
        if (query != std::string::npos)
        {
            path = path.substr(0, query);
        }
        size_t ext_end = path.find('/', pos);//Удаляем path_info
        if (ext_end != std::string::npos)
        {
            path = path.substr(0, ext_end);
        }

    }

//    std::string srcipt_filename = curLocation->root.substr(1) + path;//отночительный путь к файлу
    char cwd[500];
    if (!getcwd(cwd, sizeof(cwd)))
        throw std::runtime_error("500 Internal Server Error");
    std::string absolut_root = curLocation->root;
    if (absolut_root[0] != '/')
        absolut_root =  "/" + absolut_root;

    std::string srcipt_filename = std::string(cwd) + absolut_root + path; //Создаем абсолютный путь к файлу

    /* Блок проверок файла */
    if (access(srcipt_filename.c_str(), F_OK) != 0)//Проверка существует ли файл
		throw std::runtime_error("404 Not Found /access F_OK");//Не выходить полностью а возвращать страницу с 404

    struct stat st;
    if (stat(srcipt_filename.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
        throw std::runtime_error("404 Not Found!");//Проверка файл это или каталог

    if (access(srcipt_filename.c_str(),  R_OK) != 0)//Проверка можно ли его читать
		throw std::runtime_error("403 Forbidden");
    _exec_args.path_script = srcipt_filename;
    return srcipt_filename;
}

std::string Cgi::findQuery(void)//Функия для поиска переменной окружения QUERY_STRING и корректирования URL
{
    size_t pos = _data_rec.req.getUri().find('?');
    std::string query;

    if (pos != std::string::npos)//Если нашли нужный символ
    {
        query = "QUERY_STRING=" + _data_rec.req.getUri().substr(pos + 1);//Присоединяем все что после него
        _data_rec.url = _data_rec.req.getUri().substr(0 , pos);//Меняем url
    }
    else
    {
        query = "QUERY_STRING=";//Если символа нет то оставляем переменную пустой
        _data_rec.url = _data_rec.req.getUri();
    }

    return query;
}

std::string  Cgi::findPathInfo(void)
{
    std::string path_info = "";
    size_t pos = _data_rec.url.find_last_of('.');
    size_t pos_2 = _data_rec.url.find('/', pos);//Проверяем на наличие path_info
    if (pos_2 != std::string::npos)
    {
        path_info = _data_rec.url.substr(pos_2);//Если нашли то отрезаем нужную часть
        _data_rec.url = _data_rec.url.substr(0, pos_2);
    }
    return path_info;
}

void Cgi::createCgiEnvp(void)
{
    std::string query = findQuery();

    /*Блок поиска заголовков content-type и content_type*/
    std::string content_length = "0";
    std::string content_type = "application/x-www-form-urlencoded";
    std::map<std::string , std::string>::const_iterator it = _data_rec.req.getHeaders().find("content-length");//Ставим итератор на позицию ключа в контейнере
    if (it != _data_rec.req.getHeaders().end())//Если ключ найдет , передаем значение.Если нет передаем дефолтное значение 0
        content_length = it->second;
    it = _data_rec.req.getHeaders().find("content-type"); //Тоже самое только с другим ключем. Если нет то опять дефолтное значение запроса html
    if (it != _data_rec.req.getHeaders().end())
        content_type = it->second;

    /*Блок в котором собираем окружение CGI для execve*/
    _exec_args.envs_strings.clear();
    if (_data_rec.req.getMethod() == "GET" || _data_rec.req.getMethod() == "POST")
    {
        _exec_args.envs_strings.push_back("REQUEST_METHOD=" + _data_rec.req.getMethod());
        _exec_args.envs_strings.push_back("REQUEST_URI=" + _data_rec.req.getUri());
        _exec_args.envs_strings.push_back(query);
        _exec_args.envs_strings.push_back("PATH_INFO=" + findPathInfo());
        _exec_args.envs_strings.push_back("SCRIPT_NAME=" + _data_rec.url);
        _exec_args.envs_strings.push_back("SCRIPT_FILENAME=" + findScriptFilename());
        _exec_args.envs_strings.push_back("REDIRECT_STATUS=200");
        _exec_args.envs_strings.push_back("SERVER_PROTOCOL=" + _data_rec.req.getHttpVersion());
        _exec_args.envs_strings.push_back("SERVER_NAME=" + _data_rec.host);
        _exec_args.envs_strings.push_back("SERVER_PORT=" + to_string_98(_data_rec.port));
        _exec_args.envs_strings.push_back("REMOTE_ADDR=" + _data_rec.ip);
        _exec_args.envs_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
        if (_data_rec.req.getMethod() == "POST")
        {
            _exec_args.envs_strings.push_back("CONTENT_LENGTH=" + content_length);
            _exec_args.envs_strings.push_back("CONTENT_TYPE=" + content_type);
        }
    }
    else
        throw std::runtime_error("Error: 405 Method Not Allowed");
    _exec_args.envs_ptrs.clear();
    for (size_t i = 0; i < _exec_args.envs_strings.size(); i++)
    {
        _exec_args.envs_ptrs.push_back(const_cast<char*>(_exec_args.envs_strings[i].c_str()));
    }
    _exec_args.envs_ptrs.push_back(NULL);
}

void Cgi::printEnvpCgi(void)
{
    std::cout << "\nENVP FOR CGI\n";
    for (size_t i = 0; i < _exec_args.envs_strings.size(); i++)
    {
        std::cout << _exec_args.envs_strings[i] << std::endl;
    }
    std::cout << "---------------\n";
}

char** Cgi::getEnvp(void)
{
    return &_exec_args.envs_ptrs[0];
}

void Cgi::findArgsExecve(void)//В этой функции готовим аргументы для execve
{
    _exec_args.argv_strings.clear();
    _exec_args.argv_ptrs.clear();


    if (_exec_args.cgi_ext == ".php") //Выбераем нужное нам расширение
    {
        _exec_args.interpreter = "/usr/bin/php-cgi";//Находим path интерпретатора
        _exec_args.argv_strings.push_back("php-cgi");//Одельно имя программы
        _exec_args.argv_strings.push_back(_exec_args.path_script);//Адрес файла который нужно запустить
    }
    else if (_exec_args.cgi_ext  == ".py")
    {
        _exec_args.interpreter = "/usr/bin/python3";
        _exec_args.argv_strings.push_back("python3");
        _exec_args.argv_strings.push_back(_exec_args.path_script);
    }
    else if (_exec_args.cgi_ext  == ".cgi")
    {
        _exec_args.interpreter = _exec_args.path_script;
        _exec_args.argv_strings.push_back(_exec_args.path_script);
    }
    else
        throw std::runtime_error("500 Internal Server Error");

    for (size_t i = 0; i < _exec_args.argv_strings.size(); i++)
    {
        _exec_args.argv_ptrs.push_back(const_cast<char*>(_exec_args.argv_strings[i].c_str()));
    }
    _exec_args.argv_ptrs.push_back(NULL);
}

std::string Cgi::executeScript(void)
{
    findArgsExecve();
    std::vector<char*> argv_copy;
    std::vector<char*> envp_copy;

    for (size_t i = 0; i < _exec_args.argv_strings.size(); i++)
    {
        argv_copy.push_back(const_cast<char*>(_exec_args.argv_strings[i].c_str()));
    }
    argv_copy.push_back(NULL);

    for (size_t i = 0; i < _exec_args.envs_strings.size(); i++)
    {
        envp_copy.push_back(const_cast<char*>(_exec_args.envs_strings[i].c_str()));
    }
    envp_copy.push_back(NULL);

    int pipe_in[2];//Через этот пайп передаем данные из родительского процесса в дочерний 
    int pipe_out[2];//Через этот выводим данные из дочернего в родительский 
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
        throw std::runtime_error("500 Internal Server Error");
    pid_t pid = fork();
    if (pid == -1)
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        throw std::runtime_error("500 Internal Server Error");        
    }
    if (pid == 0)
    {
        close(pipe_in[1]);//Закрываем лишние пайпы
        close(pipe_out[0]);

        //перенаправляем ввод и вывод на пайпы
        if (dup2(pipe_in[0], STDIN_FILENO) == -1 || dup2(pipe_out[1], STDOUT_FILENO) == -1)
        {   
            close(pipe_in[0]);
            close(pipe_out[1]);
            exit(1);
        }


        close(pipe_in[0]);//Закрываем уже не нужные пайпы
        close(pipe_out[1]);
        execve(_exec_args.interpreter.c_str(), argv_copy.data(), envp_copy.data());//Запускаем скрипт в интерпретаторе
        exit(1);//Если не смогли выполнить функцию execve то закрываем дочерний процесс с ошибкой
    }
    else
    {
        close(pipe_in[0]);
        close(pipe_out[1]);

        if (_data_rec.req.getMethod() == "POST")//Отправляем тело post в скрипт 
        {
            std::string body = _data_rec.req.getBody();
            if (!body.empty())
            {
                ssize_t written = write(pipe_in[1], body.c_str(), body.length());
                if (written == -1)
                {
                    close(pipe_in[1]);
                    close(pipe_out[0]);
                    int status;
                    waitpid(pid, &status, 0);
                    throw std::runtime_error("500 Internal Server Error: CGI script failed");
                }
            }
            
        }
        close(pipe_in[1]);

        std::string repense;
        char buffer[4096];
        size_t bytes;

        while((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
        { 
            repense.append(buffer, bytes);
        }
        close(pipe_out[0]);

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            throw std::runtime_error("500 Internal Server Error: CGI script failed");
        else if (WIFSIGNALED(status))
            throw std::runtime_error("500 Internal Server Error: CGI script termined by signal");
        return repense;
    }
}

std::string Cgi::composeErrorResponse(const std::string& error)
{
    std::string status_error;
    std::string body_error;
    if (error.find("404") != std::string::npos)
    {
        status_error = "404 Not Found";
        body_error = "<h1>404 Not Found</h1>";
    }
    else if (error.find("403") != std::string::npos)
    {
        status_error = "403 Forbidden";
        body_error = "<h1>403 Forbidden</h1>";
    }   
    else if (error.find("405") != std::string::npos)
    {
        status_error = "405 Method Not Allowed";
        body_error = "<h1>405 Method Not Allowed</h1>";
    }
    else
    {
        status_error = "500 Internal Server Error";
        body_error = "<h1>500 Internal Server Error</h1>";
    }
    std::string response_error = "HTTP/1.1 " + status_error + "\r\n"
                                + "Content-Type: text/html\r\n"
                                + "Content-Length: " + to_string_98(body_error.length())
                                + "\r\n\r\n" + body_error; 
    return response_error;
}

std::string Cgi::cgiHandler(void)
{
    try
    {
        createCgiEnvp();
        std::string output = executeScript();
        if (output.empty())
            throw std::runtime_error("500 Internal Server Error: CGI returned empty output");
        std::string http_response = "HTTP/1.1 200 OK\r\n";
        std::string response;
        size_t header_end = output.find("\r\n\r\n");
        if (header_end != std::string::npos)
        {
            std::string headers = output.substr(0, header_end);
            std::string body = output.substr(header_end + 4);

            std::stringstream ss(headers);
            std::string line;
            while (std::getline(ss, line))
            {
                if (line.empty() || line == "\r")
                    break;
                
                size_t pos_colon = line.find(':');
                if (pos_colon != std::string::npos)
                {
                    std::string key = line.substr(0, pos_colon);
                    std::string value = line.substr(pos_colon + 1);

                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \r\t\n") + 1);

                    if (key == "Status")
                        http_response = "HTTP/1.1 " + value + "\r\n";
                    else
                        response += key + ": " + value + "\r\n";
                }
            }
            if (headers.find("Content-Length") == std::string::npos)
                response += "Content-Length: " + to_string_98(body.length()) + "\r\n";
            http_response += response;
            http_response += "\r\n";
            http_response += body;
        }
        else
        {
            http_response += "Content-Type: text/html\r\n";
            http_response += "Content-Length:" + to_string_98(output.length()) + "\r\n";
            http_response += "\r\n";
            http_response += output;
        }
                printEnvpCgi();
                std::cout << "\nhttp_response\n" << http_response << std::endl;
        return http_response;
    }
    catch (const std::exception &e)
    {
        std::string error = e.what();
        return composeErrorResponse(error);
    }
}

