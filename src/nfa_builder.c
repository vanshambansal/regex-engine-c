// src/nfa_builder.c
#include <string.h>
#include "regex/nfa_builder.h"

typedef struct {
    int start;
    int accept;
} NFAFragment;

static int new_state(NFA *nfa) {
    int id = nfa->state_count;
    nfa->states[id].id = id;
    nfa->states[id].transition_count = 0;
    nfa->state_count++;
    return id;
}

static void add_transition(NFA *nfa, int from, Transition t) {
    NFAState *state = &nfa->states[from];
    state->transitions[state->transition_count] = t;
    state->transition_count++;
}

static Transition make_epsilon(int target) {
    Transition t;
    t.match_type = MATCH_EPSILON;
    t.target_state = target;
    return t;
}

static Transition make_char(char c, int target) {
    Transition t;
    t.match_type = MATCH_CHAR;
    t.match_char = c;
    t.target_state = target;
    return t;
}

static Transition make_any(int target) {
    Transition t;
    t.match_type = MATCH_ANY;
    t.target_state = target;
    return t;
}

static NFAFragment build_fragment(ASTNode *node, NFA *nfa) {
    if (node->type == NODE_LITERAL) {
        int start = new_state(nfa);
        int accept = new_state(nfa);
        add_transition(nfa, start, make_char(node->value, accept));
        NFAFragment frag;
        frag.start = start;
        frag.accept = accept;
        return frag;
    }

    if (node->type == NODE_DOT) {
        int start = new_state(nfa);
        int accept = new_state(nfa);
        add_transition(nfa, start, make_any(accept));
        NFAFragment frag;
        frag.start = start;
        frag.accept = accept;
        return frag;
    }
    if (node->type == NODE_SEQ) {
    NFAFragment left = build_fragment(node->left, nfa);
    NFAFragment right = build_fragment(node->right, nfa);

    add_transition(nfa, left.accept, make_epsilon(right.start));

    NFAFragment frag;
    frag.start = left.start;
    frag.accept = right.accept;
    return frag;
}

    if (node->type == NODE_ALT) {
        NFAFragment left = build_fragment(node->left, nfa);
        NFAFragment right = build_fragment(node->right, nfa);

        int start = new_state(nfa);
        int accept = new_state(nfa);

        add_transition(nfa, start, make_epsilon(left.start));
        add_transition(nfa, start, make_epsilon(right.start));
        add_transition(nfa, left.accept, make_epsilon(accept));
        add_transition(nfa, right.accept, make_epsilon(accept));

        NFAFragment frag;
        frag.start = start;
        frag.accept = accept;
        return frag;
    }
    if (node->type == NODE_STAR) {
    NFAFragment inner = build_fragment(node->left, nfa);

    int start = new_state(nfa);
    int accept = new_state(nfa);

    add_transition(nfa, start, make_epsilon(inner.start));
    add_transition(nfa, start, make_epsilon(accept));
    add_transition(nfa, inner.accept, make_epsilon(inner.start));
    add_transition(nfa, inner.accept, make_epsilon(accept));

    NFAFragment frag;
    frag.start = start;
    frag.accept = accept;
    return frag;
}

    if (node->type == NODE_PLUS) {
        NFAFragment inner = build_fragment(node->left, nfa);

        int accept = new_state(nfa);

        add_transition(nfa, inner.accept, make_epsilon(inner.start));
        add_transition(nfa, inner.accept, make_epsilon(accept));

        NFAFragment frag;
        frag.start = inner.start;
        frag.accept = accept;
        return frag;
    }

    if (node->type == NODE_QUESTION) {
        NFAFragment inner = build_fragment(node->left, nfa);

        int start = new_state(nfa);
        int accept = new_state(nfa);

        add_transition(nfa, start, make_epsilon(inner.start));
        add_transition(nfa, start, make_epsilon(accept));
        add_transition(nfa, inner.accept, make_epsilon(accept));

        NFAFragment frag;
        frag.start = start;
        frag.accept = accept;
        return frag;
    }
    if (node->type == NODE_GROUP) {
        return build_fragment(node->left, nfa);
    }

    if (node->type == NODE_SHORTHAND) {
        int start = new_state(nfa);
        int accept = new_state(nfa);

        Transition t;
        t.match_type = MATCH_CLASS;
        t.target_state = accept;
        t.negate = 0;
        t.class_char_count = 0;

        switch (node->value) {
            case 'd': strcpy(t.class_chars, "0123456789"); t.class_char_count = 10; break;
            case 'D': strcpy(t.class_chars, "0123456789"); t.class_char_count = 10; t.negate = 1; break;
            case 's': strcpy(t.class_chars, " \t\n\r"); t.class_char_count = 4; break;
            case 'S': strcpy(t.class_chars, " \t\n\r"); t.class_char_count = 4; t.negate = 1; break;
            case 'w':
            case 'W': {
                int count = 0;
                for (char c = 'a'; c <= 'z'; c++) t.class_chars[count++] = c;
                for (char c = 'A'; c <= 'Z'; c++) t.class_chars[count++] = c;
                for (char c = '0'; c <= '9'; c++) t.class_chars[count++] = c;
                t.class_chars[count++] = '_';
                t.class_char_count = count;
                t.negate = (node->value == 'W') ? 1 : 0;
                break;
            }
        }

        add_transition(nfa, start, t);

        NFAFragment frag;
        frag.start = start;
        frag.accept = accept;
        return frag;
    }

    if (node->type == NODE_CHAR_CLASS) {
        int start = new_state(nfa);
        int accept = new_state(nfa);

        Transition t;
        t.match_type = MATCH_CLASS;
        t.target_state = accept;
        t.negate = node->negate;
        t.class_char_count = node->class_char_count;
        memcpy(t.class_chars, node->class_chars, node->class_char_count);

        add_transition(nfa, start, t);

        NFAFragment frag;
        frag.start = start;
        frag.accept = accept;
        return frag;
    }
    if (node->type == NODE_RANGE) {
    int min = node->min;
    int max = node->max;

    int overall_start = -1;
    int prev_accept = -1;
    int have_first = 0;

    for (int i = 0; i < min; i++) {
        NFAFragment copy = build_fragment(node->left, nfa);
        if (!have_first) {
            overall_start = copy.start;
            have_first = 1;
        } else {
            add_transition(nfa, prev_accept, make_epsilon(copy.start));
        }
        prev_accept = copy.accept;
    }

    if (max == -1) {
        NFAFragment extra = build_fragment(node->left, nfa);
        int loop_start = new_state(nfa);
        int loop_accept = new_state(nfa);

        add_transition(nfa, loop_start, make_epsilon(extra.start));
        add_transition(nfa, loop_start, make_epsilon(loop_accept));
        add_transition(nfa, extra.accept, make_epsilon(extra.start));
        add_transition(nfa, extra.accept, make_epsilon(loop_accept));

        if (have_first) {
            add_transition(nfa, prev_accept, make_epsilon(loop_start));
        } else {
            overall_start = loop_start;
        }

        NFAFragment frag;
        frag.start = overall_start;
        frag.accept = loop_accept;
        return frag;
    }

    int optional_count = max - min;
    for (int i = 0; i < optional_count; i++) {
        NFAFragment copy = build_fragment(node->left, nfa);
        int opt_start = new_state(nfa);
        int opt_accept = new_state(nfa);

        add_transition(nfa, opt_start, make_epsilon(copy.start));
        add_transition(nfa, opt_start, make_epsilon(opt_accept));
        add_transition(nfa, copy.accept, make_epsilon(opt_accept));

        if (have_first) {
            add_transition(nfa, prev_accept, make_epsilon(opt_start));
        } else {
            overall_start = opt_start;
            have_first = 1;
        }
        prev_accept = opt_accept;
    }

    NFAFragment frag;
    frag.start = overall_start;
    frag.accept = prev_accept;
    return frag;
}
    NFAFragment empty;
    empty.start = -1;
    empty.accept = -1;
    return empty;
}

void build_nfa(ASTNode *root, NFA *nfa) {
    nfa->state_count = 0;
    NFAFragment frag = build_fragment(root, nfa);
    nfa->start_state = frag.start;
    nfa->accept_state = frag.accept;
}