#ifndef CRAWLER_H
#define CRAWLER_H

#include "utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_URL_LENGTH 2048
#define MAX_HTML_SIZE 1024*1024 
#define USER_AGENT "F1RacingBot/1.0 (+http://example.com/bot.html)"

struct URL {
    char protocol[16]; 
    char host[256];
    int port;
    char path[1024];
    int depth; 
    
    URL() : port(80), depth(0) {
        protocol[0] = '\0';
        host[0] = '\0';
        path[0] = '\0';
    }
    
    bool parse(const char* url_str) {
        const char* p = url_str;
        
        const char* colon = strstr(p, "://");
        if (colon) {
            int len = colon - p;
            if (len < 16) {
                strncpy(protocol, p, len);
                protocol[len] = '\0';
            }
            p = colon + 3;
        } else {
            strcpy(protocol, "http");
        }
        
        const char* slash = strchr(p, '/');
        const char* colon2 = strchr(p, ':');
        
        if (colon2 && (!slash || colon2 < slash)) {
            int len = colon2 - p;
            if (len < 256) {
                strncpy(host, p, len);
                host[len] = '\0';
            }
            port = atoi(colon2 + 1);
            p = slash ? slash : (p + strlen(p));
        } else if (slash) {
            int len = slash - p;
            if (len < 256) {
                strncpy(host, p, len);
                host[len] = '\0';
            }
            p = slash;
        } else {
            strncpy(host, p, 255);
            host[255] = '\0';
            p = p + strlen(p);
        }
        
        if (*p) {
            strncpy(path, p, 1023);
            path[1023] = '\0';
        } else {
            strcpy(path, "/");
        }
        
        if (port == 80 && strcmp(protocol, "https") == 0) {
            port = 443;
        }
        
        return host[0] != '\0';
    }
    
    void normalize() {
        for (int i = 0; host[i]; i++) {
            if (host[i] >= 'A' && host[i] <= 'Z') {
                host[i] += 32;
            }
        }
        
        int len = strlen(path);
        if (len > 1 && path[len-1] == '/') {
            path[len-1] = '\0';
        }
    }
    
    void to_string(char* buffer, int max_len) const {
        if (port == 80 || (port == 443 && strcmp(protocol, "https") == 0)) {
            snprintf(buffer, max_len, "%s://%s%s", protocol, host, path);
        } else {
            snprintf(buffer, max_len, "%s://%s:%d%s", protocol, host, port, path);
        }
    }
    
    void get_base_url(char* buffer, int max_len) const {
        if (port == 80 || (port == 443 && strcmp(protocol, "https") == 0)) {
            snprintf(buffer, max_len, "%s://%s", protocol, host);
        } else {
            snprintf(buffer, max_len, "%s://%s:%d", protocol, host, port);
        }
    }
};

class URLQueue {
private:
    struct Node {
        URL url;
        Node* next;
        
        Node(const URL& u) : url(u), next(nullptr) {}
    };
    
    Node* head;
    Node* tail;
    int size;
    
public:
    URLQueue() : head(nullptr), tail(nullptr), size(0) {}
    
    ~URLQueue() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }
    
    void push(const URL& url) {
        Node* node = new Node(url);
        
        if (!tail) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        
        size++;
    }
    
    bool pop(URL& url) {
        if (!head) return false;
        
        url = head->url;
        
        Node* temp = head;
        head = head->next;
        
        if (!head) tail = nullptr;
        
        delete temp;
        size--;
        
        return true;
    }
    
    bool empty() const {
        return head == nullptr;
    }
    
    int length() const {
        return size;
    }
};

class VisitedSet {
private:
    HashMap<bool> visited;
    
public:
    VisitedSet() {}
    
    bool contains(const char* url) {
        bool dummy;
        return visited.get(url, dummy);
    }
    
    void add(const char* url) {
        visited.put(url, true);
    }
    
    int size() const {
        return visited.getSize();
    }
};

class HTTPClient {
private:
    int connect_socket(const char* host, int port, int timeout_sec = 10) {
        struct hostent* server = gethostbyname(host);
        if (!server) {
            printf("[HTTP] Ошибка разрешения имени: %s\n", host);
            return -1;
        }
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            printf("[HTTP] Ошибка создания сокета\n");
            return -1;
        }
        
        struct timeval tv;
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        serv_addr.sin_port = htons(port);
        
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("[HTTP] Ошибка подключения к %s:%d\n", host, port);
            close(sock);
            return -1;
        }
        
        return sock;
    }
    
public:
    char* fetch(const URL& url, int& status_code) {
        status_code = 0;
        
        if (strcmp(url.protocol, "http") != 0) {
            printf("[HTTP] Пропуск HTTPS URL: %s://%s\n", url.protocol, url.host);
            return nullptr;
        }
        
        int sock = connect_socket(url.host, url.port);
        if (sock < 0) return nullptr;
        
        char request[4096];
        snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: %s\r\n"
            "Accept: text/html\r\n"
            "Connection: close\r\n"
            "\r\n",
            url.path, url.host, USER_AGENT);
        
        if (send(sock, request, strlen(request), 0) < 0) {
            printf("[HTTP] Ошибка отправки запроса\n");
            close(sock);
            return nullptr;
        }
        
        char* buffer = (char*)malloc(MAX_HTML_SIZE);
        int total_bytes = 0;
        int bytes;
        
        while ((bytes = recv(sock, buffer + total_bytes, MAX_HTML_SIZE - total_bytes - 1, 0)) > 0) {
            total_bytes += bytes;
            if (total_bytes >= MAX_HTML_SIZE - 1) break;
        }
        
        buffer[total_bytes] = '\0';
        
        close(sock);
        
        if (total_bytes == 0) {
            free(buffer);
            return nullptr;
        }
        
        if (strncmp(buffer, "HTTP/", 5) == 0) {
            const char* space = strchr(buffer, ' ');
            if (space) {
                status_code = atoi(space + 1);
            }
        }
        
        const char* body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4;
            memmove(buffer, body, strlen(body) + 1);
        }
        
        return buffer;
    }
};

class HTMLParser {
private:
    bool is_space(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
    
    bool is_tag_char(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
               (c >= '0' && c <= '9') || c == '-' || c == '_';
    }
    
public:
    char* extract_text(const char* html) {
        int html_len = strlen(html);
        char* text = (char*)malloc(html_len + 1);
        int text_pos = 0;
        
        bool in_tag = false;
        bool in_script = false;
        bool in_style = false;
        
        for (int i = 0; i < html_len; i++) {
            if (!in_tag && html[i] == '<') {
                in_tag = true;
                
                if (strncasecmp(html + i, "<script", 7) == 0) {
                    in_script = true;
                } else if (strncasecmp(html + i, "</script>", 9) == 0) {
                    in_script = false;
                    i += 8;
                    in_tag = false;
                } else if (strncasecmp(html + i, "<style", 6) == 0) {
                    in_style = true;
                } else if (strncasecmp(html + i, "</style>", 8) == 0) {
                    in_style = false;
                    i += 7;
                    in_tag = false;
                }
                
                continue;
            }
            
            if (in_tag && html[i] == '>') {
                in_tag = false;
                
                if (text_pos > 0 && text[text_pos-1] != ' ') {
                    text[text_pos++] = ' ';
                }
                
                continue;
            }
            
            if (!in_tag && !in_script && !in_style) {
                if (html[i] == '&') {
                    if (strncmp(html + i, "&nbsp;", 6) == 0) {
                        text[text_pos++] = ' ';
                        i += 5;
                    } else if (strncmp(html + i, "&lt;", 4) == 0) {
                        text[text_pos++] = '<';
                        i += 3;
                    } else if (strncmp(html + i, "&gt;", 4) == 0) {
                        text[text_pos++] = '>';
                        i += 3;
                    } else if (strncmp(html + i, "&amp;", 5) == 0) {
                        text[text_pos++] = '&';
                        i += 4;
                    } else if (strncmp(html + i, "&quot;", 6) == 0) {
                        text[text_pos++] = '"';
                        i += 5;
                    } else {
                        text[text_pos++] = html[i];
                    }
                } else {
                    text[text_pos++] = html[i];
                }
            }
        }
        
        text[text_pos] = '\0';
        
        int write_pos = 0;
        bool last_was_space = true;
        
        for (int i = 0; i < text_pos; i++) {
            if (is_space(text[i])) {
                if (!last_was_space) {
                    text[write_pos++] = ' ';
                    last_was_space = true;
                }
            } else {
                text[write_pos++] = text[i];
                last_was_space = false;
            }
        }
        
        text[write_pos] = '\0';
        
        return text;
    }
    
    DynamicArray<char*>* extract_links(const char* html, const URL& base_url) {
        DynamicArray<char*>* links = new DynamicArray<char*>();
        
        const char* p = html;
        
        while ((p = strchr(p, '<'))) {
            p++;
            
            if (tolower(p[0]) == 'a' && is_space(p[1])) {
                p += 2;
                
                while (*p && *p != '>') {
                    while (*p && is_space(*p)) p++;
                    
                    if (strncasecmp(p, "href", 4) == 0) {
                        p += 4;
                        while (*p && is_space(*p)) p++;
                        
                        if (*p == '=') {
                            p++;
                            while (*p && is_space(*p)) p++;
                            
                            char quote = *p;
                            if (quote == '"' || quote == '\'') {
                                p++;
                                
                                char url_buf[2048];
                                int url_len = 0;
                                
                                while (*p && *p != quote && url_len < 2047) {
                                    url_buf[url_len++] = *p++;
                                }
                                
                                url_buf[url_len] = '\0';
                                
                                char absolute[2048];
                                
                                if (strstr(url_buf, "://")) {
                                    strcpy(absolute, url_buf);
                                } else if (url_buf[0] == '/') {
                                    char base[512];
                                    base_url.get_base_url(base, sizeof(base));
                                    snprintf(absolute, sizeof(absolute), "%s%s", base, url_buf);
                                } else {
                                    char base[512];
                                    base_url.to_string(base, sizeof(base));
                                    
                                    char* last_slash = strrchr(base, '/');
                                    if (last_slash && last_slash > base + 8) {
                                        *(last_slash + 1) = '\0';
                                    } else {
                                        strcat(base, "/");
                                    }
                                    
                                    snprintf(absolute, sizeof(absolute), "%s%s", base, url_buf);
                                }
                                
                                char* hash = strchr(absolute, '#');
                                if (hash) *hash = '\0';
                                
                                if (strstr(absolute, "://") && 
                                    strncmp(absolute, "http://", 7) == 0 ||
                                    strncmp(absolute, "https://", 8) == 0) {
                                    
                                    char* link = (char*)malloc(strlen(absolute) + 1);
                                    strcpy(link, absolute);
                                    links->push(link);
                                }
                            }
                        }
                    }
                    
                    while (*p && *p != '>' && !is_space(*p)) p++;
                }
            }
        }
        
        return links;
    }
    
    char* extract_title(const char* html) {
        const char* title_start = strcasestr(html, "<title");
        if (!title_start) return strdup("Untitled");
        
        const char* content_start = strchr(title_start, '>');
        if (!content_start) return strdup("Untitled");
        content_start++;
        
        const char* title_end = strcasestr(content_start, "</title>");
        if (!title_end) return strdup("Untitled");
        
        int len = title_end - content_start;
        if (len > 500) len = 500;
        
        char* title = (char*)malloc(len + 1);
        strncpy(title, content_start, len);
        title[len] = '\0';
        
        while (*title && is_space(*title)) {
            memmove(title, title + 1, strlen(title));
        }
        
        int title_len = strlen(title);
        while (title_len > 0 && is_space(title[title_len-1])) {
            title[--title_len] = '\0';
        }
        
        return title;
    }
    
private:
    const char* strcasestr(const char* haystack, const char* needle) {
        int needle_len = strlen(needle);
        
        for (int i = 0; haystack[i]; i++) {
            bool match = true;
            for (int j = 0; j < needle_len; j++) {
                if (tolower(haystack[i+j]) != tolower(needle[j])) {
                    match = false;
                    break;
                }
            }
            if (match) return haystack + i;
        }
        
        return nullptr;
    }
    
    int tolower(int c) {
        return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
    }
};

class RobotsTxt {
private:
    struct Rule {
        char path[512];
        bool allow;
    };
    
    DynamicArray<Rule> rules;
    int crawl_delay;
    
public:
    RobotsTxt() : crawl_delay(0) {}
    
    bool parse(const char* content) {
        const char* p = content;
        bool our_section = false;
        
        while (*p) {
            char line[1024];
            int line_len = 0;
            
            while (*p && *p != '\n' && line_len < 1023) {
                line[line_len++] = *p++;
            }
            
            line[line_len] = '\0';
            if (*p == '\n') p++;
            
            char* comment = strchr(line, '#');
            if (comment) *comment = '\0';
            
            char* trimmed = line;
            while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
            
            if (*trimmed == '\0') continue;
            
            if (strncasecmp(trimmed, "User-agent:", 11) == 0) {
                const char* agent = trimmed + 11;
                while (*agent == ' ' || *agent == '\t') agent++;
                
                if (strcmp(agent, "*") == 0 || strcasestr(agent, "F1RacingBot")) {
                    our_section = true;
                } else {
                    our_section = false;
                }
            } else if (our_section) {
                if (strncasecmp(trimmed, "Disallow:", 9) == 0) {
                    const char* path = trimmed + 9;
                    while (*path == ' ' || *path == '\t') path++;
                    
                    if (*path) {
                        Rule rule;
                        strncpy(rule.path, path, 511);
                        rule.path[511] = '\0';
                        rule.allow = false;
                        rules.push(rule);
                    }
                } else if (strncasecmp(trimmed, "Allow:", 6) == 0) {
                    const char* path = trimmed + 6;
                    while (*path == ' ' || *path == '\t') path++;
                    
                    if (*path) {
                        Rule rule;
                        strncpy(rule.path, path, 511);
                        rule.path[511] = '\0';
                        rule.allow = true;
                        rules.push(rule);
                    }
                } else if (strncasecmp(trimmed, "Crawl-delay:", 12) == 0) {
                    const char* delay = trimmed + 12;
                    while (*delay == ' ' || *delay == '\t') delay++;
                    crawl_delay = atoi(delay);
                }
            }
        }
        
        return true;
    }
    
    bool is_allowed(const char* path) {
        for (int i = 0; i < rules.length(); i++) {
            if (strncmp(path, rules[i].path, strlen(rules[i].path)) == 0) {
                return rules[i].allow;
            }
        }
        
        return true;
    }
    
    int get_crawl_delay() const {
        return crawl_delay;
    }
    
private:
    const char* strcasestr(const char* haystack, const char* needle) {
        int needle_len = strlen(needle);
        
        for (int i = 0; haystack[i]; i++) {
            bool match = true;
            for (int j = 0; j < needle_len; j++) {
                char h = haystack[i+j];
                char n = needle[j];
                if (h >= 'A' && h <= 'Z') h += 32;
                if (n >= 'A' && n <= 'Z') n += 32;
                if (h != n) {
                    match = false;
                    break;
                }
            }
            if (match) return haystack + i;
        }
        
        return nullptr;
    }
};

class WebCrawler {
private:
    URLQueue queue;
    VisitedSet visited;
    HTTPClient http;
    HTMLParser parser;
    HashMap<RobotsTxt*> robots_cache;
    HashMap<long> last_access_time;
    
    int max_pages;
    int max_depth;
    int crawl_delay;
    int fetched_count;
    int error_count;
    
    double total_download_time;
    long long total_bytes;
    
    FILE* output_file;
    
    bool should_crawl(const URL& url) {
        if (url.depth > max_depth) return false;
        
        char url_str[2048];
        url.to_string(url_str, sizeof(url_str));
        
        if (visited.contains(url_str)) return false;
        
        RobotsTxt* robots = get_robots(url);
        if (robots && !robots->is_allowed(url.path)) {
            printf("[ROBOTS] Запрещено: %s\n", url_str);
            return false;
        }
        
        return true;
    }
    
    RobotsTxt* get_robots(const URL& url) {
        RobotsTxt* robots = nullptr;
        
        if (robots_cache.get(url.host, robots)) {
            return robots;
        }
        
        URL robots_url = url;
        strcpy(robots_url.path, "/robots.txt");
        
        int status;
        char* content = http.fetch(robots_url, status);
        
        robots = new RobotsTxt();
        
        if (content && status == 200) {
            robots->parse(content);
        }
        
        if (content) free(content);
        
        robots_cache.put(url.host, robots);
        
        return robots;
    }
    
    void respect_crawl_delay(const URL& url) {
        long now = time(nullptr);
        long last_access = 0;
        
        if (last_access_time.get(url.host, last_access)) {
            long elapsed = now - last_access;
            
            RobotsTxt* robots = get_robots(url);
            int delay = crawl_delay;
            
            if (robots && robots->get_crawl_delay() > delay) {
                delay = robots->get_crawl_delay();
            }
            
            if (elapsed < delay) {
                int sleep_time = delay - elapsed;
                printf("[DELAY] Ожидание %d сек перед запросом к %s\n", sleep_time, url.host);
                sleep(sleep_time);
            }
        }
        
        last_access_time.put(url.host, now);
    }
    
public:
    WebCrawler(int max_p = 1000, int max_d = 3, int delay = 1)
        : max_pages(max_p), max_depth(max_d), crawl_delay(delay),
          fetched_count(0), error_count(0), total_download_time(0), total_bytes(0),
          output_file(nullptr) {}
    
    ~WebCrawler() {
        if (output_file) fclose(output_file);
        
        auto it = robots_cache.iterator();
        while (it.hasNext()) {
            delete it.value();
            it.next();
        }
    }
    
    void add_seed(const char* url_str) {
        URL url;
        if (url.parse(url_str)) {
            url.normalize();
            queue.push(url);
        } else {
            printf("[ERROR] Неверный URL: %s\n", url_str);
        }
    }
    
    void start(const char* output_filename) {
        output_file = fopen(output_filename, "w");
        if (!output_file) {
            printf("[ERROR] Не удалось открыть файл для записи: %s\n", output_filename);
            return;
        }
        
        printf("Макс. страниц: %d\n", max_pages);
        printf("Макс. глубина: %d\n", max_depth);
        printf("Задержка: %d сек\n", crawl_delay);
        printf("Выходной файл: %s\n", output_filename);
        
        Timer total_timer;
        
        while (!queue.empty() && fetched_count < max_pages) {
            URL url;
            if (!queue.pop(url)) break;
            
            if (!should_crawl(url)) continue;
            
            char url_str[2048];
            url.to_string(url_str, sizeof(url_str));
            visited.add(url_str);
            
            respect_crawl_delay(url);
            
            printf("[%d/%d] Загрузка: %s\n", fetched_count + 1, max_pages, url_str);
            
            Timer fetch_timer;
            int status;
            char* html = http.fetch(url, status);
            double fetch_time = fetch_timer.elapsed();
            
            if (!html) {
                error_count++;
                printf("  [ERROR] Ошибка загрузки\n");
                continue;
            }
            
            if (status != 200) {
                printf("  [ERROR] HTTP %d\n", status);
                free(html);
                error_count++;
                continue;
            }
            
            int html_size = strlen(html);
            total_download_time += fetch_time;
            total_bytes += html_size;
            
            char* text = parser.extract_text(html);
            char* title = parser.extract_title(html);
            
            DynamicArray<char*>* links = parser.extract_links(html, url);
            
            printf("  [OK] %d байт, %.2f сек, %d ссылок\n", 
                   html_size, fetch_time, links->length());
            printf("  Заголовок: %s\n", title);
            
            fprintf(output_file, "{");
            fprintf(output_file, "\"url\":\"%s\",", url_str);
            fprintf(output_file, "\"title\":\"");
            
            for (char* p = title; *p; p++) {
                if (*p == '"' || *p == '\\') {
                    fputc('\\', output_file);
                }
                fputc(*p, output_file);
            }
            
            fprintf(output_file, "\",\"text\":\"");
            
            int text_len = 0;
            for (char* p = text; *p && text_len < 50000; p++) {
                if (*p == '"' || *p == '\\') {
                    fputc('\\', output_file);
                    text_len++;
                }
                if (*p == '\n') {
                    fputs("\\n", output_file);
                    text_len += 2;
                } else {
                    fputc(*p, output_file);
                    text_len++;
                }
            }
            
            fprintf(output_file, "\",\"depth\":%d,\"status\":%d}\n", url.depth, status);
            fflush(output_file);
            
            for (int i = 0; i < links->length(); i++) {
                URL new_url;
                if (new_url.parse((*links)[i])) {
                    new_url.normalize();
                    new_url.depth = url.depth + 1;
                    
                    char new_url_str[2048];
                    new_url.to_string(new_url_str, sizeof(new_url_str));
                    
                    if (!visited.contains(new_url_str)) {
                        queue.push(new_url);
                    }
                }
                
                free((*links)[i]);
            }
            
            delete links;
            free(html);
            free(text);
            free(title);
            
            fetched_count++;
        }
        
        double total_time = total_timer.elapsed();
        
        printf("Загружено страниц: %d\n", fetched_count);
        printf("Ошибок: %d\n", error_count);
        printf("Уникальных URL: %d\n", visited.size());
        printf("Всего байт: %lld (%.2f MB)\n", total_bytes, total_bytes / 1024.0 / 1024.0);
        printf("Общее время: %.2f сек\n", total_time);
        if (fetched_count > 0) {
            printf("Среднее время на страницу: %.2f сек\n", total_time / fetched_count);
            printf("Скорость загрузки: %.2f KB/сек\n", 
                   (total_bytes / 1024.0) / total_download_time);
        }
    }
};

#endif // CRAWLER_H