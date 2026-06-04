#include "server.h"
#include <stdlib.h>
#include <string.h>

/**
 * Task structure for passing client connection data to worker threads
 */
typedef struct {
    int client_fd;
    struct Cache *cache;
    char client_ip[INET6_ADDRSTRLEN];
} ClientTask;

/**
 * Worker function to handle a client request in a thread pool
 * This function is called by the thread pool for each incoming connection
 */
static void handle_client_task(void *arg)
{
    ClientTask *task = (ClientTask *)arg;
    
    printf("server: handling connection from %s (fd=%d)\n", task->client_ip, task->client_fd);
    
    // Process the request
    request(task->client_fd, task->cache);
    
    // Close the client connection
    close(task->client_fd);
    
    // Free the task structure
    free(task);
}

/**
 * Send an HTTP response to the client
 * 
 * @param fd Client socket file descriptor
 * @param header HTTP status line (e.g., "HTTP/1.1 200 OK")
 * @param contentType MIME type of the response body
 * @param body Response body content
 * @param contentLength Length of the response body
 * @return Number of bytes sent, or -1 on error
 */
int response(int fd, const char *header, const char *contentType, const char *body, int contentLength)
{
    // Construct HTTP response headers
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

/**
 * Handle an incoming HTTP request
 * Parses the request method and path, then routes to appropriate handler
 * 
 * @param fd Client socket file descriptor
 * @param cache Cache structure for caching responses
 */
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

    // Parse HTTP method and path from request
    char method[256], path[16384];
    sscanf(request, "%s %s", method, path);

    // Cache lookup (currently disabled)
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

    char mimeType[256] = {0};

    // Route based on HTTP method
    // TODO: Integrate router for proper routing
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
        // TODO: Implement POST request handling
        char *body = "{\"message\": \"POST not yet implemented\"}";
        response(fd, "HTTP/1.1 501 Not Implemented", "application/json", body, strlen(body));
    }
    else // Handle other HTTP methods: OPTIONS, DELETE, PUT, PATCH, HEAD, etc.
    {
        // TODO: Implement other HTTP methods
        char *body = "{\"error\": \"Method not supported\"}";
        response(fd, "HTTP/1.1 405 Method Not Allowed", "application/json", body, strlen(body));
    }
}

/**
 * Start the HTTP server with thread pool for concurrent request handling
 * 
 * @param thread_count Number of worker threads in the thread pool
 * @return 0 on success, -1 on failure
 */
int server(int thread_count)
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

    // Create thread pool for concurrent request handling
    // Default to 4 threads if not specified
    if (thread_count <= 0)
        thread_count = 4;

    ThreadPool *thread_pool = thread_pool_create(thread_count);
    if (thread_pool == NULL)
    {
        fprintf(stderr, "webserver: fatal error creating thread pool\n");
        exit(1);
    }

    printf("webserver: waiting for connections on port %s...\n", PORT);
    printf("webserver: using %d worker threads\n", thread_count);

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

        // Create a task for this client connection
        ClientTask *task = (ClientTask *)malloc(sizeof(ClientTask));
        if (task == NULL)
        {
            perror("malloc");
            close(newfd);
            continue;
        }

        task->client_fd = newfd;
        task->cache = cache;
        strncpy(task->client_ip, s, INET6_ADDRSTRLEN);

        // Submit the task to the thread pool
        if (thread_pool_submit(thread_pool, handle_client_task, task) != 0)
        {
            fprintf(stderr, "server: failed to submit task to thread pool\n");
            free(task);
            close(newfd);
        }
    }

    // Cleanup (this code is unreachable in the current implementation)
    thread_pool_destroy(thread_pool);
    freeCache(cache);
    close(listenfd);

    return 0;
}

/**
 * Main entry point for the HTTP server
 * Accepts optional command-line argument for thread count
 * Usage: ./server [thread_count]
 */
int main(int argc, char **argv)
{
    int thread_count = 4; // Default to 4 worker threads
    
    if (argc > 1)
    {
        thread_count = atoi(argv[1]);
        if (thread_count <= 0)
        {
            fprintf(stderr, "Invalid thread count, using default: 4\n");
            thread_count = 4;
        }
    }
    
    return server(thread_count);
}