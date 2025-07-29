#include "Cgi.hpp"


Cgi::Cgi(): RequestHandler() {}

Cgi::Cgi(ConfigParser& config, HttpRequest request, int port, std::string ip, const std::string& host): RequestHandler(config), _config(config)
{
    this->_server = NULL;
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
        this->_config = src._config;
        this->_data_rec = src._data_rec;
        this->_server = src._server;
	    this->_location = src._location;
        this->_exec_args = src._exec_args;
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

void Cgi::findQuery(void)//Функия для поиска переменной окружения QUERY_STRING и корректирования URL
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
        if (!_data_rec.index_url.empty())//Если сработала директива index, то берем url из нее
            _data_rec.url = _data_rec.index_url;
        else
            _data_rec.url = _data_rec.req.getUri();//Если нет то из запроса
    }

    _exec_args.data_envp.query = query;
}

void  Cgi::findPathInfo(void)
{
    std::string path_info = "";
    size_t pos = _data_rec.url.find_last_of('.');//Находим индекс начала расширения.
    if (pos == std::string::npos)
        return;
    size_t pos_2 = _data_rec.url.find('/', pos);//Проверяем на наличие path_info
    if (pos_2 != std::string::npos)
    {
        path_info = _data_rec.url.substr(pos_2);//Если нашли то отрезаем нужную часть
        _data_rec.url = _data_rec.url.substr(0, pos_2);
    }
    _data_rec.ext = _data_rec.url.substr(pos);
    _exec_args.data_envp.path_info = path_info;
}

bool Cgi::isCgi(void)
{
    _server = _findServerConfig(_data_rec.port, _data_rec.ip, _data_rec.host);//Ищем наш сервер
    if (_server == NULL)
    {
        std::cerr << "Error: dont find server for cgi" << std::endl;
        return false;
    }

    _data_rec.url = _data_rec.req.getUri();
    _location = _findLocationFor(*_server, _data_rec.url);//Ищем location запроса
    if (_location == NULL)//Если у запроса нет location 
        return false;

    size_t pos = _data_rec.req.getUri().find('.');
    if (pos == std::string::npos)
    {
        if (!_location->index.empty() && !_location->cgi.empty())//Если есть деректива индекс и cgi не пустой то проверяем их расширение
        {
            for (size_t y = 0; y < _location->index.size(); y++)
            {    
                size_t pos = _location->index[y].find_last_of('.');
                if (pos == std::string::npos)
                    continue;
                std::string index_ext = _location->index[y].substr(pos);
                for (size_t i = 0; i < _location->cgi.size(); i++)//Проходим по всем директивам cgi в location
                {
                    if (_location->cgi[i].extension == index_ext)//Если расширение директивы cgi совпадает с расширением url то все ок запускаем cgi
                    {
                        _data_rec.index_url = _data_rec.req.getUri() + _location->index[y];
                        findQuery();//Отделяем query от url и сохраняем 
                        findPathInfo();//Отделяем path_info от url и сохраняем .Сохраняем расширение.
                        _data_rec.ext = index_ext;
                        _exec_args.data_cgi.extension = _data_rec.ext;//Расширение
                        _exec_args.data_cgi.pathInterpreter = _location->cgi[i].pathInterpreter;//Путь к интерпретатору
                        _exec_args.data_cgi.timeout = _location->cgi[i].timeout;//timeout

                        return true;
                    }
                }
            }
        }
        else
            return false;
    }
    findQuery();//Отделяем query от url и сохраняем 
    findPathInfo();//Отделяем path_info от url и сохраняем .Сохраняем расширение.


    for (size_t i = 0; i < _location->cgi.size(); i++)//Проходим по всем директивам cgi в location
    {
        if (_location->cgi[i].extension == _data_rec.ext)//Если расширение директивы cgi совпадает с расширением url то все ок запускаем cgi
        {
            _exec_args.data_cgi.extension = _data_rec.ext;//Расширение
            _exec_args.data_cgi.pathInterpreter = _location->cgi[i].pathInterpreter;//Путь к интерпретатору
            _exec_args.data_cgi.timeout = _location->cgi[i].timeout;//timeout
            return true;
        }
    }
    return false;
}

bool Cgi::checkMethod(void)
{
    std::string rec_method = _data_rec.req.getMethod();

    if (rec_method != "POST" && rec_method != "GET")
        return false;

    for (size_t i = 0; i < _location->allow_methods.size(); i++)
    {
        if (_location->allow_methods[i] == rec_method)
        return true;    
    } 

    return false;
}

std::string Cgi::checkPath(std::string& path)
{
    std::vector<std::string> parts_path;
    std::stringstream ss(path);
    std::string item;
    std::string result = "/";

    while (std::getline(ss, item, '/'))
    {
        if (item.empty() || item == ".")
            continue;
        if (item == "..")
        {
            if (!parts_path.empty())
                parts_path.pop_back();
        }
        else
            parts_path.push_back(item);
    }


    for(size_t i = 0; i < parts_path.size(); i++)
    {
        result += parts_path[i];
        if (i != parts_path.size() - 1)
            result += "/";
    }

    return result;
}

std::string Cgi::findScriptFilename(void)//Функция находит абсолютный путь к файлу
{
    /*Обрезаем лишние от абсолютного пути*/
    std::string path = _data_rec.url.substr(_location->prefix.length());//Удаляем префикс

    char cwd[1000];
    if (!getcwd(cwd, sizeof(cwd)))//находим абсолютный путь до корневого каталога
        throw std::runtime_error("500 Internal Server Error");

    std::string absolut_root = _location->root;
    if (absolut_root[0] != '/')
        absolut_root =  "/" + absolut_root;

    std::string script_filename = std::string(cwd) + absolut_root + path; //Создаем абсолютный путь к файлу

    script_filename = checkPath(script_filename);//Чистим путь от лишних / или .
    if (script_filename.find(std::string(cwd)) != 0)//Проверяем не вышли ли за корневой каталог
        throw std::runtime_error("403 Forbidden");
    
    /* Блок проверок файла */
    if (access(script_filename.c_str(), F_OK) != 0)//Проверка существует ли файл
		throw std::runtime_error("404 Not Found");//Не выходить полностью а возвращать страницу с 404

    if (access(script_filename.c_str(),  R_OK) != 0)//Проверка можно ли его читать
		throw std::runtime_error("403 Forbidden");

    if (_data_rec.ext == ".cgi" && access(script_filename.c_str(), X_OK) != 0)
        throw std::runtime_error("403 Forbidden");

    struct stat st;
    if (stat(script_filename.c_str(), &st) != 0)
        throw std::runtime_error("404 Not Found!");//Проверка файл это или каталог

    if (!S_ISREG(st.st_mode))
        throw std::runtime_error("403 Forbidden!");//Проверка существует файл или нет 

    _exec_args.path_relative = script_filename.substr(script_filename.find_last_of('/') + 1);//Отделяем имя скрипта для передачи в execve , относительный путь;
    _exec_args.path_absolut = script_filename;//Сохраняем абсолютный путь

    return script_filename;
}

void Cgi::createCgiEnvp(void)
{
    /*Блок в котором собираем переменные окружения CGI для execve*/
    _exec_args.envs_strings.clear();
    if (checkMethod())
    {
        _exec_args.envs_strings.push_back("REQUEST_METHOD=" + _data_rec.req.getMethod());//Метод запроса

        if (_data_rec.index_url.empty())
            _exec_args.envs_strings.push_back("REQUEST_URI=" + _data_rec.req.getUri());//Url запроса начиная от prefixa заканчивая query если есть  ///uploads/index.php/rrr?name=tt
        else
            _exec_args.envs_strings.push_back("REQUEST_URI=" + _data_rec.index_url);
        _exec_args.envs_strings.push_back(_exec_args.data_envp.query);// Все что после вопроса //name=tt
        _exec_args.envs_strings.push_back("PATH_INFO=" + _exec_args.data_envp.path_info);//Путь после имени файла до query //rrr
        _exec_args.envs_strings.push_back("SCRIPT_NAME=" + _data_rec.url);//prefix + имя скрипта //uploads/index.php
        _exec_args.envs_strings.push_back("SCRIPT_FILENAME=" + findScriptFilename());//Абсолютный путь до скрипта 
        _exec_args.envs_strings.push_back("REDIRECT_STATUS=200");
        _exec_args.envs_strings.push_back("SERVER_PROTOCOL=" + _data_rec.req.getHttpVersion());
        _exec_args.envs_strings.push_back("SERVER_NAME=" + _data_rec.host);
        _exec_args.envs_strings.push_back("SERVER_PORT=" + to_string_98(_data_rec.port));
        _exec_args.envs_strings.push_back("REMOTE_ADDR=" + _data_rec.ip);
        _exec_args.envs_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
        const std::map<std::string, std::string>& headers = _data_rec.req.getHeaders();
        std::map<std::string, std::string>::const_iterator it;

        it = headers.find("user-agent");
        if (it!= headers.end())
            _exec_args.envs_strings.push_back("HTTP_USER_AGENT=" + it->second);
        
        it = headers.find("accept");
        if (it!= headers.end())
            _exec_args.envs_strings.push_back("HTTP_ACCEPT=" + it->second);

        /*Блок поиска заголовков content-type и content_type*/
        if (_data_rec.req.getMethod() == "POST")
        {
            std::string content_length = "0";
            std::string content_type = "application/x-www-form-urlencoded";
            it = headers.find("content-length");//Ставим итератор на позицию ключа в контейнере
            if (it!= headers.end())//Если ключ найдет , передаем значение.Если нет передаем дефолтное значение 0
                content_length = it->second;
            it = headers.find("content-type"); //Тоже самое только с другим ключем. Если нет то опять дефолтное значение запроса html
            if (it != headers.end())
                content_type = it->second;
            _exec_args.envs_strings.push_back("CONTENT_LENGTH=" + content_length);
            _exec_args.envs_strings.push_back("CONTENT_TYPE=" + content_type);
        }

        if (!_location->upload_path.empty() && _location->upload_path != _location->root)
        {
            std::string path;
            if (_location->upload_path[0] == '/')
                path = _location->upload_path; //У нас изначально абсолютный путь
            else
            {
                std::string dir = _exec_args.path_absolut.substr(0, _exec_args.path_absolut.find_last_of('/'));//Если относительный пусть , то берем путь до текущего каталога
                path = dir + "/" + _location->upload_path;//Совмещаем с относительным
            }
            path = checkPath(path);
            _exec_args.envs_strings.push_back("UPLOAD_PATH=" + path);
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

void Cgi::findArgsExecve(void)//В этой функции готовим аргументы для execve
{
    _exec_args.argv_strings.clear();
    _exec_args.argv_ptrs.clear();
    

    if (_data_rec.req.getContentLength() > 0)//Проверка на client_max_body_size
    {
        if (_data_rec.req.getContentLength() > _location->client_max_body_size)
            throw std::runtime_error("413 Payload Too Large");
    }

    if (access(_exec_args.data_cgi.pathInterpreter.c_str(), X_OK) != 0)
        throw std::runtime_error("500 Internal Server Error: Intepretor not exec");

    if (_exec_args.data_cgi.extension == ".cgi")
    {
        _exec_args.argv_strings.push_back(_exec_args.path_relative);
        _exec_args.data_cgi.pathInterpreter = _exec_args.path_absolut;
    }
    else 
    {
        std::string prog_name = _exec_args.data_cgi.pathInterpreter.substr(_exec_args.data_cgi.pathInterpreter.find_last_of('/') + 1);
        _exec_args.argv_strings.push_back(prog_name);//Одельно имя программы
        _exec_args.argv_strings.push_back(_exec_args.path_relative);//Адрес файла который нужно запустить
    }

    for (size_t i = 0; i < _exec_args.argv_strings.size(); i++)
    {
        _exec_args.argv_ptrs.push_back(const_cast<char*>(_exec_args.argv_strings[i].c_str()));
    }
    _exec_args.argv_ptrs.push_back(NULL);
}

std::string Cgi::executeScript(void)
{
    findArgsExecve();

    int pipe_in[2];//Через этот пайп передаем данные из родительского процесса в дочерний 
    int pipe_out[2];//Через этот выводим данные из дочернего в родительский 
    if (pipe(pipe_in) == -1)
        throw std::runtime_error("500 Internal Server Error");
        
    if (pipe(pipe_out) == -1)
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        throw std::runtime_error("500 Internal Server Error");
    }

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
        std::string script_dir = _exec_args.path_absolut.substr(0, _exec_args.path_absolut.find_last_of('/'));//Адрес репертория в котором находиться скрипт, по типу /home/mikerf/projects_group/temp/www/html/uploads
        if (chdir(script_dir.c_str()) != 0)//Проходим в каталог со скриптом, теперь при вызове execve с аргументом в виде относительного пути , скрипт без проблем найдется в текущем каталоге
        {
            exit(1);
        }    

        execve(_exec_args.data_cgi.pathInterpreter.c_str(), _exec_args.argv_ptrs.data(), _exec_args.envs_ptrs.data());//Запускаем скрипт в интерпретаторе
            exit(1);//Если не смогли выполнить функцию execve то закрываем дочерний процесс с ошибкой
    }
    else
    {
        close(pipe_in[0]);
        close(pipe_out[1]);

        if (_data_rec.req.getMethod() == "POST")//Отправляем тело post в скрипт 
        {
            size_t total_write = 0;
            std::string body_str = _data_rec.req.getBody();
            const char* body = body_str.c_str();
            size_t to_write = body_str.length();

            if (!_data_rec.req.getBody().empty())
            {
                while (total_write < to_write)//Читаем пока не дойдем до конца 
                {
                    ssize_t written = write(pipe_in[1], body + total_write, to_write - total_write);
                    if (written == -1 || written == 0)
                    {
                        close(pipe_in[1]);
                        close(pipe_out[0]);
                        int status;
                        waitpid(pid, &status, 0);
                        throw std::runtime_error("500 Internal Server Error: Error: Write() returned unxpected");
                    }
                    total_write = total_write + written;
                }
            }
        }

        close(pipe_in[1]);

        int timeout_ms = 1000 * _exec_args.data_cgi.timeout;
        struct pollfd fd;
        fd.fd = pipe_out[0];
        fd.events = POLLIN;

        int poll_result = poll(&fd, 1, timeout_ms);
        if (poll_result == 0)
        {
            int status;
            if (waitpid(pid, &status, WNOHANG) == 0)
            {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
            }
            close(pipe_out[0]);
            throw std::runtime_error("504 Gateway Timeout");
        }

        if (poll_result < 0)
        {
            int status;
            if (waitpid(pid, &status, WNOHANG) == 0)
            {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
            }
            close(pipe_out[0]);
            throw std::runtime_error("500 Internal Server Error: poll failed");
        }

        std::string repense;
        char buffer[4096];
        ssize_t bytes;

        while((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
        { 
            repense.append(buffer, bytes);
        }

        close(pipe_out[0]);
        int status;
        waitpid(pid, &status, 0);

        if (bytes == -1)
            throw std::runtime_error("500 Internal Server Error: Read() returned -1"); 

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
    else if (error.find("413") != std::string::npos)
    {
        status_error = "413 Payload Too Large";
        body_error = "<h1>413 Payload Too Large</h1>";
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
            if (headers.find("Content-Type") == std::string::npos)
                response += "Content-Type: text/html\r\n";
            http_response += response;
            http_response += "\r\n";
            http_response += body;
        }
        else
        {
            http_response += "Content-Type: text/html\r\n";
            http_response += "Content-Length: " + to_string_98(output.length()) + "\r\n";
            http_response += "\r\n";
            http_response += output;
        }
//        printEnvpCgi();
//        std::cout << "\nhttp_response\n" << http_response << std::endl;
        return http_response;
    }
    catch (const std::exception &e)
    {
        std::string error = e.what();
        return composeErrorResponse(error);
    }
}
