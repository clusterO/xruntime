#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    // TODO: Define PackageRule struct
    // ...
} PackageRule;

typedef struct
{
    // TODO: Define DecisionsSet struct
    // ...
} DecisionsSet;

typedef struct RuleWatchNode
{
    PackageRule *rule;
    int64_t _watch1;
    int64_t _watch2;
} RuleWatchNode;

typedef struct
{
    RuleWatchNode **nodes;
    int num_nodes;
} RuleWatchGraph;

RuleWatchNode *RuleWatchNode_new(PackageRule *rule)
{
    RuleWatchNode *node = (RuleWatchNode *)malloc(sizeof(RuleWatchNode));
    if (node == NULL)
    {
        return NULL;
    }

    node->rule = rule;
    node->_watch1 = -1;
    node->_watch2 = -1;

    if (rule->num_literals > 0)
    {
        node->_watch1 = rule->literals[0];
    }
    else
    {
        node->_watch1 = 0;
    }

    if (rule->num_literals > 1)
    {
        node->_watch2 = rule->literals[1];
    }
    else
    {
        node->_watch2 = 0;
    }

    return node;
}

void RuleWatchNode_watch2_on_highest(RuleWatchNode *node, DecisionsSet *decisions)
{
    if (node->rule->num_literals >= 3)
    {
        int watch_level = 0;
        for (int i = 0; i < node->rule->num_literals; i++)
        {
            int literal = node->rule->literals[i];
            int level = DecisionsSet_decision_level(decisions, literal);

            if (level > watch_level)
            {
                node->_watch2 = literal;
                watch_level = level;
            }
        }
    }
}

int RuleWatchNode_other_watch(RuleWatchNode *node, int literal)
{
    if (node->_watch1 == literal)
    {
        return node->_watch2;
    }
    else
    {
        return node->_watch1;
    }
}

void RuleWatchNode_move_watch(RuleWatchNode *node, int from_literal, int to_literal)
{
    if (node->_watch1 == from_literal)
    {
        node->_watch1 = to_literal;
    }
    else
    {
        node->_watch2 = to_literal;
    }
}

RuleWatchGraph *RuleWatchGraph_new()
{
    RuleWatchGraph *graph = (RuleWatchGraph *)malloc(sizeof(RuleWatchGraph));
    if (graph == NULL)
    {
        return NULL;
    }

    graph->nodes = NULL;
    graph->num_nodes = 0;

    return graph;
}

void RuleWatchGraph_insert(RuleWatchGraph *graph, RuleWatchNode *node)
{
    if (node->rule->is_assertion)
    {
        return;
    }

    int literal1 = node->_watch1;
    int literal2 = node->_watch2;

    bool literal1_exists = false;
    bool literal2_exists = false;
    for (int i = 0; i < graph->num_nodes; i++)
    {
        RuleWatchNode *existing_node = graph->nodes[i];
        if (existing_node->_watch1 == literal1 || existing_node->_watch2 == literal1)
        {
            literal1_exists = true;
        }
        if (existing_node->_watch1 == literal2 || existing_node->_watch2 == literal2)
        {
            literal2_exists = true;
        }
    }

    if (!literal1_exists)
    {
        RuleWatchNode **new_nodes = (RuleWatchNode **)realloc(graph->nodes, (graph->num_nodes + 1) * sizeof(RuleWatchNode *));
        if (new_nodes == NULL)
        {
            return;
        }
        graph->nodes = new_nodes;

        graph->nodes[graph->num_nodes] = node;
        graph->num_nodes++;
    }

    if (!literal2_exists)
    {
        RuleWatchNode **new_nodes = (RuleWatchNode **)realloc(graph->nodes, (graph->num_nodes + 1) * sizeof(RuleWatchNode *));
        if (new_nodes == NULL)
        {
            return;
        }
        graph->nodes = new_nodes;

        graph->nodes[graph->num_nodes] = node;
        graph->num_nodes++;
    }
}

void RuleWatchGraph_propagate_literal(RuleWatchGraph *graph, int decided_literal, int level, DecisionsSet *decisions)
{
    int literal = -decided_literal;

    for (int i = 0; i < graph->num_nodes; i++)
    {
        RuleWatchNode *node = graph->nodes[i];
        int other_watch = RuleWatchNode_other_watch(node, literal);

        if (node->rule->enabled && !DecisionsSet_satisfy(decisions, other_watch))
        {
            int *rule_literals = node->rule->literals;
            int num_literals = node->rule->num_literals;

            int *alternative_literals = (int *)malloc(num_literals * sizeof(int));
            if (alternative_literals == NULL)
            {
                return;
            }
            int num_alternative_literals = 0;

            for (int j = 0; j < num_literals; j++)
            {
                int rule_literal = rule_literals[j];
                if (rule_literal != literal && rule_literal != other_watch && !DecisionsSet_conflict(decisions, rule_literal))
                {
                    alternative_literals[num_alternative_literals] = rule_literal;
                    num_alternative_literals++;
                }
            }

            if (num_alternative_literals > 0)
            {
                RuleWatchNode_move_watch(node, literal, alternative_literals[0]);
                free(alternative_literals);
                continue;
            }

            if (DecisionsSet_conflict(decisions, other_watch))
            {
                // TODO: return conflicting rule
                return;
            }

            DecisionsSet_decide(decisions, other_watch, level, node->rule);
            free(alternative_literals);
        }
    }
}

void RuleWatchGraph_move_watch(RuleWatchGraph *graph, int from_literal, int to_literal, RuleWatchNode *node)
{
    bool to_literal_exists = false;
    for (int i = 0; i < graph->num_nodes; i++)
    {
        RuleWatchNode *existing_node = graph->nodes[i];
        if (existing_node->_watch1 == to_literal || existing_node->_watch2 == to_literal)
        {
            to_literal_exists = true;
        }
    }

    if (!to_literal_exists)
    {
        RuleWatchNode_move_watch(node, from_literal, to_literal);
    }
}

int main()
{
    // TODO: Write test code
    return 0;
}
