#ifndef _REQUEST_HEADER_
#define _REQUEST_HEADER_

#include <stdio.h>

#include "../file.h"

#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

extern struct Data *getData(int fd, struct Cache *cache, const char *path, char *mimeType);
extern struct Data *notFound(int fd);
extern char *findBodyStart(char *header);

void get(const char *url, const char **headers, const char **params);
void post(const char *url, const char *data, const char **headers, const char **params);
void put(const char *url, const char *data, const char **headers, const char **params);
void patch(const char *url, const char *data, const char **headers, const char **params);
void delete(const char *url, const char **headers, const char **params);
void head(const char *url, const char **headers, const char **params);
void options(const char *url, const char **headers, const char **params);
void set_headers(const char **headers);
void set_params(const char **params);
void set_data(const char *data);
void set_auth(const char *username, const char *password);
void set_timeout(int timeout);
void add_header(const char *name, const char *value);
void add_param(const char *name, const char *value);
void add_data(const char *name, const char *value);
void get_response();

#endif