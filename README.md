## Participant 2: HTTP Parser & Response Generator Checklist

### 1. HttpRequest (src/HttpRequest.cpp/.hpp)  
- [x] Разбор request-line (METHOD URI VERSION) с обработкой CRLF и лишних токенов → 400  
- [x] Поддержка только GET/POST/DELETE (иначе 501)  
- [x] Поддержка только HTTP/1.1 (иначе 505)  
- [x] Валидация URI (должен начинаться с `/`) → 400  
- [x] Парсинг заголовков: trim ключей/значений, lowercase у ключей, хранение в `_headers`  
- [x] Метод `getContentLength()` конвертирует `Content-Length` в число  
- [ ] Проверять обязательность заголовка `Host` для HTTP/1.1 → 400  
- [ ] Детектировать дублирование заголовков (повтор → 400)  
- [ ] Валидировать формат строк заголовков (отсутствие `:` → 400)  
- [ ] Читать тело ровно по `Content-Length`, а не до EOF  

### 2. HttpResponse (src/HttpResponse.cpp/.hpp)  
- [x] Статус-коды и сообщения через статический словарь  
- [x] Формирование стартовой строки `HTTP/1.1 <code> <message>\r\n`  
- [x] Авто-добавление `Content-Length`, если есть тело  
- [x] Добавление заголовков через `addHeader()`  
- [x] Для 405 Method Not Allowed добавить заголовок `Allow: GET, POST, DELETE`
- [x] Учесть HTTP-версию в ответе (505 для неподдерживаемых)

### 3. RequestHandler (src/RequestHandler.cpp/.hpp)  
#### GET  
- [x] `stat` + `access`, отдача файлов (200) или ошибок (404/403)  
- [x] Поиск и отдача index-файла в директории, иначе 403 (autoindex TODO)  
- [ ] Реализовать autoindex: генерация HTML-списка при отсутствии index  
- [ ] Расширить маппинг MIME-типов (см. ниже)  
- [ ] Защитить пути от `../` (нормализация + проверка выхода за root)  

#### POST  
- [x] Проверка `client_max_body_size` → 413  
- [x] Сохранение тела в файл + возврат 201 + `Location`  
- [ ] Парсер `application/x-www-form-urlencoded`  
- [ ] Полный парсер `multipart/form-data` (границы, поля, файлы)  
- [ ] Защита от directory traversal  
- [ ] Формирование корректного URL в заголовке `Location`  

#### DELETE  
- [x] Проверка существования → 404  
- [x] Проверка прав на удаление → 403  
- [x] Удаление через `std::remove` → 204  
- [ ] Дифференцировать ошибки `remove()` по `errno`:  
  - `ENOENT` → 404  
  - `EISDIR`/`EACCES` → 403  
  - прочие → 500  
- [ ] Обработка попытки удаления директории (403 или рекурсивно)  
- [ ] Защита от directory traversal  

#### Общие  
- [x] 405 для запрещённых в конфиге методов (без `Allow`)  
- [x] 501 для нераспознанных методов (парсер)  
- [x] 400/404/413/500 через `_createErrorResponse`, выбор кастомных страниц из конфига  
- [ ] Дефолтное тело ошибок, если кастомной страницы нет  

## Дополнительные задачи для участника 2 в проекте
- [ ] Расширить словарь MIME-типов, например:  
  ```cpp
  {
    {".html","text/html"},
    {".css","text/css"},
    {".js","application/javascript"},
    {".png","image/png"},
    {".jpg","image/jpeg"},
    {".json","application/json"},
    /* … */
  }
