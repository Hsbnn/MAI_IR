#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <new>

class Timer {
private:
    clock_t start_time;
    
public:
    Timer() : start_time(clock()) {}
    
    void reset() {
        start_time = clock();
    }
    
    double elapsed() const {
        return (double)(clock() - start_time) / CLOCKS_PER_SEC;
    }
    
    void print(const char* label) const {
        printf("[TIMER] %s: %.4f seconds\n", label, elapsed());
    }
};

template<typename T>
class DynamicArray {
private:
    T* data;
    int capacity;
    int size;
    
    void resize() {
        capacity = capacity == 0 ? 8 : capacity * 2;
        T* new_data = (T*)malloc(capacity * sizeof(T));
        
        if (data) {
            for (int i = 0; i < size; i++) {
                new (&new_data[i]) T(data[i]); 
                data[i].~T();
            }
            free(data);
        }
        
        data = new_data;
    }
    
public:
    DynamicArray() : data(nullptr), capacity(0), size(0) {}
    
    ~DynamicArray() {
        if (data) {
            for (int i = 0; i < size; i++) {
                data[i].~T();
            }
            free(data);
        }
    }
    
    void push(const T& item) {
        if (size >= capacity) resize();
        new (&data[size]) T(item); 
        size++;
    }
    
    T& operator[](int index) {
        return data[index];
    }
    
    const T& operator[](int index) const {
        return data[index];
    }
    
    int length() const { return size; }
    
    T* begin() { return data; }
    T* end() { return data + size; }
    
    void clear() {
        for (int i = 0; i < size; i++) {
            data[i].~T();
        }
        size = 0;
    }
    
    void sort(int (*compare)(const T&, const T&)) {
        quicksort(data, 0, size - 1, compare);
    }
    
private:
    void quicksort(T* arr, int low, int high, int (*cmp)(const T&, const T&)) {
        if (low < high) {
            int pi = partition(arr, low, high, cmp);
            quicksort(arr, low, pi - 1, cmp);
            quicksort(arr, pi + 1, high, cmp);
        }
    }
    
    int partition(T* arr, int low, int high, int (*cmp)(const T&, const T&)) {
        T pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (cmp(arr[j], pivot) < 0) {
                i++;
                T temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        
        T temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;
        
        return i + 1;
    }
};

class String {
private:
    char* data;
    int len;
    
public:
    String() : data(nullptr), len(0) {}
    
    String(const char* str) {
        len = strlen(str);
        data = (char*)malloc(len + 1);
        strcpy(data, str);
    }
    
    String(const String& other) {
        len = other.len;
        data = (char*)malloc(len + 1);
        strcpy(data, other.data);
    }
    
    ~String() {
        if (data) free(data);
    }
    
    String& operator=(const String& other) {
        if (this != &other) {
            if (data) free(data);
            len = other.len;
            data = (char*)malloc(len + 1);
            strcpy(data, other.data);
        }
        return *this;
    }
    
    const char* c_str() const { return data ? data : ""; }
    int length() const { return len; }
    
    bool equals(const char* str) const {
        return strcmp(data, str) == 0;
    }
    
    void toLower() {
        for (int i = 0; i < len; i++) {
            if (data[i] >= 'A' && data[i] <= 'Z') {
                data[i] = data[i] + 32;
            } else if ((unsigned char)data[i] >= 0xC0 && (unsigned char)data[i] <= 0xDF) {
                if (i + 1 < len) {
                    unsigned char c1 = data[i];
                    unsigned char c2 = data[i + 1];
                    if (c1 == 0xD0 && c2 >= 0x90 && c2 <= 0xAF) {
                        data[i + 1] = c2 + 0x20;
                    } else if (c1 == 0xD0 && c2 >= 0x81 && c2 <= 0x8F) {
                        data[i] = 0xD1;
                        data[i + 1] = c2 + 0x10;
                    }
                }
            }
        }
    }
};

template<typename V>
class HashMap {
private:
    struct Node {
        char* key;
        V value;
        Node* next;
        
        Node(const char* k, const V& v) : value(v), next(nullptr) {
            key = (char*)malloc(strlen(k) + 1);
            strcpy(key, k);
        }
        
        ~Node() {
            free(key);
        }
    };
    
    Node** buckets;
    int capacity;
    int size;
    
    unsigned int hash(const char* key) const {
        unsigned int h = 0;
        while (*key) {
            h = h * 31 + *key++;
        }
        return h % capacity;
    }
    
public:
    HashMap(int cap = 1024) : capacity(cap), size(0) {
        buckets = (Node**)calloc(capacity, sizeof(Node*));
    }
    
    ~HashMap() {
        for (int i = 0; i < capacity; i++) {
            Node* node = buckets[i];
            while (node) {
                Node* next = node->next;
                delete node;
                node = next;
            }
        }
        free(buckets);
    }
    
    void put(const char* key, const V& value) {
        unsigned int idx = hash(key);
        Node* node = buckets[idx];
        
        while (node) {
            if (strcmp(node->key, key) == 0) {
                node->value = value;
                return;
            }
            node = node->next;
        }
        
        Node* new_node = new Node(key, value);
        new_node->next = buckets[idx];
        buckets[idx] = new_node;
        size++;
    }
    
    bool get(const char* key, V& value) const {
        unsigned int idx = hash(key);
        Node* node = buckets[idx];
        
        while (node) {
            if (strcmp(node->key, key) == 0) {
                value = node->value;
                return true;
            }
            node = node->next;
        }
        
        return false;
    }
    
    bool contains(const char* key) const {
        V dummy;
        return get(key, dummy);
    }
    
    int getSize() const { return size; }
    
    class Iterator {
    private:
        HashMap* map;
        int bucket_idx;
        Node* current;
        
        void advance() {
            if (current && current->next) {
                current = current->next;
                return;
            }
            
            current = nullptr;
            bucket_idx++;
            
            while (bucket_idx < map->capacity) {
                if (map->buckets[bucket_idx]) {
                    current = map->buckets[bucket_idx];
                    return;
                }
                bucket_idx++;
            }
        }
        
    public:
        Iterator(HashMap* m, int idx, Node* n) : map(m), bucket_idx(idx), current(n) {}
        
        bool hasNext() const { return current != nullptr; }
        
        void next() { advance(); }
        
        const char* key() const { return current ? current->key : nullptr; }
        V& value() { return current->value; }
    };
    
    Iterator iterator() {
        for (int i = 0; i < capacity; i++) {
            if (buckets[i]) {
                return Iterator(this, i, buckets[i]);
            }
        }
        return Iterator(this, capacity, nullptr);
    }
};

class BitArray {
private:
    unsigned char* data;
    int num_bits;
    
public:
    BitArray(int n) : num_bits(n) {
        int bytes = (n + 7) / 8;
        data = (unsigned char*)calloc(bytes, 1);
    }
    
    ~BitArray() {
        free(data);
    }
    
    void set(int pos) {
        data[pos / 8] |= (1 << (pos % 8));
    }
    
    bool get(int pos) const {
        return (data[pos / 8] & (1 << (pos % 8))) != 0;
    }
    
    void clear(int pos) {
        data[pos / 8] &= ~(1 << (pos % 8));
    }
    
    int size() const {
        return num_bits;
    }
};

char* extract_json_string(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char* pos = strstr(json, search);
    if (!pos) return nullptr;
    
    pos += strlen(search);
    while (*pos == ' ' || *pos == '\t') pos++;
    
    if (*pos == '"') {
        pos++;
        const char* end = strchr(pos, '"');
        if (!end) return nullptr;
        
        int len = end - pos;
        char* result = (char*)malloc(len + 1);
        strncpy(result, pos, len);
        result[len] = '\0';
        return result;
    } else {
        const char* end = pos;
        while (*end && *end != ',' && *end != '}' && *end != ']') end++;
        
        int len = end - pos;
        char* result = (char*)malloc(len + 1);
        strncpy(result, pos, len);
        result[len] = '\0';
        return result;
    }
}

#endif // UTILS_H
