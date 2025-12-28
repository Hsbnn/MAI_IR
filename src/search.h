#ifndef SEARCH_H
#define SEARCH_H

#include "utils.h"
#include "index.h"
#include "parser.h"

class SearchEngine {
private:
    InvertedIndex* index;
    
    DynamicArray<int>* intersect(DynamicArray<int>* a, DynamicArray<int>* b) {
        DynamicArray<int>* result = new DynamicArray<int>();
        
        int i = 0, j = 0;
        
        while (i < a->length() && j < b->length()) {
            if ((*a)[i] == (*b)[j]) {
                result->push((*a)[i]);
                i++;
                j++;
            } else if ((*a)[i] < (*b)[j]) {
                i++;
            } else {
                j++;
            }
        }
        
        return result;
    }
    
    DynamicArray<int>* union_sets(DynamicArray<int>* a, DynamicArray<int>* b) {
        DynamicArray<int>* result = new DynamicArray<int>();
        
        int i = 0, j = 0;
        
        while (i < a->length() && j < b->length()) {
            if ((*a)[i] == (*b)[j]) {
                result->push((*a)[i]);
                i++;
                j++;
            } else if ((*a)[i] < (*b)[j]) {
                result->push((*a)[i]);
                i++;
            } else {
                result->push((*b)[j]);
                j++;
            }
        }
        
        while (i < a->length()) {
            result->push((*a)[i]);
            i++;
        }
        
        while (j < b->length()) {
            result->push((*b)[j]);
            j++;
        }
        
        return result;
    }
    
    DynamicArray<int>* difference(DynamicArray<int>* a, DynamicArray<int>* b) {
        DynamicArray<int>* result = new DynamicArray<int>();
        
        int i = 0, j = 0;
        
        while (i < a->length()) {
            if (j >= b->length()) {
                result->push((*a)[i]);
                i++;
            } else if ((*a)[i] < (*b)[j]) {
                result->push((*a)[i]);
                i++;
            } else if ((*a)[i] == (*b)[j]) {
                i++;
                j++;
            } else {
                j++;
            }
        }
        
        return result;
    }
    
    DynamicArray<int>* search_phrase(DynamicArray<char*>& terms, int proximity) {
        if (terms.length() == 0) {
            return new DynamicArray<int>();
        }
        
        DynamicArray<Posting>* first_postings = index->search_term(terms[0]);
        if (!first_postings) {
            return new DynamicArray<int>();
        }
        
        DynamicArray<int>* result = new DynamicArray<int>();

        for (int i = 0; i < first_postings->length(); i++) {
            Posting& first_posting = (*first_postings)[i];
            int doc_id = first_posting.doc_id;
            
            DynamicArray<DynamicArray<int>*> all_positions;
            all_positions.push(&first_posting.positions);
            
            bool all_found = true;
            
            for (int t = 1; t < terms.length(); t++) {
                DynamicArray<Posting>* postings = index->search_term(terms[t]);
                if (!postings) {
                    all_found = false;
                    break;
                }
                
                bool found_in_doc = false;
                for (int p = 0; p < postings->length(); p++) {
                    if ((*postings)[p].doc_id == doc_id) {
                        all_positions.push(&(*postings)[p].positions);
                        found_in_doc = true;
                        break;
                    }
                }
                
                if (!found_in_doc) {
                    all_found = false;
                    break;
                }
            }
            
            if (!all_found) continue;
            
            bool phrase_found = false;
            
            for (int p0 = 0; p0 < all_positions[0]->length(); p0++) {
                int start_pos = (*all_positions[0])[p0];
                
                bool sequence_ok = true;
                int last_pos = start_pos;
                
                for (int t = 1; t < all_positions.length(); t++) {
                    bool next_found = false;
                    
                    for (int pt = 0; pt < all_positions[t]->length(); pt++) {
                        int pos = (*all_positions[t])[pt];
                        
                        if (proximity == 0) {
                            if (pos == last_pos + 1) {
                                last_pos = pos;
                                next_found = true;
                                break;
                            }
                        } else {
                            if (pos > last_pos && pos - start_pos <= proximity) {
                                last_pos = pos;
                                next_found = true;
                                break;
                            }
                        }
                    }
                    
                    if (!next_found) {
                        sequence_ok = false;
                        break;
                    }
                }
                
                if (sequence_ok) {
                    phrase_found = true;
                    break;
                }
            }
            
            if (phrase_found) {
                result->push(doc_id);
            }
        }
        
        return result;
    }
    
    DynamicArray<int>* execute_node(QueryNode* node) {
        if (!node) {
            return new DynamicArray<int>();
        }
        
        switch (node->type) {
            case NODE_TERM: {
                DynamicArray<Posting>* postings = index->search_term(node->term);
                DynamicArray<int>* result = new DynamicArray<int>();
                
                if (postings) {
                    for (int i = 0; i < postings->length(); i++) {
                        result->push((*postings)[i].doc_id);
                    }
                }
                
                return result;
            }
            
            case NODE_AND: {
                DynamicArray<int>* left = execute_node(node->left);
                DynamicArray<int>* right = execute_node(node->right);
                DynamicArray<int>* result = intersect(left, right);
                delete left;
                delete right;
                return result;
            }
            
            case NODE_OR: {
                DynamicArray<int>* left = execute_node(node->left);
                DynamicArray<int>* right = execute_node(node->right);
                DynamicArray<int>* result = union_sets(left, right);
                delete left;
                delete right;
                return result;
            }
            
            case NODE_NOT: {
                DynamicArray<int>* left = execute_node(node->left);
                
                DynamicArray<int>* all_docs = new DynamicArray<int>();
                for (int i = 0; i < index->get_num_documents(); i++) {
                    all_docs->push(i);
                }
                
                DynamicArray<int>* result = difference(all_docs, left);
                delete left;
                delete all_docs;
                return result;
            }
            
            case NODE_PHRASE: {
                return search_phrase(node->phrase_terms, node->proximity);
            }
            
            default:
                return new DynamicArray<int>();
        }
    }
    
public:
    SearchEngine(InvertedIndex* idx) : index(idx) {}
    
    InvertedIndex* get_index() {
        return index;
    }
    
    DynamicArray<int>* search(const char* query_str) {
        Timer timer;
        
        QueryParser parser;
        QueryNode* tree = parser.parse(query_str);
        
        if (!tree) {
            printf("Ошибка парсинга запроса\n");
            return new DynamicArray<int>();
        }
        
        DynamicArray<int>* results = execute_node(tree);
        
        delete tree;
        
        double elapsed = timer.elapsed();
        printf("Поиск выполнен за %.6f сек, найдено %d документов\n", 
               elapsed, results->length());
        
        return results;
    }
    
    void print_results(DynamicArray<int>* doc_ids, int offset = 0, int limit = 50) {
        printf("\nРезультаты поиска\n");
        printf("Всего найдено: %d документов\n\n", doc_ids->length());
        
        int end = offset + limit;
        if (end > doc_ids->length()) end = doc_ids->length();
        
        for (int i = offset; i < end; i++) {
            int doc_id = (*doc_ids)[i];
            Document* doc = index->get_document(doc_id);
            
            if (doc) {
                printf("[%d] %s\n", i + 1, doc->title);
                printf("    URL: %s\n", doc->url);
                printf("    PageID: %d\n\n", doc->pageid);
            }
        }
        
        if (end < doc_ids->length()) {
            printf("... и еще %d документов\n", doc_ids->length() - end);
        }
        
    }
};

#endif // SEARCH_H
