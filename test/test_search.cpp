#include <cstdio>
#include <cstring>
#include <cassert>
#include "../src/utils.h"
#include "../src/tokenizer.h"
#include "../src/lemmatizer.h"
#include "../src/index.h"
#include "../src/parser.h"
#include "../src/search.h"

void create_test_index(InvertedIndex& index) {
    Tokenizer tokenizer;
    Lemmatizer lemmatizer(false);
    
    const char* text1 = "Ferrari wins the race at Monaco Grand Prix";
    DynamicArray<Token>* tokens1 = tokenizer.tokenize(text1);
    index.add_document("Ferrari wins", "http://example.com/1", 1, tokens1);
    delete tokens1;
    
    const char* text2 = "Mercedes secures the Formula One championship";
    DynamicArray<Token>* tokens2 = tokenizer.tokenize(text2);
    index.add_document("Mercedes wins", "http://example.com/2", 2, tokens2);
    delete tokens2;
    
    const char* text3 = "Red Bull Racing and Ferrari compete for the title";
    DynamicArray<Token>* tokens3 = tokenizer.tokenize(text3);
    index.add_document("Red Bull vs Ferrari", "http://example.com/3", 3, tokens3);
    delete tokens3;
}

void test_simple_search() {
    printf("TEST: Simple search\n");
    
    InvertedIndex index;
    create_test_index(index);
    
    SearchEngine engine(&index);
    
    DynamicArray<int>* results = engine.search("ferrari");
    
    printf("  Query: ferrari\n");
    printf("  Results: %d documents\n", results->length());
    
    assert(results->length() == 2); 
    
    delete results;
    
    printf("  ✓ PASSED\n\n");
}

void test_and_search() {
    printf("TEST: AND search\n");
    
    InvertedIndex index;
    create_test_index(index);
    
    SearchEngine engine(&index);
    
    DynamicArray<int>* results = engine.search("ferrari && monaco");
    
    printf("  Query: ferrari && monaco\n");
    printf("  Results: %d documents\n", results->length());
    
    assert(results->length() == 1); 
    
    delete results;
    
    printf("  ✓ PASSED\n\n");
}

void test_or_search() {
    printf("TEST: OR search\n");
    
    InvertedIndex index;
    create_test_index(index);
    
    SearchEngine engine(&index);
    
    DynamicArray<int>* results = engine.search("ferrari || mercedes");
    
    printf("  Query: ferrari || mercedes\n");
    printf("  Results: %d documents\n", results->length());
    
    assert(results->length() == 3); 
    
    delete results;
    
    printf("  ✓ PASSED\n\n");
}

void test_not_search() {
    printf("TEST: NOT search\n");
    
    InvertedIndex index;
    create_test_index(index);
    
    SearchEngine engine(&index);
    
    DynamicArray<int>* results = engine.search("!ferrari");
    
    printf("  Query: !ferrari\n");
    printf("  Results: %d documents\n", results->length());
    
    assert(results->length() == 1); 
    
    delete results;
    
    printf("  ✓ PASSED\n\n");
}

void test_phrase_search() {
    printf("TEST: Phrase search\n");
    
    InvertedIndex index;
    create_test_index(index);
    
    SearchEngine engine(&index);
    
    DynamicArray<int>* results = engine.search("\"grand prix\"");
    
    printf("  Query: \"grand prix\"\n");
    printf("  Results: %d documents\n", results->length());
    
    assert(results->length() == 1); 
    
    delete results;
    
    printf("  ✓ PASSED\n\n");
}

void test_complex_query() {
    printf("TEST: Complex query\n");
    
    InvertedIndex index;
    create_test_index(index);
    
    SearchEngine engine(&index);
    
    DynamicArray<int>* results = engine.search("(ferrari || mercedes) && !monaco");
    
    printf("  Query: (ferrari || mercedes) && !monaco\n");
    printf("  Results: %d documents\n", results->length());
    
    assert(results->length() == 2); 
    
    delete results;
    
    printf("  ✓ PASSED\n\n");
}

int main() {
    printf("  Тестирование поиска\n");
    
    test_simple_search();
    test_and_search();
    test_or_search();
    test_not_search();
    test_phrase_search();
    test_complex_query();
    
    printf("  Тесты пройдены\n");
    
    return 0;
}
