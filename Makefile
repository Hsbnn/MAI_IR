CXX = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++11
LDFLAGS = 

SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

TARGETS = indexer search web_server crawler

TEST_TARGETS = test_tokenizer test_search test_performance generate_queries

all: $(TARGETS)

tests: $(TEST_TARGETS)

everything: all tests

indexer: $(SRC_DIR)/indexer_main.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o indexer $(SRC_DIR)/indexer_main.cpp $(LDFLAGS)

search: $(SRC_DIR)/search_main.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o search $(SRC_DIR)/search_main.cpp $(LDFLAGS)

web_server: $(SRC_DIR)/web_server.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o web_server $(SRC_DIR)/web_server.cpp $(LDFLAGS)

crawler: $(SRC_DIR)/crawler_main.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o crawler $(SRC_DIR)/crawler_main.cpp $(LDFLAGS)

test_tokenizer: $(TEST_DIR)/test_tokenizer.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o test_tokenizer $(TEST_DIR)/test_tokenizer.cpp $(LDFLAGS)

test_search: $(TEST_DIR)/test_search.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o test_search $(TEST_DIR)/test_search.cpp $(LDFLAGS)

test_performance: $(TEST_DIR)/test_performance.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o test_performance $(TEST_DIR)/test_performance.cpp $(LDFLAGS)

generate_queries: $(TEST_DIR)/generate_queries.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -o generate_queries $(TEST_DIR)/generate_queries.cpp $(LDFLAGS)

clean:
	rm -f $(TARGETS) $(TEST_TARGETS) *.o *.bin *.jsonl

distclean: clean
	rm -rf results/ logs/ *.txt *.csv

install: all
	mkdir -p ~/bin
	cp $(TARGETS) ~/bin/

# Запуск тестов
run_tests: tests
	@echo "  Запуск тестов"
	@./test_tokenizer
	@./test_search
	@./test_performance

.PHONY: all tests everything clean distclean install run_tests
