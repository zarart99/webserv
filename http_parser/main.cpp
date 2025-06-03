// main.cpp
#include <iostream>
#include "Parser.hpp"

int main() {
    // Простая тестовая строка HTTP-запроса
    const std::string rawRequest =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    HttpRequest req = parseRequest(rawRequest);

    bool ok = true;
    if (req.method != GET) {
        std::cerr << "Method parsed incorrectly\n";
        ok = false;
    }
    if (req.uri != "/") {
        std::cerr << "URI parsed incorrectly: got \"" << req.uri << "\"\n";
        ok = false;
    }
    if (req.version != "HTTP/1.1") {
        std::cerr << "Version parsed incorrectly: got \"" << req.version << "\"\n";
        ok = false;
    }
    if (!req.headers.count("Host") || req.headers["Host"] != "example.com") {
        std::cerr << "Header Host parsed incorrectly\n";
        ok = false;
    }
    if (!req.body.empty()) {
        std::cerr << "Body should be empty for GET\n";
        ok = false;
    }

    if (ok) {
        std::cout << "parseRequest test: OK\n";
        return 0;
    } else {
        std::cout << "parseRequest test: FAIL\n";
        return 1;
    }
}
