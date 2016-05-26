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

#define MAX_RESPONSE_SIZE 262144
#define PORT "3490"
#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

int response(int fd, const char *header, const char *contentType, const void *body, int contentLength)
{
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%a, %d %b %Y %H:%M:%S %Z", localTime);

    char response[MAX_RESPONSE_SIZE];
    int responseLength = snprintf(response, sizeof(response), "%s\nConnection: close\nDate: %s\nContent-Type: %s\nContent-Length: %d\n\n%s",
                                  header, timestamp, contentType, contentLength, body);

    int rv = send(fd, response, responseLength, 0);
    if (rv < 0)
    {
        perror("send");
    }
    return rv;
}

void getData(int fd)
{
    const char *data = "SOME DATA";
    const int contentLength = strlen(data);
    response(fd, "HTTP/1.1 200 OK", "text/plain", data, contentLength);
}

void getFile(int fd, struct Cache *cache, const char *path)
{
    char filePath[4096];
    struct Data *fileData;
    char *mimeType;

    if (strcmp(path, "/") == 0)
        snprintf(filePath, sizeof(filePath), "%s/index.html", SERVER_ROOT);
    else
        snprintf(filePath, sizeof(filePath), "%s%s", SERVER_ROOT, path);

    fileData = loadFile(filePath);

    if (fileData == NULL)
    {
        notFound(fd);
        return;
    }

    mimeType = getMimeType(filePath);
    cput(cache, filePath, mimeType, fileData->data, fileData->size);
    response(fd, "HTTP/1.1 200 OK", mimeType, fileData->data, fileData->size);

    free(mimeType);
    freeFile(fileData);
}

char *findBodyStart(char *header) {}

void notFound(int fd)
{
    const char *filePath = "./serverfiles/404.html";
    struct Data *fileData;
    char *mimeType;

    fileData = loadFile(filePath);

    if (fileData == NULL)
    {
        fprintf(stderr, "Cannot find system 404 file\n");
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

    if (receivedBytes < 0)
    {
        perror("recv");
        return;
    }

    char method[256], path[16384];
    sscanf(request, "%s %s", method, path);

    if (strcmp("/data", path) == 0)
    {
        getData(fd);
        return;
    }

    struct CacheEntry *entry = cget(cache, path);

    if (entry == NULL)
    {
        notFound(fd);
    }
    else if (strcmp("GET", method) == 0)
    {
        getFile(fd, cache, path);
    }
    else if (strcmp("POST", method) == 0)
    {
        // Handle POST request
    }
    else
    {
        notFound(fd);
    }

    free(entry);
}

int main(void)
{
    struct sockaddr_storage addr;
    char s[INET6_ADDRSTRLEN];
    struct Cache *cache = createCache(10, 0);
    int listenfd = getSocketListner(PORT);

    if (listenfd < 0)
    {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(1);
    }

    printf("webserver: waiting for connections on port %s...\n", PORT);

    while (1)
    {
        socklen_t sinSize = sizeof addr;
        int newfd = accept(listenfd, (struct sockaddr *)&addr, &sinSize);

        if (newfd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(addr.ss_family, getInAddr((struct sockaddr *)&addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        request(newfd, cache);
        close(newfd);
    }

    return 0;
}