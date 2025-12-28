#!/bin/bash

set -e  

if [ ! -d "data/f1_racing_corpus_data" ]; then
    echo "ERROR: Директория с данными не найдена!"
    echo "Поместите корпус в data/f1_racing_corpus_data/"
    exit 1
fi

mkdir -p results logs

echo "[1/6] Компиляция..."
make clean
make everything
echo "✓ Компиляция завершена"
echo ""

echo "[2/6] Индексация корпуса..."
./indexer data/f1_racing_corpus_data 2>&1 | tee logs/indexing.log
echo "✓ Индексация завершена"
echo ""

echo "[3/6] Генерация тестовых запросов..."
./generate_queries results/test_queries.txt 500
echo "✓ Запросы сгенерированы"
echo ""

echo "[4/6] Выполнение тестовых запросов..."
./search f1_racing_corpus_data_index.bin results/test_queries.txt > results/search_results.txt 2>&1
echo "✓ Поиск завершен"
echo ""

echo "[5/6] Тесты производительности..."
./test_performance f1_racing_corpus_data_index.bin > results/performance.txt 2>&1
echo "✓ Тесты производительности завершены"
echo ""

echo "[6/6] Анализ результатов..."
echo ""
echo "--- Размер индекса ---"
ls -lh f1_racing_corpus_data_index.bin
echo ""
echo "--- Статистика Ципфа ---"
head -20 zipf_data.txt
echo ""
echo "--- Производительность поиска ---"
tail -20 results/performance.txt
echo ""

echo "Результаты сохранены в директории results/"
echo ""
echo "Для запуска веб-интерфейса:"
echo "  ./web_server f1_racing_corpus_data_index.bin"
echo ""
