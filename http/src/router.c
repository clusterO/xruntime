#include "router.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

/**
 * Create a new router
 */
Router* createRouter()
{
    Router *router = (Router *)malloc(sizeof(Router));
    if (router == NULL) {
        return NULL;
    }
    
    router->routes = NULL;
    return router;
}

/**
 * Free the router and all its routes
 */
void freeRouter(Router *router)
{
    if (router == NULL) {
        return;
    }
    
    Route *current = router->routes;
    while (current != NULL) {
        Route *next = current->next;
        free(current->path);
        free(current);
        current = next;
    }
    
    free(router);
}

/**
 * Add a route to the router
 */
int addRoute(Router *router, const char *path, HttpMethod method, RouteHandler handler, void *user_data)
{
    if (router == NULL || path == NULL || handler == NULL) {
        return -1;
    }
    
    Route *route = (Route *)malloc(sizeof(Route));
    if (route == NULL) {
        return -1;
    }
    
    route->path = strdup(path);
    route->method = method;
    route->handler = handler;
    route->user_data = user_data;
    route->next = NULL;
    
    // Add to the beginning of the list
    if (router->routes == NULL) {
        router->routes = route;
    } else {
        route->next = router->routes;
        router->routes = route;
    }
    
    return 0;
}

/**
 * Simple path matching - exact match for now
 * TODO: Implement pattern matching (e.g., /user/:id)
 */
static int pathMatches(const char *routePath, const char *requestPath)
{
    return strcmp(routePath, requestPath) == 0;
}

/**
 * Find a route matching the given path and method
 */
Route* findRoute(Router *router, const char *path, HttpMethod method)
{
    if (router == NULL || path == NULL) {
        return NULL;
    }
    
    Route *current = router->routes;
    while (current != NULL) {
        if (current->method == method && pathMatches(current->path, path)) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * Convert string to HttpMethod
 */
HttpMethod stringToHttpMethod(const char *method)
{
    if (method == NULL) {
        return HTTP_METHOD_UNKNOWN;
    }
    
    if (strcasecmp(method, "GET") == 0) {
        return HTTP_GET;
    } else if (strcasecmp(method, "POST") == 0) {
        return HTTP_POST;
    } else if (strcasecmp(method, "PUT") == 0) {
        return HTTP_PUT;
    } else if (strcasecmp(method, "DELETE") == 0) {
        return HTTP_DELETE;
    } else if (strcasecmp(method, "PATCH") == 0) {
        return HTTP_PATCH;
    } else if (strcasecmp(method, "HEAD") == 0) {
        return HTTP_HEAD;
    } else if (strcasecmp(method, "OPTIONS") == 0) {
        return HTTP_OPTIONS;
    }
    
    return HTTP_METHOD_UNKNOWN;
}

/**
 * Convert HttpMethod to string
 */
const char* httpMethodToString(HttpMethod method)
{
    switch (method) {
        case HTTP_GET:
            return "GET";
        case HTTP_POST:
            return "POST";
        case HTTP_PUT:
            return "PUT";
        case HTTP_DELETE:
            return "DELETE";
        case HTTP_PATCH:
            return "PATCH";
        case HTTP_HEAD:
            return "HEAD";
        case HTTP_OPTIONS:
            return "OPTIONS";
        default:
            return "UNKNOWN";
    }
}