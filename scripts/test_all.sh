#!/bin/bash

set -e

echo "Компиляция тестов..."
make tests
echo ""

echo "[1/3] Тест токенизатора..."
./test_tokenizer | tee results/test_tokenizer.log
echo ""

echo "[2/3] Тест поиска..."
./test_search | tee results/test_search.log
echo ""

echo "[3/3] Тест производительности..."
if [ -f "f1_racing_corpus_data_index.bin" ]; then
    ./test_performance f1_racing_corpus_data_index.bin | tee results/test_performance.log
else
    echo "Пропущен: индекс не найден"
fi
echo ""
echo "Логи сохранены в results/"
echo ""
