#ifndef INDEX_H
#define INDEX_H

#include "utils.h"
#include "tokenizer.h"

struct TermFrequency {
    char* term;
    int frequency;
    
    TermFrequency() : term(nullptr), frequency(0) {}
    
    TermFrequency(const char* t, int f) : frequency(f) {
        term = (char*)malloc(strlen(t) + 1);
        strcpy(term, t);
    }
    
    ~TermFrequency() {
        if (term) free(term);
    }
    
    TermFrequency(const TermFrequency& other) : frequency(other.frequency) {
        if (other.term) {
            term = (char*)malloc(strlen(other.term) + 1);
            strcpy(term, other.term);
        } else {
            term = nullptr;
        }
    }
    
    TermFrequency& operator=(const TermFrequency& other) {
        if (this != &other) {
            if (term) free(term);
            frequency = other.frequency;
            if (other.term) {
                term = (char*)malloc(strlen(other.term) + 1);
                strcpy(term, other.term);
            } else {
                term = nullptr;
            }
        }
        return *this;
    }
};

struct Posting {
    int doc_id;
    DynamicArray<int> positions;
    
    Posting() : doc_id(-1) {}
    Posting(int id) : doc_id(id) {}
    
    Posting(const Posting& other) : doc_id(other.doc_id) {
        for (int i = 0; i < other.positions.length(); i++) {
            positions.push(other.positions[i]);
        }
    }
    
    Posting& operator=(const Posting& other) {
        if (this != &other) {
            doc_id = other.doc_id;
            positions.clear();
            for (int i = 0; i < other.positions.length(); i++) {
                positions.push(other.positions[i]);
            }
        }
        return *this;
    }
};

struct Document {
    int doc_id;
    char* title;
    char* url;
    int pageid;
    
    Document() : doc_id(-1), title(nullptr), url(nullptr), pageid(0) {}
    
    Document(const Document& other) : doc_id(other.doc_id), pageid(other.pageid) {
        if (other.title) {
            title = (char*)malloc(strlen(other.title) + 1);
            strcpy(title, other.title);
        } else {
            title = nullptr;
        }
        
        if (other.url) {
            url = (char*)malloc(strlen(other.url) + 1);
            strcpy(url, other.url);
        } else {
            url = nullptr;
        }
    }
    
    Document& operator=(const Document& other) {
        if (this != &other) {
            if (title) free(title);
            if (url) free(url);
            
            doc_id = other.doc_id;
            pageid = other.pageid;
            
            if (other.title) {
                title = (char*)malloc(strlen(other.title) + 1);
                strcpy(title, other.title);
            } else {
                title = nullptr;
            }
            
            if (other.url) {
                url = (char*)malloc(strlen(other.url) + 1);
                strcpy(url, other.url);
            } else {
                url = nullptr;
            }
        }
        return *this;
    }
    
    ~Document() {
        if (title) free(title);
        if (url) free(url);
    }
};

class InvertedIndex {
private:
    HashMap<DynamicArray<Posting>*> index;
    DynamicArray<Document> documents;
    int next_doc_id;

    long long total_terms;
    long long total_postings;
    double indexing_time;
    
public:
    InvertedIndex() : next_doc_id(0), total_terms(0), total_postings(0), indexing_time(0) {}
    
    ~InvertedIndex() {
        auto it = index.iterator();
        while (it.hasNext()) {
            delete it.value();
            it.next();
        }
    }
    
    void collect_term_frequencies(DynamicArray<TermFrequency>& frequencies) {
        auto it = index.iterator();
        
        while (it.hasNext()) {
            const char* term = it.key();
            DynamicArray<Posting>* postings = it.value();
            
            int total_freq = 0;
            for (int i = 0; i < postings->length(); i++) {
                total_freq += (*postings)[i].positions.length();
            }
            
            TermFrequency tf(term, total_freq);
            frequencies.push(tf);
            
            it.next();
        }
    }

    int add_document(const char* title, const char* url, int pageid, 
                     DynamicArray<Token>* tokens) {
        Timer timer;
        
        int doc_id = next_doc_id++;
        Document doc;
        doc.doc_id = doc_id;
        doc.title = (char*)malloc(strlen(title) + 1);
        strcpy(doc.title, title);
        doc.url = (char*)malloc(strlen(url) + 1);
        strcpy(doc.url, url);
        doc.pageid = pageid;
        
        documents.push(doc);
        
        for (int i = 0; i < tokens->length(); i++) {
            Token& token = (*tokens)[i];
            
            DynamicArray<Posting>* postings = nullptr;
            if (!index.get(token.text, postings)) {
                postings = new DynamicArray<Posting>();
                index.put(token.text, postings);
                total_terms++;
            }
            
            bool found = false;
            for (int j = 0; j < postings->length(); j++) {
                if ((*postings)[j].doc_id == doc_id) {
                    (*postings)[j].positions.push(token.position);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                Posting posting(doc_id);
                posting.positions.push(token.position);
                postings->push(posting);
                total_postings++;
            }
        }
        
        indexing_time += timer.elapsed();
        
        return doc_id;
    }
    
    DynamicArray<Posting>* search_term(const char* term) {
        DynamicArray<Posting>* result = nullptr;
        index.get(term, result);
        return result;
    }
    
    Document* get_document(int doc_id) {
        if (doc_id >= 0 && doc_id < documents.length()) {
            return &documents[doc_id];
        }
        return nullptr;
    }
    
    void save_to_file(const char* filename) {
        Timer timer;
        
        FILE* f = fopen(filename, "wb");
        if (!f) {
            printf("Ошибка открытия файла для записи: %s\n", filename);
            return;
        }
        
        unsigned int magic = 0xF1F1F1F1;
        unsigned int version = 1;
        int num_docs = documents.length();
        int num_terms = index.getSize();
        
        fwrite(&magic, 4, 1, f);
        fwrite(&version, 4, 1, f);
        fwrite(&num_docs, 4, 1, f);
        fwrite(&num_terms, 4, 1, f);
        
        for (int i = 0; i < num_docs; i++) {
            Document& doc = documents[i];
            
            fwrite(&doc.doc_id, 4, 1, f);
            fwrite(&doc.pageid, 4, 1, f);
            
            int title_len = strlen(doc.title);
            fwrite(&title_len, 4, 1, f);
            fwrite(doc.title, 1, title_len, f);
            
            int url_len = strlen(doc.url);
            fwrite(&url_len, 4, 1, f);
            fwrite(doc.url, 1, url_len, f);
        }
        
        auto it = index.iterator();
        while (it.hasNext()) {
            const char* term = it.key();
            DynamicArray<Posting>* postings = it.value();
            
            int term_len = strlen(term);
            fwrite(&term_len, 4, 1, f);
            fwrite(term, 1, term_len, f);
            
            int num_postings = postings->length();
            fwrite(&num_postings, 4, 1, f);
            
            for (int i = 0; i < num_postings; i++) {
                Posting& p = (*postings)[i];
                
                fwrite(&p.doc_id, 4, 1, f);
                
                int num_positions = p.positions.length();
                fwrite(&num_positions, 4, 1, f);
                
                for (int j = 0; j < num_positions; j++) {
                    int pos = p.positions[j];
                    fwrite(&pos, 4, 1, f);
                }
            }
            
            it.next();
        }
        
        fclose(f);
        
        printf("Индекс сохранен в файл: %s\n", filename);
        printf("Время сохранения: %.4f сек\n", timer.elapsed());
    }
    
    bool load_from_file(const char* filename) {
        Timer timer;
        
        FILE* f = fopen(filename, "rb");
        if (!f) {
            printf("Ошибка открытия файла для чтения: %s\n", filename);
            return false;
        }
        
        unsigned int magic, version;
        int num_docs, num_terms;
        
        fread(&magic, 4, 1, f);
        if (magic != 0xF1F1F1F1) {
            printf("Неверный формат файла индекса\n");
            fclose(f);
            return false;
        }
        
        fread(&version, 4, 1, f);
        fread(&num_docs, 4, 1, f);
        fread(&num_terms, 4, 1, f);
        
        printf("Загрузка индекса: %d документов, %d термов\n", num_docs, num_terms);
        
        for (int i = 0; i < num_docs; i++) {
            Document doc;
            
            fread(&doc.doc_id, 4, 1, f);
            fread(&doc.pageid, 4, 1, f);
            
            int title_len;
            fread(&title_len, 4, 1, f);
            doc.title = (char*)malloc(title_len + 1);
            fread(doc.title, 1, title_len, f);
            doc.title[title_len] = '\0';
            
            int url_len;
            fread(&url_len, 4, 1, f);
            doc.url = (char*)malloc(url_len + 1);
            fread(doc.url, 1, url_len, f);
            doc.url[url_len] = '\0';
            
            documents.push(doc);
        }
        
        next_doc_id = num_docs;
        
        for (int i = 0; i < num_terms; i++) {
            int term_len;
            fread(&term_len, 4, 1, f);
            
            char* term = (char*)malloc(term_len + 1);
            fread(term, 1, term_len, f);
            term[term_len] = '\0';
            
            int num_postings;
            fread(&num_postings, 4, 1, f);
            
            DynamicArray<Posting>* postings = new DynamicArray<Posting>();
            
            for (int j = 0; j < num_postings; j++) {
                Posting p;
                
                fread(&p.doc_id, 4, 1, f);
                
                int num_positions;
                fread(&num_positions, 4, 1, f);
                
                for (int k = 0; k < num_positions; k++) {
                    int pos;
                    fread(&pos, 4, 1, f);
                    p.positions.push(pos);
                }
                
                postings->push(p);
            }
            
            index.put(term, postings);
            
            free(term);
        }
        
        fclose(f);
        
        total_terms = num_terms;
        total_postings = num_terms;
        
        printf("Индекс загружен за %.4f сек\n", timer.elapsed());
        
        return true;
    }
    
    void print_stats() const {
        printf("Документов: %d\n", documents.length());
        printf("Уникальных термов: %lld\n", total_terms);
        printf("Всего postings: %lld\n", total_postings);
        printf("Время индексации: %.4f сек\n", indexing_time);
        if (documents.length() > 0 && indexing_time > 0) {
            printf("Скорость: %.2f документов/сек\n", 
                   documents.length() / indexing_time);
        }
    }
    
    int get_num_documents() const {
        return documents.length();
    }
    
    int get_num_terms() const {
        return total_terms;
    }
};

#endif // INDEX_H