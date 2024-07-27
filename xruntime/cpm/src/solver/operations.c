#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char *name;
    char *version;
} Package;

typedef struct
{
    Package *from_package;
    Package *to_package;
    char *reason;
} Update;

typedef struct
{
    Package *package;
    char *reason;
} Install, Uninstall;

int operation_equal(Operation *op1, Operation *op2)
{
    return op1->type == op2->type && op1->equal(op1, op2);
}

int update_equal(Update *update1, Update *update2)
{
    if (update1 == update2)
    {
        return 1;
    }
    if (update1 == NULL || update2 == NULL)
    {
        return 0;
    }
    return package_equal(update1->from_package, update2->from_package) &&
           package_equal(update1->to_package, update2->to_package) &&
           strcmp(update1->reason, update2->reason) == 0;
}

int package_equal(Package *pkg1, Package *pkg2)
{
    if (pkg1 == pkg2)
    {
        return 1;
    }
    if (pkg1 == NULL || pkg2 == NULL)
    {
        return 0;
    }
    return strcmp(pkg1->name, pkg2->name) == 0 &&
           strcmp(pkg1->version, pkg2->version) == 0;
}

Operation *create_update(Package *from_pkg, Package *to_pkg, char *reason)
{
    Update *update = (Update *)malloc(sizeof(Update));
    update->from_package = from_pkg;
    update->to_package = to_pkg;
    update->reason = reason;
    Operation *operation = (Operation *)malloc(sizeof(Operation));
    operation->type = UPDATE;
    operation->equal = (int (*)(Operation *, Operation *))update_equal;
    operation->data = update;
    return operation;
}

Operation *create_install(Package *pkg, char *reason)
{
    Install *install = (Install *)malloc(sizeof(Install));
    install->package = pkg;
    install->reason = reason;
    Operation *operation = (Operation *)malloc(sizeof(Operation));
    operation->type = INSTALL;
    operation->equal = (int (*)(Operation *, Operation *))install_equal;
    operation->data = install;
    return operation;
}

Operation *create_uninstall(Package *pkg, char *reason)
{
    Uninstall *uninstall = (Uninstall *)malloc(sizeof(Uninstall));
    uninstall->package = pkg;
    uninstall->reason = reason;
    Operation *operation = (Operation *)malloc(sizeof(Operation));
    operation->type = UNINSTALL;
    operation->equal = (int (*)(Operation *, Operation *))uninstall_equal;
    operation->data = uninstall;
    return operation;
}

void free_update(Update *update)
{
    free(update);
}

void free_install(Install *install)
{
    free(install);
}

void free_uninstall(Uninstall *uninstall)
{
    free(uninstall);
}

void free_operation(Operation *operation)
{
    switch (operation->type)
    {
    case UPDATE:
        free_update((Update *)operation->data);
        break;
    case INSTALL:
        free_install((Install *)operation->data);
        break;
    case UNINSTALL:
        free_uninstall((Uninstall *)operation->data);
        break;
    default:
        break;
    }
    free(operation);
}

char *operation_repr(Operation *operation)
{
    switch (operation->type)
    {
    case UPDATE:
        return update_repr((Update *)operation->data);
    case INSTALL:
        return install_repr((Install *)operation->data);
    case UNINSTALL:
        return uninstall_repr((Uninstall *)operation->data);
    default:
        return NULL;
    }
}

char *update_repr(Update *update)
{
    char *repr = (char *)malloc(256 * sizeof(char));
    snprintf(repr, 256, "Updating %s (%s) to %s (%s)",
             update->from_package->name, update->from_package->version,
             update->to_package->name, update->to_package->version);
    return repr;
}

char *install_repr(Install *install)
{
    char *repr = (char *)malloc(128 * sizeof(char));
    snprintf(repr, 128, "Installing %s (%s)",
             install->package->name, install->package->version);
    return repr;
}

char *uninstall_repr(Uninstall *uninstall)
{
    char *repr = (char *)malloc(128 * sizeof(char));
    snprintf(repr, 128, "Uninstalling %s (%s)",
             uninstall->package->name, uninstall->package->version);
    return repr;
}
