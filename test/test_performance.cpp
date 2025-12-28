#include <cstdio>
#include <cstring>
#include "../src/utils.h"
#include "../src/index.h"
#include "../src/search.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <index.bin>\n", argv[0]);
        return 1;
    }
    
    printf("Загрузка индекса: %s\n", argv[1]);
    Timer load_timer;
    
    InvertedIndex index;
    if (!index.load_from_file(argv[1])) {
        printf("ОШИБКА загрузки индекса\n");
        return 1;
    }
    
    printf("Время загрузки: %.4f сек\n\n", load_timer.elapsed());
    
    index.print_stats();
    
    SearchEngine engine(&index);
    
    const char* queries[] = {
        "ferrari",
        "ferrari && mercedes",
        "ferrari || mercedes || mclaren",
        "(ferrari || mercedes) && championship",
        "!ferrari",
        "\"formula one\"",
        "\"formula one\" / 5",
        "ferrari && mercedes && !mclaren",
        "racing",
        "championship"
    };
    
    int num_queries = sizeof(queries) / sizeof(queries[0]);
    
    printf("  Тестирование запросов\n");
    
    double total_time = 0;
    int total_results = 0;
    
    for (int i = 0; i < num_queries; i++) {
        printf("[%d/%d] Query: %s\n", i + 1, num_queries, queries[i]);
        
        Timer query_timer;
        DynamicArray<int>* results = engine.search(queries[i]);
        double elapsed = query_timer.elapsed();
        
        total_time += elapsed;
        total_results += results->length();
        
        printf("        Time: %.6f sec\n", elapsed);
        printf("        Results: %d documents\n\n", results->length());
        
        delete results;
    }
    
    printf("Всего запросов: %d\n", num_queries);
    printf("Общее время: %.6f сек\n", total_time);
    printf("Среднее время: %.6f сек/запрос\n", total_time / num_queries);
    printf("Найдено результатов: %d\n", total_results);
    
    printf("  (1000 запросов)\n");
    
    Timer stress_timer;
    int stress_results = 0;
    
    for (int i = 0; i < 1000; i++) {
        const char* query = queries[i % num_queries];
        DynamicArray<int>* results = engine.search(query);
        stress_results += results->length();
        delete results;
        
        if ((i + 1) % 100 == 0) {
            printf("  Выполнено: %d запросов\n", i + 1);
        }
    }
    
    double stress_elapsed = stress_timer.elapsed();
    
    printf("\nРезультаты стресс-теста:\n");
    printf("  Время: %.4f сек\n", stress_elapsed);
    printf("  Запросов/сек: %.2f\n", 1000.0 / stress_elapsed);
    printf("  Среднее время: %.6f сек\n", stress_elapsed / 1000.0);
    
    return 0;
}
