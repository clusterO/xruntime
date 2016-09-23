#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int id;
    char *name;
    char **dependencies;
    int num_dependencies;
    char **conflicts;
    int num_conflicts;
    char **replaces;
    int num_replaces;
} PackageInfo;

typedef struct
{
    int *literals;
    int num_literals;
    char *reason;
    char *reason_details;
    PackageInfo *package;
    struct _Job *job;
} PackageRule;

typedef struct
{
    PackageRule **unknown_rules;
    int num_unknown_rules;
    PackageRule **package_rules;
    int num_package_rules;
    PackageRule **job_rules;
    int num_job_rules;
    PackageRule **learnt_rules;
    int num_learnt_rules;
    char **rule_types;
    int num_rule_types;
    int **rules_by_hash;
    int *rules_by_id;
    int num_rules_by_id;
} RulesSet;

typedef struct
{
    Pool *pool;
    RulesSet *rules_set;
    DefaultPolicy *policy;
    Request *request;
    OrderedDict *installed_map;
    int *added_package_ids;
} RulesGenerator;

typedef struct
{
    char *name;
    char **dependencies;
    int num_dependencies;
} Requirement;

Requirement *Requirement_from_string(char *str)
{
    Requirement *requirement = (Requirement *)malloc(sizeof(Requirement));
    // Parse the requirement string and populate the fields of the Requirement struct
    return requirement;
}

void RulesSet_init(RulesSet *rules_set)
{
    rules_set->unknown_rules = NULL;
    rules_set->num_unknown_rules = 0;
    rules_set->package_rules = NULL;
    rules_set->num_package_rules = 0;
    rules_set->job_rules = NULL;
    rules_set->num_job_rules = 0;
    rules_set->learnt_rules = NULL;
    rules_set->num_learnt_rules = 0;
    rules_set->rule_types = NULL;
    rules_set->num_rule_types = 0;
    rules_set->rules_by_hash = NULL;
    rules_set->rules_by_id = NULL;
    rules_set->num_rules_by_id = 0;
}

void RulesGenerator_init(RulesGenerator *generator, Pool *pool, Request *request, OrderedDict *installed_map, DefaultPolicy *policy)
{
    generator->pool = pool;
    generator->rules_set = (RulesSet *)malloc(sizeof(RulesSet));
    RulesSet_init(generator->rules_set);
    generator->policy = policy;
    generator->request = request;
    generator->installed_map = installed_map;
    generator->added_package_ids = NULL;
}

PackageRule *PackageRule_create(Pool *pool, int *literals, int num_literals, char *reason, char *reason_details, PackageInfo *package, struct _Job *job)
{
    PackageRule *rule = (PackageRule *)malloc(sizeof(PackageRule));
    rule->literals = literals;
    rule->num_literals = num_literals;
    rule->reason = reason;
    rule->reason_details = reason_details;
    rule->package = package;
    rule->job = job;
    return rule;
}

void RulesSet_add_rule(RulesSet *rules_set, PackageRule *rule, char *rule_type)
{
    if (rule_type != NULL)
    {
        if (strcmp(rule_type, "unknown") == 0)
        {
            rules_set->unknown_rules = (PackageRule **)realloc(rules_set->unknown_rules, (rules_set->num_unknown_rules + 1) * sizeof(PackageRule *));
            rules_set->unknown_rules[rules_set->num_unknown_rules] = rule;
            rules_set->num_unknown_rules++;
        }
        else if (strcmp(rule_type, "package") == 0)
        {
            rules_set->package_rules = (PackageRule **)realloc(rules_set->package_rules, (rules_set->num_package_rules + 1) * sizeof(PackageRule *));
            rules_set->package_rules[rules_set->num_package_rules] = rule;
            rules_set->num_package_rules++;
        }
        else if (strcmp(rule_type, "job") == 0)
        {
            rules_set->job_rules = (PackageRule **)realloc(rules_set->job_rules, (rules_set->num_job_rules + 1) * sizeof(PackageRule *));
            rules_set->job_rules[rules_set->num_job_rules] = rule;
            rules_set->num_job_rules++;
        }
        else if (strcmp(rule_type, "learnt") == 0)
        {
            rules_set->learnt_rules = (PackageRule **)realloc(rules_set->learnt_rules, (rules_set->num_learnt_rules + 1) * sizeof(PackageRule *));
            rules_set->learnt_rules[rules_set->num_learnt_rules] = rule;
            rules_set->num_learnt_rules++;
        }
        else
        {
            printf("Invalid rule_type %s\n", rule_type);
            exit(1);
        }
    }

    rule->id = rules_set->num_rules_by_id;
    rules_set->rules_by_id = (int *)realloc(rules_set->rules_by_id, (rules_set->num_rules_by_id + 1) * sizeof(int));
    rules_set->rules_by_id[rules_set->num_rules_by_id] = rule->id;
    rules_set->num_rules_by_id++;

    // TODO: Update rules_by_hash
}

void RulesGenerator_add_rule(RulesGenerator *generator, PackageRule *rule, char *rule_type)
{
    if (rule != NULL && !RulesSet_contains(generator->rules_set, rule))
    {
        RulesSet_add_rule(generator->rules_set, rule, rule_type);
    }
}

void RulesGenerator_add_package_rules(RulesGenerator *generator, PackageInfo *package)
{
    deque *work_queue = deque_new();
    deque_append(work_queue, package);

    while (deque_len(work_queue) > 0)
    {
        PackageInfo *p = (PackageInfo *)deque_popleft(work_queue);

        if (!RulesGenerator_package_added(generator, p->id))
        {
            RulesGenerator_add_package_ids(generator, p->id);

            for (int i = 0; i < p->num_dependencies; i++)
            {
                char *dependency = p->dependencies[i];
                PackageInfo **dependency_candidates = Pool_what_provides(generator->pool, dependency);

                for (int j = 0; j < Pool_num_providers(generator->pool, dependency); j++)
                {
                    PackageInfo *dependency_candidate = dependency_candidates[j];
                    PackageRule *rule = RulesGenerator_create_dependency_rule(generator, p, dependency_candidate, "package_requires", dependency);
                    RulesGenerator_add_rule(generator, rule, "package");
                    deque_append(work_queue, dependency_candidate);
                }
            }

            for (int i = 0; i < p->num_conflicts; i++)
            {
                char *conflict = p->conflicts[i];
                PackageInfo **conflict_candidates = Pool_what_provides(generator->pool, conflict);

                for (int j = 0; j < Pool_num_providers(generator->pool, conflict); j++)
                {
                    PackageInfo *conflict_candidate = conflict_candidates[j];
                    PackageRule *rule = RulesGenerator_create_conflicts_rule(generator, p, conflict_candidate, "package_conflict", "%s conflict with %s", p->name, conflict_candidate->name);
                    RulesGenerator_add_rule(generator, rule, "package");
                }
            }

            int is_installed = Pool_package_installed(generator->pool, p->id);

            for (int i = 0; i < p->num_replaces; i++)
            {
                char *replace = p->replaces[i];
                PackageInfo **replace_candidates = Pool_what_provides(generator->pool, replace);

                for (int j = 0; j < Pool_num_providers(generator->pool, replace); j++)
                {
                    PackageInfo *replace_candidate = replace_candidates[j];
                    if (replace_candidate != p)
                    {
                        char *reason;
                        if (is_installed)
                        {
                            reason = "rule_installed_package_obsoletes";
                        }
                        else
                        {
                            reason = "package_obsoletes";
                        }
                        PackageRule *rule = RulesGenerator_create_conflicts_rule(generator, p, replace_candidate, reason, "%s replaces %s", p->name, replace_candidate->name);
                        RulesGenerator_add_rule(generator, rule, "package");
                    }
                }
            }

            PackageInfo **obsolete_providers = Pool_what_provides(generator->pool, p->name);
            for (int i = 0; i < Pool_num_providers(generator->pool, p->name); i++)
            {
                PackageInfo *provider = obsolete_providers[i];
                if (provider != p)
                {
                    char *reason;
                    if (strcmp(provider->name, p->name) == 0)
                    {
                        reason = "rule_package_same_name";
                    }
                    else
                    {
                        reason = "rule_package_implicit_obsoletes";
                    }
                    PackageRule *rule = RulesGenerator_create_conflicts_rule(generator, p, provider, reason, p->name);
                    RulesGenerator_add_rule(generator, rule, "package");
                }
            }
        }
    }
}

void RulesGenerator_add_updated_packages_rules(RulesGenerator *generator, PackageInfo *package)
{
    PackageInfo **updates = DefaultPolicy_find_updated_packages(generator->policy, generator->pool, generator->installed_map, package);

    for (int i = 0; i < DefaultPolicy_num_updates(generator->policy, package); i++)
    {
        RulesGenerator_add_package_rules(generator, updates[i]);
    }
}

void RulesGenerator_add_install_job_rules(RulesGenerator *generator, Job *job)
{
    if (job->num_packages < 1)
    {
        return;
    }

    for (int i = 0; i < job->num_packages; i++)
    {
        PackageInfo *package = job->packages[i];
        if (!Pool_package_installed(generator->pool, package->id))
        {
            RulesGenerator_add_package_rules(generator, package);
        }
    }

    PackageRule *rule = RulesGenerator_create_install_one_rule(generator, job->packages, "job_install", job->requirement->name, job);
    RulesGenerator_add_rule(generator, rule, "job");
}

void RulesGenerator_add_job_rules(RulesGenerator *generator)
{
    for (int i = 0; i < generator->request->num_jobs; i++)
    {
        Job *job = generator->request->jobs[i];
        if (strcmp(job->job_type, "install") == 0)
        {
            RulesGenerator_add_install_job_rules(generator, job);
        }
    }
}

RulesSet *RulesGenerator_iter_rules(RulesGenerator *generator)
{
    RulesGenerator_add_job_rules(generator);
    return generator->rules_set;
}
