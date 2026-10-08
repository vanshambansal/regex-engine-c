CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SRC = src/tokenizer.c src/parser.c src/nfa_builder.c src/simulate.c src/json_export.c src/main.c
OUT = regex_engine

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

test: test_tokenizer test_parser test_nfa test_simulate

test_tokenizer: tests/test_tokenizer.c src/tokenizer.c
	$(CC) $(CFLAGS) tests/test_tokenizer.c src/tokenizer.c -o test_tokenizer
	./test_tokenizer

test_parser: tests/test_parser.c src/parser.c src/tokenizer.c
	$(CC) $(CFLAGS) tests/test_parser.c src/parser.c src/tokenizer.c -o test_parser
	./test_parser

test_nfa: tests/test_nfa.c src/nfa_builder.c src/parser.c src/tokenizer.c
	$(CC) $(CFLAGS) tests/test_nfa.c src/nfa_builder.c src/parser.c src/tokenizer.c -o test_nfa
	./test_nfa

test_simulate: tests/test_simulate.c src/simulate.c src/nfa_builder.c src/parser.c src/tokenizer.c
	$(CC) $(CFLAGS) tests/test_simulate.c src/simulate.c src/nfa_builder.c src/parser.c src/tokenizer.c -o test_simulate
	./test_simulate

clean:
	rm -f $(OUT) test_tokenizer test_parser test_nfa test_simulate *.exe *.o

.PHONY: all clean test test_tokenizer test_parser test_nfa test_simulate