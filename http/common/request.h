#ifndef _REQUEST_HEADER_
#define _REQUEST_HEADER_

#include <stdio.h>

#include "../file.h"

#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

extern struct Data *getData(int fd, struct Cache *cache, const char *path, char *mimeType);
extern char *findBodyStart(char *header);
extern struct Data *notFound(int fd);

#endif