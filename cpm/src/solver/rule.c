#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
    int id;
    char *version;
} PackageInfo;

typedef struct
{
    char **data;
    int size;
    int capacity;
} List;

typedef enum
{
    internal_allow_update,
    job_install,
    job_remove,
    package_conflict,
    package_requires,
    package_obsoletes,
    rule_installed_package_obsoletes,
    rule_package_same_name,
    rule_package_implicit_obsoletes,
    rule_learned,
    rule_package_alias
} RuleReason;

typedef struct
{
    struct Pool *pool;
    List literals;
    RuleReason reason;
    char *reason_details;
    struct Job *job;
    bool enabled;
    long id;
    char *rule_type;
    char *rule_hash;
} PackageRule;

typedef struct
{
    PackageRule **data;
    int size;
    int capacity;
} PackageRuleList;

typedef struct Pool
{
    // Pool fields here
} Pool;

typedef struct
{
    // Job fields here
} Job;

typedef struct
{
    // Requirement fields here
} Requirement;

typedef struct
{
    // Version fields here
} SemanticVersion;

typedef struct
{
    // Version fields here
} Version;

PackageRule *PackageRule_from_string(Pool *pool, char *rule_string, RuleReason reason, char *reason_details, Job *job, long id);
PackageRule *PackageRule_from_packages(Pool *pool, PackageInfo **packages, RuleReason reason, char *reason_details, Job *job, long id);
PackageRule *PackageRule_new(Pool *pool, List literals, RuleReason reason, char *reason_details, Job *job, long id, SemanticVersion *version_factory);
char *PackageRule_get_rule_hash(PackageRule *package_rule);
bool PackageRule_is_assertion(PackageRule *package_rule);
char *PackageRule_get_required_package_name(PackageRule *package_rule);
bool PackageRule_is_equivalent(PackageRule *package_rule, PackageRule *other);
char *PackageRule_to_string(PackageRule *package_rule);
char *PackageRule_to_repr(PackageRule *package_rule);
bool PackageRule_is_equal(PackageRule *package_rule, PackageRule *other);

int main()
{
    // Main function implementation
}

PackageRule *PackageRule_from_string(Pool *pool, char *rule_string, RuleReason reason, char *reason_details, Job *job, long id)
{
    char *token;
    char *package_string;
    bool positive;
    int package_id;
    List package_literals;
    char *delimiter = "|";
    PackageRule *package_rule = (PackageRule *)malloc(sizeof(PackageRule));
    package_rule->pool = pool;
    package_rule->literals = (List){.data = NULL, .size = 0, .capacity = 0};
    package_rule->reason = reason;
    package_rule->reason_details = reason_details;
    package_rule->job = job;
    package_rule->enabled = true;
    package_rule->id = id;
    package_rule->rule_type = "unknown";
    package_rule->rule_hash = "";

    token = strtok(rule_string, delimiter);
    while (token != NULL)
    {
        package_string = token;
        if (package_string[0] == '-')
        {
            positive = false;
            package_string++;
        }
        else
        {
            positive = true;
        }

        // Extract package id from package_string
        // You need to implement the logic to extract the package id from the package_string
        package_id = extract_package_id(pool, package_string);

        if (positive)
        {
            add_literal(&package_literals, package_id);
        }
        else
        {
            add_literal(&package_literals, -package_id);
        }

        token = strtok(NULL, delimiter);
    }

    package_rule->literals = package_literals;
    return package_rule;
}

PackageRule *PackageRule_from_packages(Pool *pool, PackageInfo **packages, RuleReason reason, char *reason_details, Job *job, long id)
{
    List literals;
    literals.data = (char **)malloc(sizeof(char *) * packages.size);
    literals.size = 0;
    literals.capacity = packages.size;

    for (int i = 0; i < packages.size; i++)
    {
        literals.data[i] = packages[i]->id;
        literals.size++;
    }

    PackageRule *package_rule = PackageRule_new(pool, literals, reason, reason_details, job, id, packages[0]->version_factory);
    return package_rule;
}

PackageRule *PackageRule_new(Pool *pool, List literals, RuleReason reason, char *reason_details, Job *job, long id, SemanticVersion *version_factory)
{
    PackageRule *package_rule = (PackageRule *)malloc(sizeof(PackageRule));
    package_rule->pool = pool;
    package_rule->literals = literals;
    package_rule->reason = reason;
    package_rule->reason_details = reason_details;
    package_rule->job = job;
    package_rule->enabled = true;
    package_rule->id = id;
    package_rule->rule_type = "unknown";
    package_rule->rule_hash = "";

    // Check reason and reason_details
    if (reason == job_install)
    {
        if (!is_valid_package_name(reason_details))
        {
            printf("reason_details must be a valid package name for 'job_install' rule\n");
            exit(1);
        }
    }
    else if (reason == package_requires)
    {
        Requirement *requirement = Requirement_from_string(reason_details, version_factory);
        if (requirement == NULL)
        {
            printf("Invalid requirement string '%s'\n", reason_details);
            exit(1);
        }
    }

    // Sort literals
    sort_literals(&package_rule->literals);

    return package_rule;
}

char *PackageRule_get_rule_hash(PackageRule *package_rule)
{
    // The exact rule hash algorithm is copied from composer
    char *data = "";
    for (int i = 0; i < package_rule->literals.size; i++)
    {
        char *literal_str = int_to_string(package_rule->literals.data[i]);
        data = strcat(data, literal_str);
        free(literal_str);
    }

    int data_len = strlen(data);
    int hash_len = (data_len < 5) ? data_len : 5;
    char *rule_hash = (char *)malloc(sizeof(char) * (hash_len + 1));
    strncpy(rule_hash, data, hash_len);
    rule_hash[hash_len] = '\0';

    return rule_hash;
}

bool PackageRule_is_assertion(PackageRule *package_rule)
{
    return package_rule->literals.size == 1;
}

char *PackageRule_get_required_package_name(PackageRule *package_rule)
{
    if (package_rule->reason == job_install)
    {
        return package_rule->reason_data;
    }
    else if (package_rule->reason == package_requires)
    {
        return Requirement_from_string(package_rule->reason_data);
    }
    else
    {
        return "";
    }
}

bool PackageRule_is_equivalent(PackageRule *package_rule, PackageRule *other)
{
    if (other == NULL)
    {
        return false;
    }
    if (package_rule->literals.size != other->literals.size)
    {
        return false;
    }
    for (int i = 0; i < package_rule->literals.size; i++)
    {
        if (package_rule->literals.data[i] != other->literals.data[i])
        {
            return false;
        }
    }
    return true;
}

char *PackageRule_to_string(PackageRule *package_rule)
{
    int max_length = 0;
    for (int i = 0; i < package_rule->literals.size; i++)
    {
        char *literal_str = id_to_string(package_rule->literals.data[i]);
        int length = strlen(literal_str);
        if (length > max_length)
        {
            max_length = length;
        }
        free(literal_str);
    }

    int str_length = (max_length + 3) * package_rule->literals.size;
    char *str = (char *)malloc(sizeof(char) * (str_length + 3));
    strcpy(str, "(");
    for (int i = 0; i < package_rule->literals.size; i++)
    {
        char *literal_str = id_to_string(package_rule->literals.data[i]);
        strcat(str, literal_str);
        if (i < package_rule->literals.size - 1)
        {
            strcat(str, " | ");
        }
        free(literal_str);
    }
    strcat(str, ")");

    return str;
}

char *PackageRule_to_repr(PackageRule *package_rule)
{
    int max_length = 0;
    for (int i = 0; i < package_rule->literals.size; i++)
    {
        char *literal_str = id_to_string(package_rule->literals.data[i]);
        int length = strlen(literal_str);
        if (length > max_length)
        {
            max_length = length;
        }
        free(literal_str);
    }

    int str_length = (max_length + 3) * package_rule->literals.size + strlen(package_rule->reason) + strlen(package_rule->reason_details) + 30;
    char *str = (char *)malloc(sizeof(char) * (str_length + 3));
    strcpy(str, "PackageRule('");
    for (int i = 0; i < package_rule->literals.size; i++)
    {
        char *literal_str = id_to_string(package_rule->literals.data[i]);
        strcat(str, literal_str);
        if (i < package_rule->literals.size - 1)
        {
            strcat(str, " | ");
        }
        free(literal_str);
    }
    strcat(str, "', '");
    strcat(str, package_rule->reason);
    strcat(str, "', '");
    strcat(str, package_rule->reason_details);
    strcat(str, "')");

    return str;
}

bool PackageRule_is_equal(PackageRule *package_rule, PackageRule *other)
{
    if (package_rule == other)
    {
        return true;
    }
    if (package_rule == NULL || other == NULL)
    {
        return false;
    }
    if (strcmp(PackageRule_to_repr(package_rule), PackageRule_to_repr(other)) == 0 &&
        strcmp(package_rule->reason, other->reason) == 0 &&
        strcmp(package_rule->reason_details, other->reason_details) == 0)
    {
        return true;
    }
    return false;
}
