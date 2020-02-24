#ifndef CPM_CACHE_HEADER
#define CPM_CACHE_HEADER

int ccCreate(time_t t);
int ccInit();
const char *ccPath();
int ccConfigExist(char *author, char *name, char *version);
char *ccGetConfig(char *author, char *name, char *version);
int ccSetConfig(char *author, char *name, char *version, char *content);
int ccDeleteConfig(char *author, char *name, char *version);
int ccSearchExist();
char *ccGetSearch();
int ccSetSearch(char *content);
int ccDeleteSearch();
int ccPackageExist(char *author, char *name, char *version);
int ccPackageExpired(char *author, char *name, char * version);
int ccLoadPackage(char *author, char *name, char *version, char *path);
int ccSetPackage(char *author, char *name, char *version, char *path);
int ccDeletePackage(char *author, char *name, char *version);

