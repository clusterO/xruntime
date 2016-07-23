#ifndef _HTTP_SERVER_HEADER_
#define _HTTP_SERVER_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>

#include "net.h"
#include "file.h"
#include "mime.h"
#include "cache.h"
#include "hashtable.h"
#include "helpers/request.h"

#define MAX_RESPONSE_SIZE 262144
#define PORT "3490"

extern int response(int fd, const char *header, const char *contentType, const char *body, int contentLength);
extern void request(int fd, struct Cache *cache);
extern int server();
// request builder
// response parser

#endif