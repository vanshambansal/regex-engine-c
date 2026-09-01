// include/regex/nfa.h
#ifndef REGEX_NFA_H
#define REGEX_NFA_H

typedef enum {
    MATCH_CHAR,        // matches one exact character
    MATCH_ANY,         // matches any character (from '.')
    MATCH_CLASS,       // matches a character class [a-z] or shorthand \d
    MATCH_EPSILON      // free transition, consumes no character
} MatchType;

typedef struct {
    MatchType match_type;

    char match_char;             // used by MATCH_CHAR
    int negate;                  // used by MATCH_CLASS
    char class_chars[128];       // used by MATCH_CLASS
    int class_char_count;        // used by MATCH_CLASS

    int target_state;            // which state this transition leads to
} Transition;

#define MAX_TRANSITIONS_PER_STATE 8

typedef struct {
    int id;
    Transition transitions[MAX_TRANSITIONS_PER_STATE];
    int transition_count;
} NFAState;

#define MAX_NFA_STATES 512

typedef struct {
    NFAState states[MAX_NFA_STATES];
    int state_count;
    int start_state;
    int accept_state;
} NFA;

#endif