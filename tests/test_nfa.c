// tests/test_nfa.c
#include <stdio.h>
#include "regex/token.h"
#include "regex/parser.h"
#include "regex/nfa_builder.h"

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

static int build_from_pattern(const char *pattern, NFA *nfa, char *error_msg) {
    TokenList tokens;
    if (!tokenize(pattern, &tokens, error_msg)) {
        return 0;
    }

    ASTNode *ast = parse(&tokens, error_msg);
    if (ast == NULL) {
        return 0;
    }

    build_nfa(ast, nfa);
    return 1;
}

static void test_literal(void) {
    NFA nfa;
    char err[256];
    int ok = build_from_pattern("a", &nfa, err);

    check(ok, "'a' builds successfully");
    check(nfa.state_count == 2, "'a' produces 2 states");
    check(nfa.states[nfa.start_state].transition_count == 1, "start state has 1 transition");
    check(nfa.states[nfa.start_state].transitions[0].match_type == MATCH_CHAR, "transition matches a char");
    check(nfa.states[nfa.start_state].transitions[0].match_char == 'a', "transition char is 'a'");
}

static void test_seq(void) {
    NFA nfa;
    char err[256];
    int ok = build_from_pattern("ab", &nfa, err);

    check(ok, "'ab' builds successfully");
    check(nfa.state_count == 4, "'ab' produces 4 states");
}

static void test_alt(void) {
    NFA nfa;
    char err[256];
    int ok = build_from_pattern("a|b", &nfa, err);

    check(ok, "'a|b' builds successfully");
    check(nfa.states[nfa.start_state].transition_count == 2, "start state branches into 2 epsilons");
    check(nfa.states[nfa.start_state].transitions[0].match_type == MATCH_EPSILON, "branch 0 is epsilon");
    check(nfa.states[nfa.start_state].transitions[1].match_type == MATCH_EPSILON, "branch 1 is epsilon");
}

static void test_star(void) {
    NFA nfa;
    char err[256];
    int ok = build_from_pattern("a*", &nfa, err);

    check(ok, "'a*' builds successfully");
    check(nfa.states[nfa.start_state].transition_count == 2, "start has skip + enter epsilons");

    int accept_reachable_directly = 0;
    for (int i = 0; i < nfa.states[nfa.start_state].transition_count; i++) {
        if (nfa.states[nfa.start_state].transitions[i].target_state == nfa.accept_state) {
            accept_reachable_directly = 1;
        }
    }
    check(accept_reachable_directly, "'a*' start can skip straight to accept (zero matches allowed)");
}

static void test_plus(void) {
    NFA nfa;
    char err[256];
    int ok = build_from_pattern("a+", &nfa, err);

    check(ok, "'a+' builds successfully");

    int start_skips_to_accept = 0;
    for (int i = 0; i < nfa.states[nfa.start_state].transition_count; i++) {
        if (nfa.states[nfa.start_state].transitions[i].target_state == nfa.accept_state) {
            start_skips_to_accept = 1;
        }
    }
    check(!start_skips_to_accept, "'a+' start cannot skip straight to accept (at least one match required)");
}

static void test_char_class(void) {
    NFA nfa;
    char err[256];
    int ok = build_from_pattern("[a-c]", &nfa, err);

    check(ok, "'[a-c]' builds successfully");
    check(nfa.states[nfa.start_state].transitions[0].match_type == MATCH_CLASS, "transition is MATCH_CLASS");
    check(nfa.states[nfa.start_state].transitions[0].class_char_count == 3, "class has 3 characters");
}

static void test_range(void) {
    NFA nfa;
    char err[256];
    int ok = build_from_pattern("a{2,3}", &nfa, err);

    check(ok, "'a{2,3}' builds successfully");
}

int main(void) {
    test_literal();
    test_seq();
    test_alt();
    test_star();
    test_plus();
    test_char_class();
    test_range();

    printf("\n%d passed, %d failed\n", pass_count, fail_count);
    return fail_count == 0 ? 0 : 1;
}