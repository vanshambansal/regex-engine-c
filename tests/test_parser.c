// tests/test_parser.c
#include <stdio.h>
#include "regex/token.h"
#include "regex/parser.h"

static int pass_count = 0;
static int fail_count = 0;

static void check(int condition, const char *label) {
    if (condition) {
        printf("PASS: %s\n", label);
        pass_count++;
    } else {
        printf("FAIL: %s\n", label);
        fail_count++;
    }
}

static ASTNode *parse_pattern(const char *pattern, char *error_msg) {
    TokenList tokens;
    if (!tokenize(pattern, &tokens, error_msg)) {
        return NULL;
    }
    return parse(&tokens, error_msg);
}

static void test_simple_seq(void) {
    char err[256];
    ASTNode *ast = parse_pattern("ab", err);

    check(ast != NULL, "ab parses successfully");
    check(ast->type == NODE_SEQ, "ab produces a SEQ node");
    check(ast->left->type == NODE_LITERAL && ast->left->value == 'a', "left child is 'a'");
    check(ast->right->type == NODE_LITERAL && ast->right->value == 'b', "right child is 'b'");
    free_ast(ast);
}

static void test_star(void) {
    char err[256];
    ASTNode *ast = parse_pattern("a*", err);

    check(ast != NULL, "a* parses successfully");
    check(ast->type == NODE_STAR, "a* produces a STAR node");
    check(ast->left->type == NODE_LITERAL && ast->left->value == 'a', "STAR wraps literal 'a'");
    free_ast(ast);
}

static void test_alternation(void) {
    char err[256];
    ASTNode *ast = parse_pattern("a|b", err);

    check(ast != NULL, "a|b parses successfully");
    check(ast->type == NODE_ALT, "a|b produces an ALT node");
    free_ast(ast);
}

static void test_group(void) {
    char err[256];
    ASTNode *ast = parse_pattern("(a|b)c", err);

    check(ast != NULL, "(a|b)c parses successfully");
    check(ast->type == NODE_SEQ, "(a|b)c produces a SEQ node");
    check(ast->left->type == NODE_GROUP, "left side is a GROUP");
    check(ast->left->left->type == NODE_ALT, "inside the group is an ALT");
    free_ast(ast);
}

static void test_char_class(void) {
    char err[256];
    ASTNode *ast = parse_pattern("[a-c]", err);

    check(ast != NULL, "[a-c] parses successfully");
    check(ast->type == NODE_CHAR_CLASS, "[a-c] produces a CHAR_CLASS node");
    check(ast->class_char_count == 3, "[a-c] expands to 3 characters");
    free_ast(ast);
}

static void test_range_quantifier(void) {
    char err[256];
    ASTNode *ast = parse_pattern("a{2,4}", err);

    check(ast != NULL, "a{2,4} parses successfully");
    check(ast->type == NODE_RANGE, "a{2,4} produces a RANGE node");
    check(ast->min == 2 && ast->max == 4, "a{2,4} has min=2 max=4");
    free_ast(ast);
}

static void test_malformed_pattern(void) {
    char err[256];
    ASTNode *ast = parse_pattern("(ab", err);

    check(ast == NULL, "(ab with no closing paren is rejected");
}

static void test_memory_cleanup(void) {
    free_ast(NULL);
    check(1, "free_ast handles NULL safely");

    char err[256];
    ASTNode *ast = parse_pattern("(a|b)*[0-9]+c{2,4}", err);
    check(ast != NULL, "complex expression parses successfully");
    free_ast(ast);
    check(1, "complex AST freed cleanly without error");
}

int main(void) {
    test_simple_seq();
    test_star();
    test_alternation();
    test_group();
    test_char_class();
    test_range_quantifier();
    test_malformed_pattern();
    test_memory_cleanup();

    printf("\n%d passed, %d failed\n", pass_count, fail_count);
    return fail_count == 0 ? 0 : 1;
}