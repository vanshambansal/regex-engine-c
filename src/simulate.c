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
