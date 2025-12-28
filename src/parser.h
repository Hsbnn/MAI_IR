#ifndef PARSER_H
#define PARSER_H

#include "utils.h"

enum NodeType {
    NODE_TERM,  
    NODE_AND, 
    NODE_OR,  
    NODE_NOT,  
    NODE_PHRASE  
};

struct QueryNode {
    NodeType type;
    char* term; 
    DynamicArray<char*> phrase_terms; 
    int proximity;    
    QueryNode* left;
    QueryNode* right;
    
    QueryNode() : type(NODE_TERM), term(nullptr), proximity(0), 
                  left(nullptr), right(nullptr) {}
    
    ~QueryNode() {
        if (term) free(term);
        for (int i = 0; i < phrase_terms.length(); i++) {
            free(phrase_terms[i]);
        }
        if (left) delete left;
        if (right) delete right;
    }
};

class QueryParser {
private:
    const char* input;
    int pos;
    
    char current() {
        return input[pos];
    }
    
    void skip_spaces() {
        while (input[pos] == ' ' || input[pos] == '\t' || input[pos] == '\n') {
            pos++;
        }
    }
    
    bool match(const char* str) {
        int len = strlen(str);
        if (strncmp(input + pos, str, len) == 0) {
            pos += len;
            return true;
        }
        return false;
    }
    
    char* read_word() {
        skip_spaces();
        
        char buffer[256];
        int buf_pos = 0;
        
        while (current() && current() != ' ' && current() != ')' && 
               current() != '(' && current() != '&' && current() != '|' && 
               current() != '!' && current() != '"' && current() != '/') {
            if (buf_pos < 255) {
                buffer[buf_pos++] = current();
            }
            pos++;
        }
        
        buffer[buf_pos] = '\0';
        
        if (buf_pos == 0) return nullptr;
        
        char* result = (char*)malloc(buf_pos + 1);
        strcpy(result, buffer);
        
        for (int i = 0; i < buf_pos; i++) {
            if (result[i] >= 'A' && result[i] <= 'Z') {
                result[i] = result[i] + 32;
            }
        }
        
        return result;
    }
    
    QueryNode* parse_phrase() {
        skip_spaces();
        
        if (current() != '"') return nullptr;
        pos++; 
        
        QueryNode* node = new QueryNode();
        node->type = NODE_PHRASE;
        node->proximity = 0; 
        
        while (current() && current() != '"') {
            char* word = read_word();
            if (word) {
                node->phrase_terms.push(word);
            }
            skip_spaces();
        }
        
        if (current() == '"') {
            pos++;
        }
        
        skip_spaces();
        
        if (current() == '/') {
            pos++;
            skip_spaces();
            
            char num_buf[16];
            int num_pos = 0;
            
            while (current() >= '0' && current() <= '9') {
                if (num_pos < 15) {
                    num_buf[num_pos++] = current();
                }
                pos++;
            }
            
            num_buf[num_pos] = '\0';
            
            if (num_pos > 0) {
                node->proximity = atoi(num_buf);
            }
        }
        
        return node;
    }
    
    QueryNode* parse_primary() {
        skip_spaces();
        
        if (current() == '(') {
            pos++;
            QueryNode* node = parse_or();
            skip_spaces();
            if (current() == ')') pos++;
            return node;
        }
        
        if (current() == '!') {
            pos++;
            QueryNode* node = new QueryNode();
            node->type = NODE_NOT;
            node->left = parse_primary();
            return node;
        }
        
        if (current() == '"') {
            return parse_phrase();
        }
        
        char* word = read_word();
        if (!word) return nullptr;
        
        QueryNode* node = new QueryNode();
        node->type = NODE_TERM;
        node->term = word;
        
        return node;
    }
    
    QueryNode* parse_and() {
        QueryNode* left = parse_primary();
        if (!left) return nullptr;
        
        while (true) {
            skip_spaces();
            
            bool is_and = false;
            
            if (match("&&")) {
                is_and = true;
            } else if (current() != '|' && current() != ')' && current() != '\0' && current() != '!') {
                is_and = true;
            }
            
            if (!is_and) break;
            
            QueryNode* right = parse_primary();
            if (!right) break;
            
            QueryNode* node = new QueryNode();
            node->type = NODE_AND;
            node->left = left;
            node->right = right;
            
            left = node;
        }
        
        return left;
    }
    
    QueryNode* parse_or() {
        QueryNode* left = parse_and();
        if (!left) return nullptr;
        
        while (true) {
            skip_spaces();
            
            if (!match("||")) break;
            
            QueryNode* right = parse_and();
            if (!right) break;
            
            QueryNode* node = new QueryNode();
            node->type = NODE_OR;
            node->left = left;
            node->right = right;
            
            left = node;
        }
        
        return left;
    }
    
public:
    QueryNode* parse(const char* query) {
        input = query;
        pos = 0;
        return parse_or();
    }
    
    void print_tree(QueryNode* node, int depth = 0) {
        if (!node) return;
        
        for (int i = 0; i < depth; i++) printf("  ");
        
        switch (node->type) {
            case NODE_TERM:
                printf("TERM: %s\n", node->term);
                break;
            case NODE_AND:
                printf("AND\n");
                print_tree(node->left, depth + 1);
                print_tree(node->right, depth + 1);
                break;
            case NODE_OR:
                printf("OR\n");
                print_tree(node->left, depth + 1);
                print_tree(node->right, depth + 1);
                break;
            case NODE_NOT:
                printf("NOT\n");
                print_tree(node->left, depth + 1);
                break;
            case NODE_PHRASE:
                printf("PHRASE (proximity=%d): ", node->proximity);
                for (int i = 0; i < node->phrase_terms.length(); i++) {
                    printf("%s ", node->phrase_terms[i]);
                }
                printf("\n");
                break;
        }
    }
};

#endif // PARSER_H
