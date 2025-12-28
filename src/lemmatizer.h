#ifndef LEMMATIZER_H
#define LEMMATIZER_H

#include "utils.h"

class Stemmer {
private:
    static const char* ru_endings[];
    static const int ru_endings_count;
    static const char* en_endings[];
    static const int en_endings_count;
    
    bool ends_with(const char* word, const char* suffix) {
        int word_len = strlen(word);
        int suffix_len = strlen(suffix);
        
        if (word_len < suffix_len) return false;
        
        return strcmp(word + word_len - suffix_len, suffix) == 0;
    }
    
    void remove_suffix(char* word, int suffix_len) {
        int len = strlen(word);
        if (len > suffix_len) {
            word[len - suffix_len] = '\0';
        }
    }
    
public:
    void stem_russian(char* word) {
        int len = strlen(word);
        if (len < 4) return; 
        
        for (int i = 0; i < ru_endings_count; i++) {
            if (ends_with(word, ru_endings[i])) {
                int suffix_len = strlen(ru_endings[i]);
                if (len - suffix_len >= 3) {
                    remove_suffix(word, suffix_len);
                    return;
                }
            }
        }
    }

    void stem_english(char* word) {
        int len = strlen(word);
        if (len < 4) return;
        
        if (ends_with(word, "sses")) {
            remove_suffix(word, 2); 
        } else if (ends_with(word, "ies")) {
            remove_suffix(word, 3); 
        } else if (ends_with(word, "s") && len > 4) {
            remove_suffix(word, 1);
        }
        
        len = strlen(word);
        if (len < 4) return;
        
        if (ends_with(word, "ed")) {
            if (len - 2 >= 3) {
                remove_suffix(word, 2);
            }
        } else if (ends_with(word, "ing")) {
            if (len - 3 >= 3) {
                remove_suffix(word, 3);
            }
        }
        
        len = strlen(word);
        if (len < 4) return;
        
        for (int i = 0; i < en_endings_count; i++) {
            if (ends_with(word, en_endings[i])) {
                int suffix_len = strlen(en_endings[i]);
                if (len - suffix_len >= 3) {
                    remove_suffix(word, suffix_len);
                    return;
                }
            }
        }
    }
    
    void stem(char* word) {
        if (!word || strlen(word) < 3) return;
        
        bool is_cyrillic = false;
        for (int i = 0; word[i]; i++) {
            if ((unsigned char)word[i] >= 0xC0) {
                is_cyrillic = true;
                break;
            }
        }
        
        if (is_cyrillic) {
            stem_russian(word);
        } else {
            stem_english(word);
        }
    }
};

const char* Stemmer::ru_endings[] = {
    "ами", "ями", "ости", "ость",
    "ами", "ями", "ов", "ев", "ах", "ях", "ом", "ем", "ам", "ям",
    "ого", "его",
    "ов", "ам", "ом", "ах", "ми", "ей", "ий", "ый", "ой",
    "ем", "им", "ым", "их", "ых", "ую", "юю", "ая", "яя",
    "ое", "ее", "ие", "ые", "ых", "ая",
    "а", "я", "у", "ю", "о", "е", "и", "ы", "й"
};

const int Stemmer::ru_endings_count = sizeof(Stemmer::ru_endings) / sizeof(char*);

const char* Stemmer::en_endings[] = {
    "ational", "tional", "ization", "ation", "ator", "alism", "iveness",
    "fulness", "ousness", "aliti", "iviti", "biliti", "icate", "ative",
    "alize", "iciti", "ical", "ful", "ness", "ment", "ence", "ance",
    "able", "ible", "ant", "ent", "ion", "ism", "ate", "iti", "ous",
    "ive", "ize", "al", "er", "ic", "ly"
};

const int Stemmer::en_endings_count = sizeof(Stemmer::en_endings) / sizeof(char*);

class Lemmatizer {
private:
    Stemmer stemmer;
    bool enabled;
    
public:
    Lemmatizer(bool enable = true) : enabled(enable) {}
    
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    bool is_enabled() const { return enabled; }
    
    void lemmatize(Token& token) {
        if (!enabled || !token.text) return;
        
        stemmer.stem(token.text);
    }

    void lemmatize_all(DynamicArray<Token>* tokens) {
        if (!enabled || !tokens) return;
        
        for (int i = 0; i < tokens->length(); i++) {
            lemmatize((*tokens)[i]);
        }
    }
    
    char* lemmatize_string(const char* text) {
        char* result = (char*)malloc(strlen(text) + 1);
        strcpy(result, text);
        
        if (enabled) {
            stemmer.stem(result);
        }
        
        return result;
    }
};

#endif // LEMMATIZER_H
