#ifndef _ROUTER_HEADER_
#define _ROUTER_HEADER_

#include <stdlib.h>

// HTTP methods
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_METHOD_UNKNOWN
} HttpMethod;

// Route handler function type
typedef void (*RouteHandler)(int client_fd, const char *path, const char *method, void *user_data);

// Route structure
typedef struct Route {
    char *path;
    HttpMethod method;
    RouteHandler handler;
    void *user_data;
    struct Route *next;
} Route;

// Router structure
typedef struct {
    Route *routes;
} Router;

/**
 * Create a new router
 */
Router* createRouter();

/**
 * Free the router and all its routes
 */
void freeRouter(Router *router);

/**
 * Add a route to the router
 */
int addRoute(Router *router, const char *path, HttpMethod method, RouteHandler handler, void *user_data);

/**
 * Find a route matching the given path and method
 */
Route* findRoute(Router *router, const char *path, HttpMethod method);

/**
 * Convert string to HttpMethod
 */
HttpMethod stringToHttpMethod(const char *method);

/**
 * Convert HttpMethod to string
 */
const char* httpMethodToString(HttpMethod method);

#endif