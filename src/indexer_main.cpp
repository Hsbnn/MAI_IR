#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include "utils.h"
#include "tokenizer.h"
#include "lemmatizer.h" 
#include "index.h"
#include "stats.h"

#define MAX_LINE 1024*1024

bool is_batch_file(const char* filename) {
    return strncmp(filename, "batch_", 6) == 0 && 
           strstr(filename, ".jsonl") != nullptr;
}

int process_batch_file(const char* filepath, Tokenizer& tokenizer, 
                       Lemmatizer& lemmatizer, 
                       InvertedIndex& index) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        printf("ОШИБКА: Не удалось открыть файл %s\n", filepath);
        return 0;
    }
    
    printf("  Обработка: %s\n", filepath);
    
    char* line = (char*)malloc(MAX_LINE);
    int doc_count = 0;
    
    while (fgets(line, MAX_LINE, f)) {
        char* title = extract_json_string(line, "title");
        char* text = extract_json_string(line, "text");
        char* pageid_str = extract_json_string(line, "pageid");
        char* source = extract_json_string(line, "source");
        char* lang = extract_json_string(line, "lang");
        
        if (!title || !text) {
            if (title) free(title);
            if (text) free(text);
            if (pageid_str) free(pageid_str);
            if (source) free(source);
            if (lang) free(lang);
            continue;
        }
        
        int pageid = pageid_str ? atoi(pageid_str) : 0;
        
        DynamicArray<Token>* tokens = tokenizer.tokenize(text);
        lemmatizer.lemmatize_all(tokens);
        
        char url[512];
        if (lang && strcmp(lang, "ru") == 0) {
            snprintf(url, sizeof(url), "https://ru.wikipedia.org/?curid=%d", pageid);
        } else if (lang && strcmp(lang, "de") == 0) {
            snprintf(url, sizeof(url), "https://de.wikipedia.org/?curid=%d", pageid);
        } else if (lang && strcmp(lang, "es") == 0) {
            snprintf(url, sizeof(url), "https://es.wikipedia.org/?curid=%d", pageid);
        } else if (lang && strcmp(lang, "it") == 0) {
            snprintf(url, sizeof(url), "https://it.wikipedia.org/?curid=%d", pageid);
        } else if (lang && strcmp(lang, "fr") == 0) {
            snprintf(url, sizeof(url), "https://fr.wikipedia.org/?curid=%d", pageid);
        } else {
            snprintf(url, sizeof(url), "https://en.wikipedia.org/?curid=%d", pageid);
        }
        
        index.add_document(title, url, pageid, tokens);
        
        delete tokens;
        doc_count++;
        
        free(title);
        free(text);
        if (pageid_str) free(pageid_str);
        if (source) free(source);
        if (lang) free(lang);
    }
    
    fclose(f);
    free(line);
    
    return doc_count;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Использование:\n");
        printf("  %s <directory> [--no-lemma]    # Индексировать все батчи в директории\n", argv[0]);
        printf("  %s <batch.jsonl> <out.bin>     # Индексировать один файл\n", argv[0]);
        printf("\nОпции:\n");
        printf("  --no-lemma    Отключить лемматизацию\n");
        printf("\nПримеры:\n");
        printf("  %s ./f1_racing_corpus_data\n", argv[0]);
        printf("  %s ./f1_racing_corpus_data --no-lemma\n", argv[0]);
        printf("  %s batch_0001.jsonl f1_index.bin\n", argv[0]);
        return 1;
    }
    
    const char* input_path = argv[1];
    char output_file[512] = "f1_index.bin";
    
    bool use_lemmatization = true;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--no-lemma") == 0) {
            use_lemmatization = false;
        }
    }
    
    struct stat path_stat;
    stat(input_path, &path_stat);
    
    printf("\n");
    printf("=========================================\n");
    printf("  ИНДЕКСАТОР КОРПУСА ФОРМУЛЫ-1 v2.1\n");
    printf("=========================================\n");
    printf("Лемматизация: %s\n", use_lemmatization ? "ВКЛ" : "ВЫКЛ");
    printf("=========================================\n\n");
    
    Timer total_timer;
    
    Tokenizer tokenizer;
    Lemmatizer lemmatizer(use_lemmatization); 
    InvertedIndex index;
    
    int total_docs = 0;
    long long total_bytes = 0;
    
    if (S_ISDIR(path_stat.st_mode)) {
        printf("Режим: Обработка директории\n");
        printf("Директория: %s\n", input_path);
        
        const char* dir_name = strrchr(input_path, '/');
        if (!dir_name) dir_name = input_path;
        else dir_name++;
        
        snprintf(output_file, sizeof(output_file), "%s_index.bin", dir_name);
        
        printf("Выходной файл: %s\n\n", output_file);
        
        DIR* dir = opendir(input_path);
        if (!dir) {
            printf("ОШИБКА: Не удалось открыть директорию %s\n", input_path);
            return 1;
        }
        
        DynamicArray<char*> batch_files;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (is_batch_file(entry->d_name)) {
                char* filepath = (char*)malloc(1024);
                snprintf(filepath, 1024, "%s/%s", input_path, entry->d_name);
                batch_files.push(filepath);
            }
        }
        
        closedir(dir);
        
        printf("Найдено батчей: %d\n\n", batch_files.length());
        
        if (batch_files.length() == 0) {
            printf("ОШИБКА: В директории не найдено батч-файлов (batch_*.jsonl)\n");
            return 1;
        }
        
        for (int i = 0; i < batch_files.length() - 1; i++) {
            for (int j = i + 1; j < batch_files.length(); j++) {
                if (strcmp(batch_files[i], batch_files[j]) > 0) {
                    char* temp = batch_files[i];
                    batch_files[i] = batch_files[j];
                    batch_files[j] = temp;
                }
            }
        }
        
        printf("Начало индексации...\n\n");
        
        for (int i = 0; i < batch_files.length(); i++) {
            printf("[Батч %d/%d]\n", i + 1, batch_files.length());
            
            struct stat file_stat;
            stat(batch_files[i], &file_stat);
            total_bytes += file_stat.st_size;
            
            int docs = process_batch_file(batch_files[i], tokenizer, lemmatizer, index);
            total_docs += docs;
            
            printf("  Документов в батче: %d\n", docs);
            printf("  Всего документов: %d\n\n", total_docs);
            
            free(batch_files[i]);
        }
        
    } else {
        printf("Режим: Обработка одного файла\n");
        printf("Входной файл: %s\n", input_path);
        
        if (argc >= 3 && strcmp(argv[2], "--no-lemma") != 0) {
            strncpy(output_file, argv[2], sizeof(output_file) - 1);
        }
        
        printf("Выходной файл: %s\n\n", output_file);
        
        struct stat file_stat;
        stat(input_path, &file_stat);
        total_bytes = file_stat.st_size;
        
        printf("Начало индексации...\n\n");
        
        total_docs = process_batch_file(input_path, tokenizer, lemmatizer, index);
        
        printf("\nОбработано документов: %d\n\n", total_docs);
    }
    
    printf("Сохранение индекса...\n");
    index.save_to_file(output_file);
    
    printf("\n");
    tokenizer.print_stats();
    index.print_stats();
    
    Statistics stats(&index);
    stats.compute_zipf_law("zipf_data.txt");
    stats.print_corpus_stats(&tokenizer);
    
    double total_time = total_timer.elapsed();
    
    printf("Общее время: %.4f сек (%.2f мин)\n", total_time, total_time / 60.0);
    printf("Обработано документов: %d\n", total_docs);
    printf("Обработано байт: %lld (%.2f MB)\n", total_bytes, total_bytes / 1024.0 / 1024.0);
    if (total_time > 0) {
        printf("Скорость индексации:\n");
        printf("  %.2f документов/сек\n", total_docs / total_time);
        printf("  %.2f MB/сек\n", (total_bytes / 1024.0 / 1024.0) / total_time);
        printf("  %.4f сек на документ\n", total_time / total_docs);
        printf("  %.4f сек на 1 KB текста\n", total_time / (total_bytes / 1024.0));
    }
    printf("Индексация завершена\n\n");
    
    return 0;
}
