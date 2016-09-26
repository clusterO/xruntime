#ifndef _NET_HEADER_
#define _NET_HEADER_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 10

void *getInAddr(struct sockaddr *sa);
int getSocketListner(char *port);

#endif