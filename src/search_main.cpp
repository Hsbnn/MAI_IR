#include <cstdio>
#include <cstring>
#include "utils.h"
#include "index.h"
#include "parser.h"
#include "search.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Использование: %s <index.bin> [queries.txt]\n", argv[0]);
        printf("\nЕсли queries.txt не указан, работает в интерактивном режиме.\n");
        return 1;
    }
    
    const char* index_file = argv[1];
    
    printf("\n");
    printf("  ПОИСКОВАЯ СИСТЕМА ФОРМУЛА-1\n");
    
    // Загружаем индекс
    printf("Загрузка индекса...\n");
    InvertedIndex index;
    
    if (!index.load_from_file(index_file)) {
        printf("ОШИБКА: Не удалось загрузить индекс\n");
        return 1;
    }
    
    printf("\n");
    index.print_stats();
    
    SearchEngine engine(&index);
    
    if (argc >= 3) {
        const char* queries_file = argv[2];
        
        FILE* f = fopen(queries_file, "r");
        if (!f) {
            printf("ОШИБКА: Не удалось открыть файл запросов %s\n", queries_file);
            return 1;
        }
        
        char line[1024];
        int query_num = 0;
        
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            
            if (strlen(line) == 0) continue;
            
            query_num++;
            printf("\n--- Запрос #%d: %s ---\n", query_num, line);
            
            DynamicArray<int>* results = engine.search(line);
            engine.print_results(results, 0, 10);
            
            delete results;
        }
        
        fclose(f);
    } else {
        printf("Интерактивный режим поиска\n");
        printf("Введите запрос (или 'exit' для выхода):\n\n");
        
        printf("Примеры запросов:\n");
        printf("  ferrari\n");
        printf("  ferrari && mercedes\n");
        printf("  (ferrari || mercedes) && !mclaren\n");
        printf("  \"formula one\"\n");
        printf("  \"formula one\" / 3\n\n");
        
        char line[1024];
        
        while (true) {
            printf("Запрос> ");
            fflush(stdout);
            
            if (!fgets(line, sizeof(line), stdin)) break;
            
            line[strcspn(line, "\n")] = 0;
            
            if (strlen(line) == 0) continue;
            if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
            
            DynamicArray<int>* results = engine.search(line);
            engine.print_results(results, 0, 50);
            
            delete results;
            printf("\n");
        }
    }
    
    printf("\nЗавершение работы.\n");
    
    return 0;
}
