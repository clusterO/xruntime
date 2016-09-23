#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Package
{
    int id;
    char name[50];
    // Add other package properties here
} Package;

typedef struct Operation
{
    Package *package;
    char reason[100];
    // Add other operation properties here
} Operation;

typedef struct DefaultPolicy
{
    // Add policy properties here
} DefaultPolicy;

typedef struct Pool
{
    Package *packages;
    int numPackages;
    // Add other pool properties here
} Pool;

typedef struct DecisionsSet
{
    Operation *decisions;
    int numDecisions;
    // Add other decisions properties here
} DecisionsSet;

typedef struct Transaction
{
    DefaultPolicy *policy;
    Pool *pool;
    DecisionsSet *decisions;

    Package **installed_map;
    int installed_map_size;

    Operation **transactions;
    int transactions_size;
} Transaction;

Transaction *createTransaction(Pool *pool, DefaultPolicy *policy, Package **installed_map, int installed_map_size, DecisionsSet *decisions)
{
    Transaction *transaction = (Transaction *)malloc(sizeof(Transaction));
    transaction->policy = policy;
    transaction->pool = pool;
    transaction->decisions = decisions;

    transaction->installed_map = installed_map;
    transaction->installed_map_size = installed_map_size;

    transaction->transactions = NULL;
    transaction->transactions_size = 0;

    return transaction;
}

void destroyTransaction(Transaction *transaction)
{
    free(transaction->transactions);
    free(transaction);
}

void compute_operations(Transaction *transaction)
{
    transaction->transactions_size = 0;
    free(transaction->transactions);
    transaction->transactions = NULL;

    Package **install_means_update_map = (Package **)malloc(sizeof(Package *) * transaction->installed_map_size);
    memset(install_means_update_map, 0, sizeof(Package *) * transaction->installed_map_size);

    // Find updates
    for (int i = 0; i < transaction->decisions->numDecisions; i++)
    {
        Operation *decision = &(transaction->decisions->decisions[i]);
        int literal = decision->package->id;
        int package_id = abs(literal);
        Package *package = getPackageById(transaction->pool, package_id);

        if (literal <= 0 && isPackageInstalled(transaction->installed_map, transaction->installed_map_size, package_id))
        {
            Package **updates = findUpdatedPackages(transaction->policy, transaction->pool, transaction->installed_map, package);
            int literals_size = 1;
            while (updates[literals_size - 1] != NULL)
            {
                literals_size++;
            }

            for (int j = 0; j < literals_size; j++)
            {
                int update_literal = updates[j]->id;
                if (update_literal != literal)
                {
                    install_means_update_map[abs(update_literal)] = package;
                }
            }

            free(updates);
        }
    }

    // Process install, update, and uninstall maps
    Package **install_map = (Package **)malloc(sizeof(Package *) * transaction->installed_map_size);
    memset(install_map, 0, sizeof(Package *) * transaction->installed_map_size);
    int install_map_size = 0;

    Package **update_map = (Package **)malloc(sizeof(Package *) * transaction->installed_map_size);
    memset(update_map, 0, sizeof(Package *) * transaction->installed_map_size);
    int update_map_size = 0;

    Package **uninstall_map = (Package **)malloc(sizeof(Package *) * transaction->installed_map_size);
    memset(uninstall_map, 0, sizeof(Package *) * transaction->installed_map_size);
    int uninstall_map_size = 0;

    Package **ignore_remove = (Package **)malloc(sizeof(Package *) * transaction->installed_map_size);
    memset(ignore_remove, 0, sizeof(Package *) * transaction->installed_map_size);
    int ignore_remove_size = 0;

    for (int i = 0; i < transaction->decisions->numDecisions; i++)
    {
        Operation *decision = &(transaction->decisions->decisions[i]);
        int literal = decision->package->id;
        int package_id = abs(literal);
        Package *package = getPackageById(transaction->pool, package_id);

        // wanted & installed || !wanted & !installed
        if ((literal > 0 && isPackageInstalled(transaction->installed_map, transaction->installed_map_size, package_id)) ||
            (literal <= 0 && !isPackageInstalled(transaction->installed_map, transaction->installed_map_size, package_id)))
        {
            continue;
        }

        if (literal > 0)
        {
            if (isPackageInstalled(install_means_update_map, transaction->installed_map_size, package_id))
            {
                int source_id = getPackageIdByPackage(install_means_update_map[package_id]);
                Package *source = getPackageById(transaction->pool, source_id);
                update_map[update_map_size++] = package;

                ignore_remove[ignore_remove_size++] = source;
                install_means_update_map[package_id] = NULL;
            }
            else
            {
                install_map[install_map_size++] = package;
            }
        }
    }

    for (int i = 0; i < transaction->decisions->numDecisions; i++)
    {
        Operation *decision = &(transaction->decisions->decisions[i]);
        int literal = decision->package->id;
        int package_id = abs(literal);
        Package *package = getPackageById(transaction->pool, package_id);

        if (literal <= 0 && isPackageInstalled(transaction->installed_map, transaction->installed_map_size, package_id) &&
            !isPackageInstalled(ignore_remove, ignore_remove_size, package_id))
        {
            uninstall_map[uninstall_map_size++] = package;
        }
    }

    transaction_from_maps(transaction, install_map, install_map_size, update_map, update_map_size, uninstall_map, uninstall_map_size);

    free(install_means_update_map);
    free(install_map);
    free(update_map);
    free(uninstall_map);
    free(ignore_remove);
}

void transaction_from_maps(Transaction *transaction, Package **install_map, int install_map_size, Package **update_map, int update_map_size, Package **uninstall_map, int uninstall_map_size)
{
    Package **root_packages = find_root_packages(install_map, install_map_size, update_map, update_map_size);

    Package **queue = (Package **)malloc(sizeof(Package *) * transaction->pool->numPackages);
    int queue_size = 0;

    for (int i = 0; i < install_map_size; i++)
    {
        queue[queue_size++] = install_map[i];
    }
    for (int i = 0; i < update_map_size; i++)
    {
        queue[queue_size++] = update_map[i];
    }

    int *visited = (int *)malloc(sizeof(int) * transaction->pool->numPackages);
    memset(visited, 0, sizeof(int) * transaction->pool->numPackages);

    while (queue_size > 0)
    {
        Package *package = queue[--queue_size];
        int package_id = package->id;

        if (!visited[package_id])
        {
            queue[queue_size++] = package;

            for (int i = 0; i < package->numDependencies; i++)
            {
                char *dependency = package->dependencies[i];
                Package **possible_dependencies = what_provides(transaction->pool, dependency);

                for (int j = 0; possible_dependencies[j] != NULL; j++)
                {
                    queue[queue_size++] = possible_dependencies[j];
                }
            }

            visited[package_id] = 1;
        }
        else
        {
            if (isPackageInstalled(install_map, install_map_size, package_id))
            {
                int operation_index = getPackageIndexByPackage(install_map, install_map_size, package);
                _install(transaction, install_map[operation_index]);
                removePackageByIndex(install_map, &install_map_size, operation_index);
            }

            if (isPackageInstalled(update_map, update_map_size, package_id))
            {
                int operation_index = getPackageIndexByPackage(update_map, update_map_size, package);
                int source_id = getPackageIdByPackage(update_map[operation_index]);
                Package *source = getPackageById(transaction->pool, source_id);
                _update(transaction, source, update_map[operation_index]);
                removePackageByIndex(update_map, &update_map_size, operation_index);
            }
        }
    }

    for (int i = 0; i < uninstall_map_size; i++)
    {
        _uninstall(transaction, uninstall_map[i]);
    }

    free(root_packages);
    free(queue);
    free(visited);
}

Package **find_root_packages(Package **install_map, int install_map_size, Package **update_map, int update_map_size)
{
    Package **packages = (Package **)malloc(sizeof(Package *) * (install_map_size + update_map_size));
    int packages_size = 0;

    for (int i = 0; i < install_map_size; i++)
    {
        packages[packages_size++] = install_map[i];
    }
    for (int i = 0; i < update_map_size; i++)
    {
        packages[packages_size++] = update_map[i];
    }

    Package **roots = (Package **)malloc(sizeof(Package *) * packages_size);
    int roots_size = 0;

    for (int i = 0; i < packages_size; i++)
    {
        Package *package = packages[i];
        int package_id = package->id;
        int is_root = 1;

        for (int j = 0; j < packages_size; j++)
        {
            if (j == i)
            {
                continue;
            }
            Package *other_package = packages[j];
            if (isPackageProvisioned(other_package, package->dependencies, package->numDependencies))
            {
                is_root = 0;
                break;
            }
        }

        if (is_root)
        {
            roots[roots_size++] = package;
        }
    }

    free(packages);
    return roots;
}

Package **what_provides(Pool *pool, char *dependency)
{
    Package **result = (Package **)malloc(sizeof(Package *) * pool->numPackages);
    int result_size = 0;

    for (int i = 0; i < pool->numPackages; i++)
    {
        Package *package = &(pool->packages[i]);
        if (isPackageProvisioned(package, &dependency, 1))
        {
            result[result_size++] = package;
        }
    }

    result[result_size] = NULL;
    return result;
}

Package *getPackageById(Pool *pool, int package_id)
{
    for (int i = 0; i < pool->numPackages; i++)
    {
        Package *package = &(pool->packages[i]);
        if (package->id == package_id)
        {
            return package;
        }
    }
    return NULL;
}

int isPackageInstalled(Package **installed_map, int installed_map_size, int package_id)
{
    return installed_map[package_id] != NULL;
}

int isPackageProvisioned(Package *package, char **dependencies, int dependencies_size)
{
    for (int i = 0; i < dependencies_size; i++)
    {
        char *dependency = dependencies[i];
        int is_provided = 0;

        for (int j = 0; j < package->numProvides; j++)
        {
            char *provided = package->provides[j];
            if (strcmp(provided, dependency) == 0)
            {
                is_provided = 1;
                break;
            }
        }

        if (!is_provided)
        {
            return 0;
        }
    }

    return 1;
}

Package **findUpdatedPackages(DefaultPolicy *policy, Pool *pool, Package **installed_map, Package *package)
{
    Package **updated_packages = (Package **)malloc(sizeof(Package *) * pool->numPackages);
    int updated_packages_size = 0;

    for (int i = 0; i < pool->numPackages; i++)
    {
        Package *other_package = &(pool->packages[i]);
        if (isPackageUpdatable(policy, installed_map, other_package, package))
        {
            updated_packages[updated_packages_size++] = other_package;
        }
    }

    updated_packages[updated_packages_size] = NULL;
    return updated_packages;
}

int isPackageUpdatable(DefaultPolicy *policy, Package **installed_map, Package *package, Package *target_package)
{
    if (strcmp(package->name, target_package->name) != 0)
    {
        return 0;
    }

    if (!isPackageInstalled(installed_map, package->id, 0))
    {
        return 0;
    }

    // Check version constraints
    // Add your version comparison logic here

    return 1;
}

int getPackageIdByPackage(Package *package)
{
    return package->id;
}

int getPackageIndexByPackage(Package **packages, int packages_size, Package *package)
{
    for (int i = 0; i < packages_size; i++)
    {
        if (packages[i] == package)
        {
            return i;
        }
    }
    return -1;
}

void removePackageByIndex(Package **packages, int *packages_size, int index)
{
    for (int i = index; i < *packages_size - 1; i++)
    {
        packages[i] = packages[i + 1];
    }
    (*packages_size)--;
}

void _install(Transaction *transaction, Package *package)
{
    // Perform install operation
    printf("Installing package: %s\n", package->name);
    transaction->transactions = (Operation **)realloc(transaction->transactions, sizeof(Operation *) * (transaction->transactions_size + 1));
    Operation *operation = (Operation *)malloc(sizeof(Operation));
    operation->package = package;
    strcpy(operation->reason, "install");
    transaction->transactions[transaction->transactions_size++] = operation;
}

void _update(Transaction *transaction, Package *source, Package *target)
{
    // Perform update operation
    printf("Updating package: %s (from %s)\n", target->name, source->name);
    transaction->transactions = (Operation **)realloc(transaction->transactions, sizeof(Operation *) * (transaction->transactions_size + 1));
    Operation *operation = (Operation *)malloc(sizeof(Operation));
    operation->package = target;
    strcpy(operation->reason, "update");
    transaction->transactions[transaction->transactions_size++] = operation;
}

void _uninstall(Transaction *transaction, Package *package)
{
    // Perform uninstall operation
    printf("Uninstalling package: %s\n", package->name);
    transaction->transactions = (Operation **)realloc(transaction->transactions, sizeof(Operation *) * (transaction->transactions_size + 1));
    Operation *operation = (Operation *)malloc(sizeof(Operation));
    operation->package = package;
    strcpy(operation->reason, "uninstall");
    transaction->transactions[transaction->transactions_size++] = operation;
}
