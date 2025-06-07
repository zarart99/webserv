
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <iostream>
#include <string>

void run_test(const std::string &test_name, const std::string &raw_request)
{
    std::cout << "=================================================" << std::endl;
    std::cout << ">>> ЗАПУСК ТЕСТА: " << test_name << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Сырой запрос:\n"
              << raw_request << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;

    try
    {
        HttpRequest request(raw_request);

        std::cout << "[OK] Парсинг успешен!" << std::endl;
        std::cout << "   Метод: " << request.getMethod() << std::endl;
        std::cout << "   URI: " << request.getUri() << std::endl;
        std::cout << "   Версия: " << request.getHttpVersion() << std::endl;

        const std::map<std::string, std::string> &headers = request.getHeaders();
        std::cout << "   Заголовки (" << headers.size() << " шт.):" << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        {
            std::cout << "     - " << it->first << ": " << it->second << std::endl;
        }

        // Выводим тело
        if (!request.getBody().empty())
        {
            std::cout << "   Тело запроса: " << request.getBody() << std::endl;
        }
        else
        {
            std::cout << "   Тело запроса: [пусто]" << std::endl;
        }

        // --- Фаза 2: Генерация ответа ---
        HttpResponse response;
        response.setStatusCode(200);
        response.setStatusMessage("OK");
        response.addHeader("Content-Type", "text/plain");
        response.setBody("Request was successfully parsed!");

        std::cout << "-------------------------------------------------" << std::endl;
        std::cout << "Сгенерированный ответ:\n"
                  << response.buildResponse() << std::endl;
    }
    catch (const std::exception &e)
    {
        // --- Обработка ошибок парсинга ---
        std::cout << "[ERROR] Ошибка парсинга: " << e.what() << std::endl;

        int statusCode = 400; 
        std::string statusMessage = "Bad Request";

        std::string error_str = e.what();
        std::stringstream ss(error_str);
        ss >> statusCode; 

        if (statusCode == 501)
            statusMessage = "Not Implemented";
        if (statusCode == 505)
            statusMessage = "HTTP Version Not Supported";

        HttpResponse error_response;
        error_response.setStatusCode(statusCode);
        error_response.setStatusMessage(statusMessage);
        error_response.addHeader("Content-Type", "text/html");
        error_response.setBody("<html><body><h1>" + error_str + "</h1></body></html>");

        std::cout << "-------------------------------------------------" << std::endl;
        std::cout << "Сгенерированный ответ с ошибкой:\n"
                  << error_response.buildResponse() << std::endl;
    }
    std::cout << "=================================================\n"
              << std::endl;
}

int main()
{
    // Тест 1: Идеальный GET запрос без тела
    run_test("GET-запрос",
             "GET /index.html HTTP/1.1\r\n"
             "Host: localhost:8080\r\n"
             "User-Agent: curl/7.64.1\r\n"
             "Accept: */*\r\n\r\n");

    // Тест 2: POST запрос с телом
    run_test("POST-запрос с телом",
             "POST /submit_form HTTP/1.1\r\n"
             "Host: example.com\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: 27\r\n\r\n"
             "field1=value1&field2=value2");

    // Тест 3: Ошибка - неподдерживаемый метод
    run_test("Неподдерживаемый метод (PUT)",
             "PUT /data.json HTTP/1.1\r\n"
             "Host: api.example.com\r\n\r\n");

    // Тест 4: Ошибка - некорректная стартовая строка
    run_test("Некорректная стартовая строка",
             "GET /index.html\r\n" // Отсутствует версия HTTP
             "Host: localhost\r\n\r\n");

    // Тест 5: Пустой запрос
    run_test("Пустой запрос", "");

    return 0;
}
