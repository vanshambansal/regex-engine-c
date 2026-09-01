CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SRC = src/tokenizer.c src/main.c
OUT = regex_engine

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

test_tokenizer: tests/test_tokenizer.c src/tokenizer.c
	$(CC) $(CFLAGS) tests/test_tokenizer.c src/tokenizer.c -o test_tokenizer
	./test_tokenizer

test_parser: tests/test_parser.c src/parser.c src/tokenizer.c
	$(CC) $(CFLAGS) tests/test_parser.c src/parser.c src/tokenizer.c -o test_parser
	./test_parser

test_nfa: tests/test_nfa.c src/nfa_builder.c src/parser.c src/tokenizer.c
	$(CC) $(CFLAGS) tests/test_nfa.c src/nfa_builder.c src/parser.c src/tokenizer.c -o test_nfa
	./test_nfa

clean:
	rm -f $(OUT) test_tokenizer test_parser test_nfa

.PHONY: all clean test_tokenizer test_parser test_nfa