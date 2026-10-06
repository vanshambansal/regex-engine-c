#ifndef REGEX_SIMULATE_H
#define REGEX_SIMULATE_H

#include "regex/nfa.h"

/*
 * Tracks a collection of active NFA states during simulation.
 * in_set provides O(1) membership checks to prevent duplicates,
 * while states array allows linear traversal of active states.
 */
typedef struct {
    int states[MAX_NFA_STATES];
    int count;
    int in_set[MAX_NFA_STATES];
} StateSet;

/* Initialize or reset a state set to empty */
void stateset_init(StateSet *set);

/* Insert a state into the set if not already present */
void stateset_add(StateSet *set, int state_id);

/*
 * Expand the state set in-place with all states reachable
 * through epsilon transitions without consuming any input.
 */
void compute_epsilon_closure(const NFA *nfa, StateSet *set);

/*
 * Transition from the current active states on input character 'c'.
 * Populates 'next' with target states that have matching transitions.
 */
void step_nfa(const NFA *nfa, const StateSet *current, char c, StateSet *next);

/*
 * Simulates the NFA against the entire input text.
 * Returns 1 if the pattern matches, 0 otherwise.
 */
int simulate_nfa(const NFA *nfa, const char *text);

#endif
