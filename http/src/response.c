#include "response.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Create a new Response structure
 */
Response* createResponse()
{
    Response *response = (Response *)malloc(sizeof(Response));
    if (response == NULL) {
        return NULL;
    }
    
    response->status = 200;
    response->content = NULL;
    response->contentLength = 0;
    response->contentType = strdup("text/html");
    
    return response;
}

/**
 * Free the memory allocated for a Response structure
 */
void freeResponse(Response *response)
{
    if (response == NULL) {
        return;
    }
    
    free(response->content);
    free(response->contentType);
    free(response);
}

/**
 * Set the HTTP status code for the response
 */
void setResponseStatus(Response *response, int status)
{
    if (response == NULL) {
        return;
    }
    
    response->status = status;
}

/**
 * Set the content body for the response
 */
void setResponseContent(Response *response, const char *content)
{
    if (response == NULL) {
        return;
    }
    
    free(response->content);
    response->content = content ? strdup(content) : NULL;
    response->contentLength = content ? strlen(content) : 0;
}

/**
 * Set the content length for the response
 */
void setResponseContentLength(Response *response, size_t contentLength)
{
    if (response == NULL) {
        return;
    }
    
    response->contentLength = contentLength;
}

/**
 * Set the content type for the response
 */
void setResponseContentType(Response *response, const char *contentType)
{
    if (response == NULL) {
        return;
    }
    
    free(response->contentType);
    response->contentType = contentType ? strdup(contentType) : NULL;
}

/**
 * Get the HTTP status code from the response
 */
int getResponseStatus(const Response *response)
{
    return response ? response->status : 0;
}

/**
 * Get the content body from the response
 */
const char* getResponseContent(const Response *response)
{
    return response ? response->content : NULL;
}

/**
 * Get the content length from the response
 */
size_t getResponseContentLength(const Response *response)
{
    return response ? response->contentLength : 0;
}

/**
 * Get the content type from the response
 */
const char* getResponseContentType(const Response *response)
{
    return response ? response->contentType : NULL;
}
