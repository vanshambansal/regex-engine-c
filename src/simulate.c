#include <string.h>
#include "regex/simulate.h"

void stateset_init(StateSet *set) {
    set->count = 0;
    memset(set->in_set, 0, sizeof(set->in_set));
}

void stateset_add(StateSet *set, int state_id) {
    if (state_id < 0 || state_id >= MAX_NFA_STATES) {
        return;
    }
    if (!set->in_set[state_id]) {
        set->in_set[state_id] = 1;
        set->states[set->count++] = state_id;
    }
}

void compute_epsilon_closure(const NFA *nfa, StateSet *set) {
    // Traverse active states; newly added states are also scanned
    for (int i = 0; i < set->count; i++) {
        int state_id = set->states[i];
        const NFAState *state = &nfa->states[state_id];

        for (int t = 0; t < state->transition_count; t++) {
            const Transition *tr = &state->transitions[t];
            if (tr->match_type == MATCH_EPSILON) {
                stateset_add(set, tr->target_state);
            }
        }
    }
}

static int matches_transition(const Transition *tr, char c) {
    switch (tr->match_type) {
        case MATCH_CHAR:
            return c == tr->match_char;

        case MATCH_ANY:
            return c != '\0';

        case MATCH_CLASS: {
            int found = 0;
            for (int i = 0; i < tr->class_char_count; i++) {
                if (tr->class_chars[i] == c) {
                    found = 1;
                    break;
                }
            }
            return tr->negate ? !found : found;
        }

        case MATCH_EPSILON:
        default:
            return 0;
    }
}

void step_nfa(const NFA *nfa, const StateSet *current, char c, StateSet *next) {
    stateset_init(next);

    for (int i = 0; i < current->count; i++) {
        int state_id = current->states[i];
        const NFAState *state = &nfa->states[state_id];

        for (int t = 0; t < state->transition_count; t++) {
            const Transition *tr = &state->transitions[t];
            if (matches_transition(tr, c)) {
                stateset_add(next, tr->target_state);
            }
        }
    }

    compute_epsilon_closure(nfa, next);
}

int simulate_nfa(const NFA *nfa, const char *text) {
    if (!nfa || !text) {
        return 0;
    }

    StateSet current;
    stateset_init(&current);
    stateset_add(&current, nfa->start_state);
    compute_epsilon_closure(nfa, &current);

    StateSet next;
    int i = 0;

    while (text[i] != '\0') {
        if (current.count == 0) {
            return 0;
        }
        step_nfa(nfa, &current, text[i], &next);
        current = next;
        i++;
    }

    return current.in_set[nfa->accept_state] ? 1 : 0;
}
