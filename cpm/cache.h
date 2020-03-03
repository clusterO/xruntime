#ifndef CPM_CACHE_HEADER
#define CPM_CACHE_HEADER

extern int ccCreate(time_t);
extern int ccInit();
extern const char *ccMetaPath();
extern const char *ccPath();
extern int ccConfigExists(char *, char *, char *);
extern char *ccGetConfig(char *, char *, char *);
extern int ccSetConfig(char *, char *, char *, char *);
extern int ccDeleteConfig(char *, char *, char *);
extern int ccSearchExist();
extern char *ccGetSearch();
extern int ccSetSearch(char *);
extern int ccDeleteSearch();
extern int ccPackageExist(char *, char *, char *);
extern int ccPackageExpired(char *, char *, char * );
extern int ccLoadPackage(char *, char *, char *, char *);
extern int ccSetPackage(char *, char *, char *, char *);
extern int ccDeletePackage(char *, char *, char *);

#endif
