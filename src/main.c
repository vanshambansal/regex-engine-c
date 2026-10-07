#include <stdio.h>
#include <string.h>
#include "regex/token.h"
#include "regex/parser.h"
#include "regex/nfa_builder.h"
#include "regex/simulate.h"

static void print_usage(const char *prog_name) {
    printf("Usage: %s <pattern> <text>\n", prog_name);
    printf("Options:\n");
    printf("  <pattern>   Regular expression pattern\n");
    printf("  <text>      Input text to match against\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 2;
    }

    const char *pattern = argv[1];
    const char *text = argv[2];

    char error_msg[256];
    TokenList tokens;
    if (!tokenize(pattern, &tokens, error_msg)) {
        fprintf(stderr, "Lexer error: %s\n", error_msg);
        return 2;
    }

    ASTNode *ast = parse(&tokens, error_msg);
    if (!ast) {
        fprintf(stderr, "Parser error: %s\n", error_msg);
        return 2;
    }

    NFA nfa;
    build_nfa(ast, &nfa);

    int matched = simulate_nfa(&nfa, text);

    free_ast(ast);

    if (matched) {
        printf("[MATCH] Pattern '%s' matches input '%s'\n", pattern, text);
        return 0;
    } else {
        printf("[NO MATCH] Pattern '%s' does not match input '%s'\n", pattern, text);
        return 1;
    }
}
