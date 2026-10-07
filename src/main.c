#include <stdio.h>
#include <string.h>
#include "regex/token.h"
#include "regex/parser.h"
#include "regex/nfa_builder.h"
#include "regex/simulate.h"

static void print_usage(const char *prog_name) {
    printf("Usage: %s <pattern> [text] [options]\n", prog_name);
    printf("Options:\n");
    printf("  --tree      Print the Abstract Syntax Tree (AST)\n");
}

static void print_ast_tree(const ASTNode *node, int depth) {
    if (!node) {
        return;
    }

    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    switch (node->type) {
        case NODE_LITERAL:
            printf("LITERAL '%c'\n", node->value);
            break;
        case NODE_DOT:
            printf("DOT (.)\n");
            break;
        case NODE_SEQ:
            printf("SEQ\n");
            break;
        case NODE_ALT:
            printf("ALT (|)\n");
            break;
        case NODE_STAR:
            printf("STAR (*)\n");
            break;
        case NODE_PLUS:
            printf("PLUS (+)\n");
            break;
        case NODE_QUESTION:
            printf("QUESTION (?)\n");
            break;
        case NODE_GROUP:
            printf("GROUP ()\n");
            break;
        case NODE_CHAR_CLASS:
            printf("CHAR_CLASS %s[", node->negate ? "^" : "");
            for (int i = 0; i < node->class_char_count; i++) {
                printf("%c", node->class_chars[i]);
            }
            printf("]\n");
            break;
        case NODE_SHORTHAND:
            printf("SHORTHAND \\%c\n", node->value);
            break;
        case NODE_RANGE:
            if (node->max == -1) {
                printf("RANGE {%d,}\n", node->min);
            } else {
                printf("RANGE {%d,%d}\n", node->min, node->max);
            }
            break;
    }

    print_ast_tree(node->left, depth + 1);
    print_ast_tree(node->right, depth + 1);
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
        fprintf(stderr, "Lexer error: %s\n", error_msg);
        return 2;
    }

    ASTNode *ast = parse(&tokens, error_msg);
    if (!ast) {
        fprintf(stderr, "Parser error: %s\n", error_msg);
        return 2;
    }

    if (show_tree) {
        printf("Syntax Tree (AST):\n");
        print_ast_tree(ast, 1);
    }

    // If only pattern and --tree were provided, exit cleanly
    if (!text) {
        free_ast(ast);
        return 0;
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
