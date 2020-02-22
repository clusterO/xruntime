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
    time_t time = time(NULL);
    struct tm *localTime = localtime(&time);
    char *timestamp = asctime(localTime);

    int responseLength = sprintf(response, "%\nConnection: close\nDate %s\nContent-Type: %s\nContent-Length: %d\n\n%s", header, timestamp, contentType, contentLength, body);
    
    int rv = send(fd, response, responseLength, 0);
    if(rv < 0) perror("send");
    return rv;
}

void getData(int fd) 
{
    char *data = "SOME DATA";
    response(fd, "HTTP/1.1 OK", "text/plain", data, sizeof data);
}

char getFile(int fd, struct Cache *cache, char *path) 
{
    char filePath[4096];
    struct Data *fileData;
    char *mimeType;

    if(strcmp(path, "/") == 0) 
        sprintf(filePath, sizeof filePath, "%s/index.html", SERVER_ROOT);
    else
        sprintf(filePath, sizeof filePath, "%s%s", SERVER_ROOT, path);
    
    fileData = loadFilr(filePath);

    if(filePath == NULL)
        notFount(fd);
    else {
        mimeType = getMimeType(filePath);
        cput(cache, filePath, mimeType, fileData->data, fileData->size);
        response(fd, "HTTP/1.1 OK", mimeType, fileData->data, fileData->size);
        free(mimeType);
    }

    ffree(fileData);
    free(filePath);
}

char *findBodyStart(char *header) {}


void notFound(int fd)
{
    char filePath[4096];
    struct Data *fileData;
    char *mimeType;

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

    char method[1024], path[16384];
    //Implement
    sscanf(request, "%s %s", method, path);
    
    struct cacheEntry *entry = cget(cache, path);
    if(entry == NULL) 
        response(fd, "HTTP/1.1 OK", entry->contentType, entry->contentLength);
    else {
        if(strcmp("/data", path) == 0) getData(fd);
        else if (strcmp("GET", method) == 0) getFile(fd, cache, path);
        else if (strcmp("POST", method) == 0) return;
        else notFound(fd);
    } 
}

int main(void) 
{
    int newfd;
    struct sockaddr_storage addr;
    char s[INET6_ADDRSTRLEN];
    struct Cache *cache = createCache(10, 0);
    int listenfd = getSocketListner(PORT);

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

        inet_ntop(addr.ss_family, getInAddr((struct sockaddr *)&addr), s, sizeof s);
        printf("server: got connection from %S\n", s);
        
        request(newfd, cache);
        close(newfd);
    }
    
    return 0;
}






























































