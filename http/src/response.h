#ifndef _RESPONSE_HEADER_
#define _RESPONSE_HEADER_

typedef struct {
    int status;
    char *content;
    size_t contentLength;
    char *contentType;
    // Other response properties as needed...
} Response;

Response* createResponse();
void freeResponse(Response *response);
void setResponseStatus(Response *response, int status);
void setResponseContent(Response *response, const char *content);
void setResponseContentLength(Response *response, size_t contentLength);
void setResponseContentType(Response *response, const char *contentType);
int getResponseStatus(const Response *response);
const char* getResponseContent(const Response *response);
size_t getResponseContentLength(const Response *response);
const char* getResponseContentType(const Response *response);

#endif