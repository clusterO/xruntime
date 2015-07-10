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

#define PORT "3490"
#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

int response(int fd, char *header, char *contentType, void *body, int contentLength)
{
    const int maxResponseSize = 262144;
    char response[maxResponseSize];

    //Implementation
    
    int rv = send(fd, response, responseLength, 0);
    if(rv < 0) perror("send");
    return rv;
}

void getNumber(int fd) {}
char getFile(int fd, struct Cache *cache, char *path) {}
char *findBodyStart(char *header) {}


void notFound(int fd)
{
    char filePath[4096];
    struct Data *fileData;
    char *MimeType;

    snprintf(filePath, sizeof filePath, "%s/404.html", SERVER_FILES);
    fileData = loadFile(filePath);

    if(fileData == NULL) {
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }

    mimeType = getMimeType(filePath);
    response(fd, "HTTP/1.1 404 NOT FOUND", mimeType, fileData->data, fileData->size);
    freeFile(fileData);
}

void request(int fd, struct Cache *cache)
{
    const int requestBufferSize = 65536;
    char request[requestBufferSize];
    int receivedBytes = recv(fd, request, requestBufferSize - 1, 0);

    if(receivedBytes < 0) {
        perror("recv");
        return;
    }

    //Check method & serve file
}

int main(void) 
{
    int newfd;
    struct sockAddrStorage addr;
    char s[INET6_ADDRSTRLEN];
    struct Cache *cache = createCache(10, 0);
    int listenfd = getListener(PORT);

    if(listenfd < 0) {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(1);
    }

    printf("webserver: waiting for connections on port %s...\n", PORT);

    while(1) {
        socklen_t sinSize = sizeof addr;
        newfd = accept(listenfd, (struct sockaddr *)&addr, &sinSize);

        if(newfd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(addr.ss_family, get_in_addr((struct sockaddr *)&addr), s, sizeof s);
        printf("server: got connection from %S\n", s);
        
        request(newfd, cache);
        close(newfd);
    }
    
    return 0;
}






























































