# MAI_IR

 ![IMAGE 2025-12-28 17:30:36](https://github.com/user-attachments/assets/6694fe41-a9ca-481c-b358-42549a8325bd)
 
 f1 and cars search
 
## Инструкция по запуску


### Компиляция 
```
make clean
make everything
```
### Индексация корпуса

С лемматизацией
```
./indexer data/f1_racing_corpus_data
```
Без лемматизации
```
./indexer data/f1_racing_corpus_data --no-lemma
```

### Поиск

Интерактивный режим
```
./search f1_racing_corpus_data_index.bin
```
Пакетный режим
```
./search f1_racing_corpus_data_index.bin queries.txt > results.txt
```
### Веб-интерфейс
```
./web_server f1_racing_corpus_data_index.bin
# Открыть в браузере: http://localhost:8080
```

### Тестирование
```
# Все тесты
make run_tests
# Только производительность
./test_performance f1_racing_corpus_data_index.bin
```
![IMAGE 2025-12-28 17:31:20](https://github.com/user-attachments/assets/4d8abd04-a7d5-4072-9a5a-3ead1680a884)

