#ifndef CPM_PACKAGE_HEADER
#define CPM_PACKAGE_HEADER

#include <curl/curl.h>
#include "../libs/list.h"

typedef struct
{
    char *name;
    char *author;
    char *version;
} Dependency;

typedef struct
{
    char *name;
    char *author;
    char *description;
    char *install;
    char *configure;
    char *json;
    char *license;
    char *repo;
    char *reponame;
    char *url;
    char *version;
    char *makefile;
    char *filename;
    char *flags;
    char *prefix;
    list_t *dependencies;
    list_t *development;
    list_t *src;
    void *user;
    unsigned int refs;
} Package;

typedef struct
{
    int skipCache;
    int force;
    int global;
    char *prefix;
    int concurrency;
    char *token;
} Options;

CURLSH *cpcs;
extern void setPkgOptions(Options);
extern Package *newPkg(const char *, int);
extern Package *newPkgSlug(const char *, int);
extern Package *loadPkg(const char *, int);
extern Package *loadManifest(int);
extern char *pkgUrl(const char *, const char *, const char *);
extern char *pkgRepoUrl(const char *repo, const char *version);
extern char *pkgVersion(const char *);
extern char *pkgAuthor(const char *);
extern char *pkgName(const char *);
extern Dependency *newDeps(const char *, const char *);
extern int installExe(Package *pkg, char *path, int verbose);
extern int installPkg(Package *, const char *, int);
extern int installDeps(Package *, const char *, int);
extern int installDev(Package *, const char *, int);
extern void freePkg(Package *);
extern void freeDeps(Dependency *);
extern void cleanPkgs();

#endif
