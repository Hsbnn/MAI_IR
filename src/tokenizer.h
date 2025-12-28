#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "utils.h"

struct Token {
    char* text;
    int position; 
    
    Token() : text(nullptr), position(0) {}
    
    Token(const char* t, int pos) : position(pos) {
        int len = strlen(t);
        text = (char*)malloc(len + 10); 
        strcpy(text, t);
    }
    
    ~Token() {
        if (text) free(text);
    }
    
    Token(const Token& other) : position(other.position) {
        text = (char*)malloc(strlen(other.text) + 1);
        strcpy(text, other.text);
    }
    
    Token& operator=(const Token& other) {
        if (this != &other) {
            if (text) free(text);
            text = (char*)malloc(strlen(other.text) + 1);
            strcpy(text, other.text);
            position = other.position;
        }
        return *this;
    }
};

class Tokenizer {
private:
    long long total_tokens;
    long long total_chars;
    double tokenization_time;
    
    bool is_letter(char c) {
        return (c >= 'a' && c <= 'z') || 
               (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') ||
               (unsigned char)c >= 0x80;  
    }
    
    void to_lower(char* str) {
        for (int i = 0; str[i]; i++) {
            if (str[i] >= 'A' && str[i] <= 'Z') {
                str[i] = str[i] + 32;
            }
        }
    }
    
public:
    Tokenizer() : total_tokens(0), total_chars(0), tokenization_time(0) {}
    
    DynamicArray<Token>* tokenize(const char* text) {
        Timer timer;
        
        DynamicArray<Token>* tokens = new DynamicArray<Token>();
        
        char buffer[256];
        int buf_pos = 0;
        int position = 0;
        
        for (int i = 0; text[i]; i++) {
            if (is_letter(text[i])) {
                if (buf_pos < 255) {
                    buffer[buf_pos++] = text[i];
                }
            } else {
                if (buf_pos > 0) {
                    buffer[buf_pos] = '\0';
                    to_lower(buffer);
                    
                    Token token(buffer, position);
                    tokens->push(token);
                    
                    total_tokens++;
                    total_chars += buf_pos;
                    
                    buf_pos = 0;
                    position++;
                }
            }
        }
        
        if (buf_pos > 0) {
            buffer[buf_pos] = '\0';
            to_lower(buffer);
            
            Token token(buffer, position);
            tokens->push(token);
            
            total_tokens++;
            total_chars += buf_pos;
        }
        
        tokenization_time += timer.elapsed();
        
        return tokens;
    }
    
    void print_stats() const {
        printf("\nСтатистика токенизации\n");
        printf("Всего токенов: %lld\n", total_tokens);
        if (total_tokens > 0) {
            printf("Средняя длина токена: %.2f символов\n", 
                   (double)total_chars / total_tokens);
        }
        printf("Время токенизации: %.4f сек\n", tokenization_time);
        if (tokenization_time > 0) {
            printf("Скорость: %.0f токенов/сек\n", total_tokens / tokenization_time);
        }
    }
};

#endif // TOKENIZER_H
