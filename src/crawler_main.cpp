#include <cstdio>
#include <cstring>
#include "crawler.h"

void print_usage(const char* prog) {
    printf("Использование: %s [опции]\n\n", prog);
    printf("Опции:\n");
    printf("  -s, --seed <url>       Начальный URL (можно указать несколько)\n");
    printf("  -f, --file <file>      Файл с начальными URL (по одному на строку)\n");
    printf("  -o, --output <file>    Выходной файл (по умолчанию: crawled_data.jsonl)\n");
    printf("  -n, --max-pages <N>    Максимум страниц (по умолчанию: 1000)\n");
    printf("  -d, --max-depth <N>    Максимальная глубина (по умолчанию: 3)\n");
    printf("  -t, --delay <N>        Задержка между запросами в сек (по умолчанию: 1)\n");
    printf("  -h, --help             Показать эту справку\n\n");
    printf("Примеры:\n");
    printf("  %s -s https://example.com -n 100\n", prog);
    printf("  %s -f seeds.txt -o output.jsonl -n 5000 -d 2\n", prog);
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    DynamicArray<char*> seeds;
    const char* output = "crawled_data.jsonl";
    int max_pages = 1000;
    int max_depth = 3;
    int delay = 1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seed") == 0) {
            if (i + 1 < argc) {
                char* seed = (char*)malloc(strlen(argv[i+1]) + 1);
                strcpy(seed, argv[i+1]);
                seeds.push(seed);
                i++;
            }
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
            if (i + 1 < argc) {
                FILE* f = fopen(argv[i+1], "r");
                if (f) {
                    char line[2048];
                    while (fgets(line, sizeof(line), f)) {
                        line[strcspn(line, "\n")] = 0;
                        if (strlen(line) > 0) {
                            char* seed = (char*)malloc(strlen(line) + 1);
                            strcpy(seed, line);
                            seeds.push(seed);
                        }
                    }
                    fclose(f);
                } else {
                    printf("Ошибка открытия файла: %s\n", argv[i+1]);
                }
                i++;
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                output = argv[i+1];
                i++;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--max-pages") == 0) {
            if (i + 1 < argc) {
                max_pages = atoi(argv[i+1]);
                i++;
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--max-depth") == 0) {
            if (i + 1 < argc) {
                max_depth = atoi(argv[i+1]);
                i++;
            }
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--delay") == 0) {
            if (i + 1 < argc) {
                delay = atoi(argv[i+1]);
                i++;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    if (seeds.length() == 0) {
        printf("Ошибка: не указаны начальные URL\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    WebCrawler crawler(max_pages, max_depth, delay);
    
    for (int i = 0; i < seeds.length(); i++) {
        crawler.add_seed(seeds[i]);
        free(seeds[i]);
    }
    
    crawler.start(output);
    
    return 0;
}
