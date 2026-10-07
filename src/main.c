#include <stdio.h>
#include <string.h>
#include "regex/token.h"
#include "regex/parser.h"
#include "regex/nfa_builder.h"
#include "regex/simulate.h"

// ANSI Color Codes for terminal formatting
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"

static void print_usage(const char *prog_name) {
    printf("Usage: %s <pattern> [text] [options]\n", prog_name);
    printf("Options:\n");
    printf("  --tree      Print the Abstract Syntax Tree (AST)\n");
}

static void print_ast_tree(const ASTNode *node, const char *prefix, int is_last) {
    if (!node) {
        return;
    }

    printf("%s%s%s%s", COLOR_DIM, prefix, is_last ? "└── " : "├── ", COLOR_RESET);

    switch (node->type) {
        case NODE_LITERAL:
            printf("%sLITERAL%s %s'%c'%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_GREEN, node->value, COLOR_RESET);
            break;
        case NODE_DOT:
            printf("%sDOT%s %s(Matches any character)%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
            break;
        case NODE_SEQ:
            printf("%sSEQ%s %s(Concatenation: left then right)%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
            break;
        case NODE_ALT:
            printf("%sALT%s %s(Choice: left OR right)%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
            break;
        case NODE_STAR:
            printf("%sSTAR%s %s(Repetition: 0 or more times)%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
            break;
        case NODE_PLUS:
            printf("%sPLUS%s %s(Repetition: 1 or more times)%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
            break;
        case NODE_QUESTION:
            printf("%sQUESTION%s %s(Optional: 0 or 1 time)%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
            break;
        case NODE_GROUP:
            printf("%sGROUP%s %s(Parentheses)%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
            break;
        case NODE_CHAR_CLASS:
            printf("%sCHAR_CLASS%s %s%s[", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_GREEN, node->negate ? "^" : "");
            for (int i = 0; i < node->class_char_count; i++) {
                printf("%c", node->class_chars[i]);
            }
            printf("]%s\n", COLOR_RESET);
            break;
        case NODE_SHORTHAND:
            printf("%sSHORTHAND%s %s\\%c%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_GREEN, node->value, COLOR_RESET);
            break;
        case NODE_RANGE:
            if (node->max == -1) {
                printf("%sRANGE%s %s{%d,}%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, node->min, COLOR_RESET);
            } else {
                printf("%sRANGE%s %s{%d,%d}%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, node->min, node->max, COLOR_RESET);
            }
            break;
    }

    char new_prefix[512];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");

    if (node->type == NODE_SEQ || node->type == NODE_ALT) {
        print_ast_tree(node->left, new_prefix, 0);
        print_ast_tree(node->right, new_prefix, 1);
    } else if (node->left) {
        print_ast_tree(node->left, new_prefix, 1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 2;
    }

    const char *pattern = NULL;
    const char *text = NULL;
    int show_tree = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tree") == 0) {
            show_tree = 1;
        } else if (!pattern) {
            pattern = argv[i];
        } else if (!text) {
            text = argv[i];
        }
    }

    if (!pattern) {
        print_usage(argv[0]);
        return 2;
    }

    char error_msg[256];
    TokenList tokens;
    if (!tokenize(pattern, &tokens, error_msg)) {
        fprintf(stderr, "%sLexer error:%s %s\n", COLOR_BOLD COLOR_RED, COLOR_RESET, error_msg);
        return 2;
    }

    ASTNode *ast = parse(&tokens, error_msg);
    if (!ast) {
        fprintf(stderr, "%sParser error:%s %s\n", COLOR_BOLD COLOR_RED, COLOR_RESET, error_msg);
        return 2;
    }

    if (show_tree) {
        printf("%sSyntax Tree (AST):%s\n", COLOR_BOLD, COLOR_RESET);
        print_ast_tree(ast, "", 1);
    }

    if (!text) {
        free_ast(ast);
        return 0;
    }

    NFA nfa;
    build_nfa(ast, &nfa);

    int matched = simulate_nfa(&nfa, text);

    free_ast(ast);

    if (matched) {
        printf("%s[MATCH]%s Pattern '%s' matches input '%s'\n", COLOR_BOLD COLOR_GREEN, COLOR_RESET, pattern, text);
        return 0;
    } else {
        printf("%s[NO MATCH]%s Pattern '%s' does not match input '%s'\n", COLOR_BOLD COLOR_RED, COLOR_RESET, pattern, text);
        return 1;
    }
}
