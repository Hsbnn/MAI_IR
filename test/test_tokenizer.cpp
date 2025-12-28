#include <cstdio>
#include <cstring>
#include <cassert>
#include "../src/utils.h"
#include "../src/tokenizer.h"
#include "../src/lemmatizer.h"

void test_basic_tokenization() {
    printf("TEST: Basic tokenization\n");
    
    Tokenizer tokenizer;
    const char* text = "Ferrari is the best Formula-1 team!";
    
    DynamicArray<Token>* tokens = tokenizer.tokenize(text);
    
    assert(tokens->length() == 6);
    assert(strcmp((*tokens)[0].text, "ferrari") == 0);
    assert(strcmp((*tokens)[1].text, "is") == 0);
    assert(strcmp((*tokens)[2].text, "the") == 0);
    assert(strcmp((*tokens)[3].text, "best") == 0);
    
    delete tokens;
    
    printf("  ✓ PASSED\n\n");
}

void test_russian_tokenization() {
    printf("TEST: Russian tokenization\n");
    
    Tokenizer tokenizer;
    const char* text = "Формула-1 это лучший спорт!";
    
    DynamicArray<Token>* tokens = tokenizer.tokenize(text);
    
    assert(tokens->length() > 0);
    
    delete tokens;
    
    printf("  ✓ PASSED\n\n");
}

void test_lemmatization() {
    printf("TEST: Lemmatization\n");
    
    Lemmatizer lemmatizer(true);
    
    char word1[] = "racing";
    lemmatizer.lemmatize_string(word1);
    printf("  racing -> %s\n", word1);
    
    char word2[] = "championships";
    lemmatizer.lemmatize_string(word2);
    printf("  championships -> %s\n", word2);
    
    char word3[] = "автомобилей";
    lemmatizer.lemmatize_string(word3);
    printf("  автомобилей -> %s\n", word3);
    
    printf("  ✓ PASSED\n\n");
}

void test_positions() {
    printf("TEST: Token positions\n");
    
    Tokenizer tokenizer;
    const char* text = "Ferrari won the race at Monaco";
    
    DynamicArray<Token>* tokens = tokenizer.tokenize(text);
    
    assert((*tokens)[0].position == 0);
    assert((*tokens)[1].position == 1);
    assert((*tokens)[2].position == 2);
    
    delete tokens;
    
    printf("  ✓ PASSED\n\n");
}

void test_performance() {
    printf("TEST: Tokenization performance\n");
    
    Tokenizer tokenizer;
    Timer timer;
    
    const int TEXT_SIZE = 100000;
    char* big_text = (char*)malloc(TEXT_SIZE);
    
    strcpy(big_text, "Ferrari Mercedes Red Bull McLaren Williams Aston Martin Alpine ");
    int len = strlen(big_text);
    
    for (int i = 0; i < TEXT_SIZE / len - 1; i++) {
        strcat(big_text, "Ferrari Mercedes Red Bull McLaren ");
    }
    
    DynamicArray<Token>* tokens = tokenizer.tokenize(big_text);
    
    double elapsed = timer.elapsed();
    
    printf("  Текста: %d байт\n", (int)strlen(big_text));
    printf("  Токенов: %d\n", tokens->length());
    printf("  Время: %.4f сек\n", elapsed);
    printf("  Скорость: %.0f байт/сек\n", strlen(big_text) / elapsed);
    
    delete tokens;
    free(big_text);
    
    printf("  ✓ PASSED\n\n");
}

int main() {
    printf("  Тестирование токенизатора\n");
    
    test_basic_tokenization();
    test_russian_tokenization();
    test_lemmatization();
    test_positions();
    test_performance();
    printf("  Тесты пройдены\n");
    
    return 0;
}
