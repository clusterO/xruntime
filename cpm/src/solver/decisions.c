#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Decision {
    int literal;
    char* reason;
} Decision;

typedef struct DecisionsSet DecisionsSet;

struct DecisionsSet {
    // Package id -> decision level mapping
    int* decision_map;

    // Queue of Decision instances
    Decision* decision_queue;
    int queue_size;
};

DecisionsSet* create_decisions_set() {
    DecisionsSet* decisions_set = malloc(sizeof(DecisionsSet));
    decisions_set->decision_map = NULL;
    decisions_set->decision_queue = NULL;
    decisions_set->queue_size = 0;
    return decisions_set;
}

void destroy_decisions_set(DecisionsSet* decisions_set) {
    if (decisions_set == NULL) {
        return;
    }
    free(decisions_set->decision_map);
    for (int i = 0; i < decisions_set->queue_size; i++) {
        free(decisions_set->decision_queue[i].reason);
    }
    free(decisions_set->decision_queue);
    free(decisions_set);
}

void decide(DecisionsSet* decisions_set, int literal, int level, char* why) {
    int package_id = abs(literal);
    decisions_set->decision_map = realloc(decisions_set->decision_map, (package_id + 1) * sizeof(int));
    decisions_set->decision_map[package_id] = (literal > 0) ? level : -level;

    decisions_set->decision_queue = realloc(decisions_set->decision_queue, (decisions_set->queue_size + 1) * sizeof(Decision));
    decisions_set->decision_queue[decisions_set->queue_size].literal = literal;
    decisions_set->decision_queue[decisions_set->queue_size].reason = why;
    decisions_set->queue_size++;
}

bool satisfy(DecisionsSet* decisions_set, int literal) {
    int package_id = abs(literal);
    bool positive_case = (literal > 0) && (package_id < decisions_set->queue_size) && (decisions_set->decision_map[package_id] > 0);
    bool negative_case = (literal < 0) && (package_id < decisions_set->queue_size) && (decisions_set->decision_map[package_id] < 0);
    return positive_case || negative_case;
}

bool conflict(DecisionsSet* decisions_set, int literal) {
    int package_id = abs(literal);
    bool positive_case = (literal > 0) && (package_id < decisions_set->queue_size) && (decisions_set->decision_map[package_id] < 0);
    bool negative_case = (literal < 0) && (package_id < decisions_set->queue_size) && (decisions_set->decision_map[package_id] > 0);
    return positive_case || negative_case;
}

bool is_decided(DecisionsSet* decisions_set, int literal) {
    int package_id = abs(literal);
    return decisions_set->decision_map[package_id] != 0;
}

bool is_undecided(DecisionsSet* decisions_set, int literal) {
    int package_id = abs(literal);
    return decisions_set->decision_map[package_id] == 0;
}

bool is_decided_install(DecisionsSet* decisions_set, int literal) {
    int package_id = abs(literal);
    return decisions_set->decision_map[package_id] > 0;
}

int decision_level(DecisionsSet* decisions_set, int literal) {
    int package_id = abs(literal);
    if (package_id < decisions_set->queue_size) {
        return abs(decisions_set->decision_map[package_id]);
    } else {
        return 0;
    }
}

Decision* at_offset(DecisionsSet* decisions_set, int offset) {
    if (offset >= 0 && offset < decisions_set->queue_size) {
        return &(decisions_set->decision_queue[offset]);
    } else {
        return NULL;
    }
}

bool is_offset_valid(DecisionsSet* decisions_set, int offset) {
    return offset >= 0 && offset < decisions_set->queue_size;
}

void revert_last(DecisionsSet* decisions_set) {
    if (decisions_set->queue_size > 0) {
        int last_literal = decisions_set->decision_queue[decisions_set->queue_size - 1].literal;
        decisions_set->decision_map[abs(last_literal)] = 0;
        free(decisions_set->decision_queue[decisions_set->queue_size - 1].reason);
        decisions_set->queue_size--;
        decisions_set->decision_queue = realloc(decisions_set->decision_queue, decisions_set->queue_size * sizeof(Decision));
    }
}

int main() {
    DecisionsSet* decisions_set = create_decisions_set();

    // Example usage
    decide(decisions_set, 1, 1, "Reason 1");
    decide(decisions_set, 2, 2, "Reason 2");
    decide(decisions_set, -3, 3, "Reason 3");

    bool is_satisfied = satisfy(decisions_set, 1);
    bool has_conflict = conflict(decisions_set, -2);
    bool is_literal_decided = is_decided(decisions_set, 3);
    int literal_decision_level = decision_level(decisions_set, 1);

    revert_last(decisions_set);

    destroy_decisions_set(decisions_set);

    return 0;
}
