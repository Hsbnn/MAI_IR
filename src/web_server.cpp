#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils.h"
#include "index.h"
#include "search.h"

#define PORT 8080
#define BUFFER_SIZE 65536

void url_decode(char* dst, const char* src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && 
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void html_escape(char* dst, const char* src, int max_len) {
    int pos = 0;
    while (*src && pos < max_len - 10) {
        if (*src == '<') {
            strcpy(dst + pos, "&lt;");
            pos += 4;
        } else if (*src == '>') {
            strcpy(dst + pos, "&gt;");
            pos += 4;
        } else if (*src == '&') {
            strcpy(dst + pos, "&amp;");
            pos += 5;
        } else if (*src == '"') {
            strcpy(dst + pos, "&quot;");
            pos += 6;
        } else {
            dst[pos++] = *src;
        }
        src++;
    }
    dst[pos] = '\0';
}

void send_main_page(int client_socket) {
    const char* html = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html><head>\n"
        "<meta charset='utf-8'>\n"
        "<title>Поиск по Формуле-1</title>\n"
        "<style>\n"
        "body { font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; padding: 20px; }\n"
        "h1 { color: #E10600; }\n"
        ".search-box { margin: 30px 0; }\n"
        "input[type=text] { width: 70%; padding: 10px; font-size: 16px; }\n"
        "input[type=submit] { padding: 10px 20px; font-size: 16px; background: #E10600; color: white; border: none; cursor: pointer; }\n"
        ".examples { background: #f5f5f5; padding: 15px; margin: 20px 0; border-radius: 5px; }\n"
        "</style>\n"
        "</head><body>\n"
        "<h1>🏎️ Поиск по корпусу Формулы-1</h1>\n"
        "<div class='search-box'>\n"
        "<form action='/search' method='GET'>\n"
        "  <input type='text' name='q' placeholder='Введите запрос...' autofocus>\n"
        "  <input type='submit' value='Поиск'>\n"
        "</form>\n"
        "</div>\n"
        "<div class='examples'>\n"
        "<b>Примеры запросов:</b><br>\n"
        "• <code>ferrari</code> — поиск термина<br>\n"
        "• <code>ferrari && mercedes</code> — оба термина<br>\n"
        "• <code>ferrari || mclaren</code> — любой из терминов<br>\n"
        "• <code>!ferrari</code> — без Ferrari<br>\n"
        "• <code>\"formula one\"</code> — точная фраза<br>\n"
        "• <code>\"formula one\" / 3</code> — фраза с допуском<br>\n"
        "</div>\n"
        "</body></html>\n";
    
    send(client_socket, html, strlen(html), 0);
}

void send_search_results(int client_socket, SearchEngine* engine, 
                        const char* query, int page) {
    
    DynamicArray<int>* results = engine->search(query);
    
    int total = results->length();
    int per_page = 50;
    int offset = page * per_page;
    
    char* response = (char*)malloc(BUFFER_SIZE);
    int pos = 0;
    
    pos += snprintf(response + pos, BUFFER_SIZE - pos,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html><head>\n"
        "<meta charset='utf-8'>\n"
        "<title>Результаты поиска: %s</title>\n"
        "<style>\n"
        "body { font-family: Arial, sans-serif; max-width: 900px; margin: 20px auto; padding: 20px; }\n"
        "h1 { color: #E10600; }\n"
        ".search-box { margin: 20px 0; }\n"
        "input[type=text] { width: 70%%; padding: 10px; font-size: 16px; }\n"
        "input[type=submit] { padding: 10px 20px; font-size: 16px; background: #E10600; color: white; border: none; }\n"
        ".result { margin: 20px 0; padding: 15px; border-left: 3px solid #E10600; background: #f9f9f9; }\n"
        ".result h3 { margin: 0 0 10px 0; }\n"
        ".result a { color: #1a0dab; text-decoration: none; font-size: 18px; }\n"
        ".result a:hover { text-decoration: underline; }\n"
        ".url { color: #006621; font-size: 14px; }\n"
        ".stats { color: #70757a; margin: 20px 0; }\n"
        ".pagination { margin: 30px 0; }\n"
        ".pagination a { padding: 5px 10px; margin: 0 5px; background: #f1f1f1; text-decoration: none; }\n"
        "</style>\n"
        "</head><body>\n"
        "<h1>🏎️ Поиск по Формуле-1</h1>\n"
        "<div class='search-box'>\n"
        "<form action='/search' method='GET'>\n"
        "  <input type='text' name='q' value='%s'>\n"
        "  <input type='submit' value='Поиск'>\n"
        "</form>\n"
        "</div>\n"
        "<div class='stats'>Найдено результатов: %d (%.4f сек)</div>\n",
        query, query, total, 0.0);
    
    InvertedIndex* idx = engine->get_index();
    
    int end = offset + per_page;
    if (end > total) end = total;
    
    for (int i = offset; i < end && pos < BUFFER_SIZE - 1000; i++) {
        int doc_id = (*results)[i];
        Document* doc = idx->get_document(doc_id);
        
        if (doc) {
            char escaped_title[512];
            html_escape(escaped_title, doc->title, sizeof(escaped_title));
            
            pos += snprintf(response + pos, BUFFER_SIZE - pos,
                "<div class='result'>\n"
                "  <h3><a href='%s' target='_blank'>%s</a></h3>\n"
                "  <div class='url'>%s</div>\n"
                "</div>\n",
                doc->url, escaped_title, doc->url);
        }
    }
    
    if (total > per_page) {
        pos += snprintf(response + pos, BUFFER_SIZE - pos, "<div class='pagination'>\n");
        
        if (page > 0) {
            pos += snprintf(response + pos, BUFFER_SIZE - pos,
                "<a href='/search?q=%s&page=%d'>« Предыдущая</a>\n", query, page - 1);
        }
        
        pos += snprintf(response + pos, BUFFER_SIZE - pos, "Страница %d из %d\n", 
                       page + 1, (total + per_page - 1) / per_page);
        
        if (end < total) {
            pos += snprintf(response + pos, BUFFER_SIZE - pos,
                "<a href='/search?q=%s&page=%d'>Следующая »</a>\n", query, page + 1);
        }
        
        pos += snprintf(response + pos, BUFFER_SIZE - pos, "</div>\n");
    }
    
    pos += snprintf(response + pos, BUFFER_SIZE - pos, "</body></html>\n");
    
    send(client_socket, response, pos, 0);
    
    free(response);
    delete results;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Использование: %s <index.bin>\n", argv[0]);
        return 1;
    }
    
    const char* index_file = argv[1];
    
    printf("Загрузка индекса...\n");
    InvertedIndex index;
    
    if (!index.load_from_file(index_file)) {
        printf("ОШИБКА загрузки индекса\n");
        return 1;
    }
    
    SearchEngine engine(&index);
    
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }
    
    if (listen(server_socket, 10) < 0) {
        perror("listen");
        return 1;
    }
    
    printf("\n✓ Веб-сервер запущен на http://localhost:%d\n", PORT);
    printf("Нажмите Ctrl+C для остановки\n\n");
    
    char buffer[BUFFER_SIZE];
    
    while (true) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            close(client_socket);
            continue;
        }
        
        buffer[bytes_read] = '\0';
        
        char method[16], path[512];
        sscanf(buffer, "%s %s", method, path);
        
        printf("Request: %s %s\n", method, path);
        
        if (strcmp(path, "/") == 0) {
            send_main_page(client_socket);
        } else if (strncmp(path, "/search", 7) == 0) {
            char* query_start = strchr(path, '?');
            char query[512] = "";
            int page = 0;
            
            if (query_start) {
                query_start++;
                
                char* q_param = strstr(query_start, "q=");
                if (q_param) {
                    q_param += 2;
                    char* end = strchr(q_param, '&');
                    if (end) {
                        int len = end - q_param;
                        strncpy(query, q_param, len);
                        query[len] = '\0';
                    } else {
                        strcpy(query, q_param);
                    }
                    
                    url_decode(query, query);
                }
                
                char* p_param = strstr(query_start, "page=");
                if (p_param) {
                    page = atoi(p_param + 5);
                }
            }
            
            send_search_results(client_socket, &engine, query, page);
        } else {
            const char* not_found = 
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<h1>404 Not Found</h1>\n";
            send(client_socket, not_found, strlen(not_found), 0);
        }
        
        close(client_socket);
    }
    
    close(server_socket);
    return 0;
}
