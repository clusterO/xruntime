#include "server.h"

int response(int fd, const char *header, const char *contentType, const char *body, int contentLength)
{
    // header construction

    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%a, %d %b %Y %H:%M:%S %Z", localTime);

    char response[MAX_RESPONSE_SIZE];
    int responseLength = snprintf(response, sizeof(response), "%s\nConnection: close\nDate: %s\nContent-Type: %s\nContent-Length: %d\n\n%s",
                                  header, timestamp, contentType, contentLength, body);

    int rv = send(fd, response, responseLength, 0);
    if (rv < 0)
        perror("send");

    return rv;
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

    if (0) // Reading from cache - OFF
    {
        struct CacheEntry *entry = cget(cache, path);

        if (entry != NULL)
        {
            char *body = "<h1>hello world¡</h1>";
            int res = response(fd, "HTTP/1.1 200 OK", "text/plain", body, sizeof(char) * strlen(body));
            return;
        }

        free(entry);
    }

    char *mimeType;

    // should be handled by a router
    if (strcmp("GET", method) == 0)
    {
        struct Data *fileData = getData(fd, cache, path, mimeType);

        if (fileData == NULL)
        {
            fileData = notFound(fd);
            response(fd, "HTTP/1.1 404 NOT FOUND", mimeType, fileData->data, fileData->size);
        }
        else
            response(fd, "HTTP/1.1 200 OK", mimeType, fileData->data, fileData->size);
    }
    else if (strcmp("POST", method) == 0)
    {
        // handle POST request
    }
    else // complete other HTTP request methods OPTIONS, DELETE, and TRACE...
    {
        // handle the requests
        mimeType = "application/json";
        char *body = "{'data': 'error'}";
        response(fd, "HTTP/1.1 404 NOT FOUND", mimeType, body, sizeof(char) * strlen(body));
    }
}

// dynamic configuration as arguments
int server()
{
    struct sockaddr_storage addr;
    char s[INET6_ADDRSTRLEN];
    int listenfd = getSocketListner(PORT);

    struct Cache *cache = createCache(10, 0); // hashSize DEFAULT_SIZE 128

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

// rename variables and functions
// turn this to a lib/package/module
int main(void)
{
    server();
}