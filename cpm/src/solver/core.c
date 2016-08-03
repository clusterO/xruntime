#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct _BranchInfo
{
    int32_t *literals;
    int level;
} BranchInfo;

typedef struct _Solver
{
    DefaultPolicy *policy;
    Pool *pool;
    Repository *installed_repository;
    OrderedDict *installed_map;
    OrderedDict *update_map;
    int32_t propagate_index;
    OrderedDict *learnt_why;
    BranchInfo *branches;
} Solver;

typedef struct _SolverResult
{
    DecisionsSet *decisions;
    Transaction *transaction;
} SolverResult;

Solver *createSolver(Pool *pool, Repository *installed_repository)
{
    Solver *solver = malloc(sizeof(Solver));
    solver->policy = createDefaultPolicy();
    solver->pool = pool;
    solver->installed_repository = installed_repository;
    solver->installed_map = createOrderedDict();
    solver->update_map = createOrderedDict();
    solver->propagate_index = -1;
    solver->learnt_why = createOrderedDict();
    solver->branches = NULL;
    return solver;
}

void destroySolver(Solver *solver)
{
    destroyDefaultPolicy(solver->policy);
    destroyPool(solver->pool);
    destroyRepository(solver->installed_repository);
    destroyOrderedDict(solver->installed_map);
    destroyOrderedDict(solver->update_map);
    destroyOrderedDict(solver->learnt_why);
    free(solver->branches);
    free(solver);
}

SolverResult *solve(Solver *solver, Request *request)
{
    DecisionsSet *decisions = solveInternal(solver, request);
    Transaction *transaction = calculateTransaction(solver, decisions);
    SolverResult *result = malloc(sizeof(SolverResult));
    result->decisions = decisions;
    result->transaction = transaction;
    return result;
}

DecisionsSet *solveInternal(Solver *solver, Request *request)
{
    DecisionsSet *decisions = prepareSolver(solver, request);
    makeAssertionRulesDecisions(solver, decisions, rulesSet);

    RulesWatchGraph *watchGraph = createRulesWatchGraph();
    for (int i = 0; i < rulesSet->length; i++)
    {
        RuleWatchNode *node = createRuleWatchNode(rulesSet->rules[i]);
        insertRuleWatchNode(watchGraph, node);
    }
    runSAT(solver, decisions, rulesSet, watchGraph, true);

    for (int i = 0; i < solver->installed_map->length; i++)
    {
        int32_t packageId = solver->installed_map->keys[i];
        if (!decisions->is_decided(packageId))
        {
            decisions->decide(-packageId, 1, "");
        }
    }

    return decisions;
}

Transaction *calculateTransaction(Solver *solver, DecisionsSet *decisions)
{
    Transaction *transaction = createTransaction(solver->pool, solver->policy, solver->installed_map, decisions);
    computeOperations(transaction);
    return transaction;
}

DecisionsSet *prepareSolver(Solver *solver, Request *request)
{
    setupInstallMap(solver, request->jobs);

    DecisionsSet *decisions = createDecisionsSet(solver->pool);

    RulesGenerator *rulesGenerator = createRulesGenerator(solver->pool, request, solver->installed_map);
    RulesSet *rulesSet = iterateRules(rulesGenerator);

    return decisions;
}

void makeAssertionRulesDecisions(Solver *solver, DecisionsSet *decisions, RulesSet *rulesSet)
{
    int decisionStart = decisions->length - 1;

    for (int i = 0; i < rulesSet->length; i++)
    {
        Rule *rule = rulesSet->rules[i];
        if (!rule->is_assertion || !rule->enabled)
        {
            continue;
        }

        int32_t *literals = rule->literals;
        int32_t literal = literals[0];

        if (!decisions->is_decided(abs(literal)))
        {
            decisions->decide(literal, 1, rule);
            continue;
        }

        if (decisions->satisfy(literal))
        {
            continue;
        }

        if (strcmp(rule->rule_type, "learnt") == 0)
        {
            rule->enabled = false;
            continue;
        }

        // handle error case
        printf("Error: Rule handling not implemented.\n");
        exit(1);
    }
}

void propagate(Solver *solver, DecisionsSet *decisions, RulesWatchGraph *watchGraph, int level)
{
    while (decisions->is_offset_valid(solver->propagate_index))
    {
        Decision *decision = decisions->at_offset(solver->propagate_index);
        Rule *conflict = propagateLiteral(watchGraph, decision->literal, level, decisions);
        solver->propagate_index++;
        if (conflict != NULL)
        {
            return conflict;
        }
    }
    return NULL;
}

int32_t *pruneUpdatedPackages(Solver *solver, int32_t *decisionQueue)
{
    if (solver->installed_repository->length != solver->update_map->length)
    {
        int32_t *prunedQueue = malloc(sizeof(int32_t));
        int prunedQueueSize = 0;
        for (int i = 0; i < decisionQueueSize; i++)
        {
            int32_t literal = decisionQueue[i];
            int32_t packageId = abs(literal);
            if (findOrderedDict(solver->installed_map, packageId) != NULL)
            {
                prunedQueue[prunedQueueSize] = literal;
                prunedQueueSize++;
                if (findOrderedDict(solver->update_map, packageId) != NULL)
                {
                    free(prunedQueue);
                    return decisionQueue;
                }
            }
        }
        free(decisionQueue);
        return prunedQueue;
    }
    return decisionQueue;
}

int setPropagateLearn(Solver *solver, DecisionsSet *decisions, RulesSet *rulesSet, RulesWatchGraph *watchGraph, int level, int32_t literal, int disableRules, Rule *rule)
{
    level++;

    decisions->decide(literal, level, rule);

    while (true)
    {
        Rule *conflict = propagate(decisions, watchGraph, level);
        if (conflict == NULL)
        {
            break;
        }

        if (level == 1)
        {
            printf("Error: Analyzing conflict rules not yet implemented.\n");
            exit(1);
        }

        int32_t learnLiteral, newLevel;
        Rule *newRule;
        char *why;
        analyze(level, rule, &learnLiteral, &newLevel, &newRule, &why);

        if (newLevel <= 0 || newLevel >= level)
        {
            printf("Error: Trying to revert to invalid level %d from level %d\n", newLevel, level);
            exit(1);
        }
        else if (newRule == NULL)
        {
            printf("Error: No rule was learned from analyzing rule at level %d.\n", level);
            exit(1);
        }

        level = newLevel;
        revert(solver, level);

        addRule(rulesSet, newRule, "learnt");
        insertOrderedDict(solver->learnt_why, newRule->id, why);

        RuleWatchNode *ruleNode = createRuleWatchNode(newRule);
        ruleNode->watch2_on_highest(decisions);
        insert(watchGraph, ruleNode);

        decisions->decide(learnLiteral, level, newRule);
    }
    return level;
}

int selectAndInstall(Solver *solver, DecisionsSet *decisions, RulesSet *rulesSet, RulesWatchGraph *watchGraph, int level, int32_t *decisionQueue, int disableRules, Rule *rule)
{
    int32_t *literals = selectPreferredPackages(solver->policy, solver->pool, solver->installed_map, decisionQueue);

    if (arraySize(literals) == 0)
    {
        printf("Error: Internal error in solver.\n");
        exit(1);
    }
    else
    {
        int32_t selectedLiteral;
        int32_t *remainingLiterals;
        arrayShift(literals, &selectedLiteral, &remainingLiterals);

        if (arraySize(remainingLiterals) > 0)
        {
            solver->branches = addElementToArrayList(solver->branches, &(solver->branches_size), remainingLiterals);
        }

        return setPropagateLearn(solver, decisions, rulesSet, watchGraph, level, selectedLiteral, disableRules, rule);
    }
}

void runSat(Solver *solver, DecisionsSet *decisions, RulesSet *rulesSet, RulesWatchGraph *watchGraph, int disableRules)
{
    solver->propagate_index = 0;

    // main loop
    int32_t *decisionQueue = NULL;
    int32_t *decisionSupplementQueue = NULL;
    int32_t *disableRules = NULL;

    int level = 1;
    int systemLevel = level + 1;
    int installedPos = 0;

    while (true)
    {
        if (level == 1)
        {
            Rule *conflict = propagate(decisions, watchGraph, level);
            if (conflict != NULL)
            {
                printf("Error: Analyzing conflict rules not yet implemented.\n");
                exit(1);
            }
        }

        // Handle job rules
        if (level < systemLevel)
        {
            for (int i = 0; i < rulesSet->job_rules_size; i++)
            {
                Rule *rule = rulesSet->job_rules[i];
                if (rule->enabled)
                {
                    decisionQueue = NULL;
                    bool noneSatisfied = true;

                    for (int j = 0; j < arraySize(rule->literals); j++)
                    {
                        int32_t literal = rule->literals[j];
                        if (decisions->satisfy(literal))
                        {
                            noneSatisfied = false;
                            break;
                        }

                        if (literal > 0 && decisions->is_undecided(literal))
                        {
                            decisionQueue = arrayPush(decisionQueue, literal);
                        }
                    }

                    if (noneSatisfied && arraySize(decisionQueue) > 0)
                    {
                        decisionQueue = pruneUpdatedPackages(solver, decisionQueue);
                    }

                    if (noneSatisfied && arraySize(decisionQueue) > 0)
                    {
                        int oldLevel = level;
                        level = selectAndInstall(solver, decisions, rulesSet, watchGraph, level, decisionQueue, disableRules, rule);

                        if (level == 0)
                        {
                            return;
                        }
                        if (level <= oldLevel)
                        {
                            break;
                        }
                    }
                }
            }
            systemLevel = level + 1;
        }

        if (level < systemLevel)
        {
            systemLevel = level;
        }

        int ruleId = 0;
        int n = 0;
        while (n < rulesSet->rules_size)
        {
            if (ruleId == rulesSet->rules_size)
            {
                ruleId = 0;
                if (n == 0)
                {
                    systemLevel = level + 1;
                }
                else
                {
                    break;
                }
            }
            Rule *rule = rulesSet->rules[ruleId];
            ruleId++;

            if (!rule->enabled || rule->is_assertion)
            {
                continue;
            }

            decisionQueue = NULL;

            bool noneSatisfied = true;
            for (int j = 0; j < arraySize(rule->literals); j++)
            {
                int32_t literal = rule->literals[j];
                if (decisions->satisfy(literal))
                {
                    noneSatisfied = false;
                    break;
                }
                if (literal > 0 && decisions->is_undecided(literal))
                {
                    decisionQueue = arrayPush(decisionQueue, literal);
                }
            }

            if (noneSatisfied && arraySize(decisionQueue) > 0)
            {
                decisionQueue = pruneUpdatedPackages(solver, decisionQueue);
            }

            if (noneSatisfied && arraySize(decisionQueue) > 0)
            {
                int oldLevel = level;
                level = selectAndInstall(solver, decisions, rulesSet, watchGraph, level, decisionQueue, disableRules, rule);

                if (level == 0)
                {
                    return;
                }
                if (level <= oldLevel)
                {
                    break;
                }
            }
            n++;
        }

        if (decisionSupplementQueue != NULL && arraySize(decisionSupplementQueue) > 0)
        {
            decisionQueue = decisionSupplementQueue;
            decisionSupplementQueue = NULL;
        }
        else if (solver->branches_size > 0)
        {
            int32_t *branch = solver->branches[solver->branches_size - 1];
            int32_t *literals = selectPreferredPackages(solver->policy, solver->pool, solver->installed_map, branch);

            if (arraySize(literals) > 0)
            {
                int32_t selectedLiteral;
                int32_t *remainingLiterals;
                arrayShift(literals, &selectedLiteral, &remainingLiterals);

                if (arraySize(remainingLiterals) > 0)
                {
                    solver->branches = removeLastElementFromArrayList(solver->branches, &(solver->branches_size));
                    solver->branches = addElementToArrayList(solver->branches, &(solver->branches_size), remainingLiterals);
                }

                decisionQueue = arrayPush(decisionQueue, selectedLiteral);
            }
        }
        else
        {
            break;
        }
    }

    solver->propagate_index = -1;
    solver->branches = NULL;
}

int main()
{
    Pool *pool = createPool();
    Repository *installed_repository = createRepository();
    Request *request = createRequest();
    Solver *solver = createSolver(pool, installed_repository);

    SolverResult *result = solve(solver, request);
    Transaction *transaction = result->transaction;
    performTransaction(transaction);

    destroySolver(solver);
    destroyTransaction(transaction);
    destroyRequest(request);
    destroyRepository(installed_repository);
    destroyPool(pool);

    return 0;
}
