#ifndef STATS_H
#define STATS_H

#include "utils.h"
#include "index.h" 
#include <cmath>

int compare_frequency(const TermFrequency& a, const TermFrequency& b) {
    return b.frequency - a.frequency; 
}

class Statistics {
private:
    InvertedIndex* index;
    
public:
    Statistics(InvertedIndex* idx) : index(idx) {}
    
    void compute_zipf_law(const char* output_file) {
        printf("Сбор статистики по термам...\n");
        
        Timer timer;
        
        DynamicArray<TermFrequency> frequencies;
        
        index->collect_term_frequencies(frequencies);
        
        if (frequencies.length() == 0) {
            printf("Нет данных для построения графика\n");
            return;
        }
        
        frequencies.sort(compare_frequency);
        
        printf("Термов собрано: %d\n", frequencies.length());
        printf("Самый частый терм: '%s' (%d вхождений)\n", 
               frequencies[0].term, frequencies[0].frequency);
        
        FILE* f = fopen(output_file, "w");
        if (!f) {
            printf("Ошибка открытия файла: %s\n", output_file);
            return;
        }
        
        fprintf(f, "# Закон Ципфа\n");
        fprintf(f, "# Rank\tFrequency\tTerm\tZipf_Theoretical\n");
        
        double C = frequencies[0].frequency;
        double alpha = 1.0;
        
        int limit = frequencies.length() < 1000 ? frequencies.length() : 1000;
        
        for (int rank = 1; rank <= limit; rank++) {
            int actual_freq = frequencies[rank - 1].frequency;
            double theoretical_freq = C / pow(rank, alpha);
            
            fprintf(f, "%d\t%d\t%s\t%.2f\n", 
                    rank, actual_freq, frequencies[rank - 1].term, theoretical_freq);
        }
        
        fclose(f);
        
        double sum_diff = 0;
        int compare_limit = limit < 100 ? limit : 100;
        
        for (int rank = 1; rank <= compare_limit; rank++) {
            double actual = frequencies[rank - 1].frequency;
            double theoretical = C / pow(rank, alpha);
            double diff = fabs(actual - theoretical) / actual;
            sum_diff += diff;
        }
        
        double avg_diff = sum_diff / compare_limit;
        
        printf("Данные сохранены в: %s\n", output_file);
        printf("Среднее отклонение от закона Ципфа: %.2f%%\n", avg_diff * 100);
        printf("Время обработки: %.4f сек\n", timer.elapsed());
        
        printf("\nАнализ распределения:\n");
        printf("  Топ-10 термов покрывают %.2f%% всех вхождений\n", 
               calculate_coverage(frequencies, 10));
        printf("  Топ-100 термов покрывают %.2f%% всех вхождений\n", 
               calculate_coverage(frequencies, 100));
        printf("  Топ-1000 термов покрывают %.2f%% всех вхождений\n", 
               calculate_coverage(frequencies, 1000));
        
        int hapax_count = 0;
        for (int i = 0; i < frequencies.length(); i++) {
            if (frequencies[i].frequency == 1) {
                hapax_count++;
            }
        }
        
        printf("  Hapax legomena (1 раз): %d (%.2f%%)\n", 
               hapax_count, (hapax_count * 100.0) / frequencies.length());
        
    }
    
    double calculate_coverage(DynamicArray<TermFrequency>& frequencies, int top_n) {
        if (top_n > frequencies.length()) top_n = frequencies.length();
        
        long long top_sum = 0;
        long long total_sum = 0;
        
        for (int i = 0; i < frequencies.length(); i++) {
            total_sum += frequencies[i].frequency;
            if (i < top_n) {
                top_sum += frequencies[i].frequency;
            }
        }
        
        return (top_sum * 100.0) / total_sum;
    }
    
    void print_corpus_stats(Tokenizer* tokenizer) {
        
        printf("Документы:\n");
        printf("  Всего документов: %d\n", index->get_num_documents());
        printf("\n");
        
        printf("Термы:\n");
        printf("  Уникальных термов: %d\n", index->get_num_terms());
        printf("\n");
        
        if (tokenizer) {
            tokenizer->print_stats();
        }
        
    }
};

#endif // STATS_H
