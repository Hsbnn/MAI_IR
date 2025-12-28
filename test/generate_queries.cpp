#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

const char* f1_terms[] = {
    "ferrari", "mercedes", "mclaren", "red bull", "williams",
    "racing", "championship", "formula one", "grand prix",
    "driver", "team", "circuit", "monaco", "silverstone",
    "engine", "car", "race", "winner", "fastest", "podium"
};

const int num_terms = sizeof(f1_terms) / sizeof(f1_terms[0]);

void generate_simple_queries(FILE* f, int count) {
    for (int i = 0; i < count; i++) {
        int idx = rand() % num_terms;
        fprintf(f, "%s\n", f1_terms[idx]);
    }
}

void generate_and_queries(FILE* f, int count) {
    for (int i = 0; i < count; i++) {
        int idx1 = rand() % num_terms;
        int idx2 = rand() % num_terms;
        
        if (idx1 != idx2) {
            fprintf(f, "%s && %s\n", f1_terms[idx1], f1_terms[idx2]);
        }
    }
}

void generate_or_queries(FILE* f, int count) {
    for (int i = 0; i < count; i++) {
        int idx1 = rand() % num_terms;
        int idx2 = rand() % num_terms;
        
        fprintf(f, "%s || %s\n", f1_terms[idx1], f1_terms[idx2]);
    }
}

void generate_complex_queries(FILE* f, int count) {
    for (int i = 0; i < count; i++) {
        int idx1 = rand() % num_terms;
        int idx2 = rand() % num_terms;
        int idx3 = rand() % num_terms;
        
        int type = rand() % 4;
        
        switch (type) {
            case 0:
                fprintf(f, "(%s || %s) && %s\n", f1_terms[idx1], f1_terms[idx2], f1_terms[idx3]);
                break;
            case 1:
                fprintf(f, "%s && %s && !%s\n", f1_terms[idx1], f1_terms[idx2], f1_terms[idx3]);
                break;
            case 2:
                fprintf(f, "(%s || %s) && !%s\n", f1_terms[idx1], f1_terms[idx2], f1_terms[idx3]);
                break;
            case 3:
                fprintf(f, "%s && (%s || %s)\n", f1_terms[idx1], f1_terms[idx2], f1_terms[idx3]);
                break;
        }
    }
}

void generate_phrase_queries(FILE* f, int count) {
    const char* phrases[] = {
        "\"formula one\"",
        "\"grand prix\"",
        "\"world championship\"",
        "\"red bull racing\"",
        "\"formula one\" / 3",
        "\"grand prix\" / 5"
    };
    
    int num_phrases = sizeof(phrases) / sizeof(phrases[0]);
    
    for (int i = 0; i < count; i++) {
        int idx = rand() % num_phrases;
        fprintf(f, "%s\n", phrases[idx]);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <output_file> [num_queries]\n", argv[0]);
        printf("Generates test queries for search engine\n");
        return 1;
    }
    
    const char* output_file = argv[1];
    int total_queries = argc >= 3 ? atoi(argv[2]) : 100;
    
    srand(time(NULL));
    
    FILE* f = fopen(output_file, "w");
    if (!f) {
        printf("Error opening file: %s\n", output_file);
        return 1;
    }
    
    printf("Generating %d test queries...\n", total_queries);
    
    int per_type = total_queries / 5;
    
    printf("  Simple queries: %d\n", per_type);
    generate_simple_queries(f, per_type);
    
    printf("  AND queries: %d\n", per_type);
    generate_and_queries(f, per_type);
    
    printf("  OR queries: %d\n", per_type);
    generate_or_queries(f, per_type);
    
    printf("  Complex queries: %d\n", per_type);
    generate_complex_queries(f, per_type);
    
    printf("  Phrase queries: %d\n", per_type);
    generate_phrase_queries(f, per_type);
    
    fclose(f);
    
    printf("\n✓ Queries saved to: %s\n", output_file);
    
    return 0;
}
