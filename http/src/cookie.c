#define _GNU_SOURCE
#include "cookie.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Create a new cookie with the specified name and value
 */
Cookie* createCookie(const char *name, const char *value)
{
    Cookie *cookie = (Cookie *)malloc(sizeof(Cookie));
    if (cookie == NULL) {
        return NULL;
    }
    
    cookie->name = name ? strdup(name) : NULL;
    cookie->value = value ? strdup(value) : NULL;
    cookie->domain = NULL;
    cookie->path = NULL;
    cookie->expires = 0;
    cookie->secure = 0;
    cookie->httpOnly = 0;
    
    return cookie;
}

/**
 * Free the memory allocated for a cookie
 */
void freeCookie(Cookie *cookie)
{
    if (cookie == NULL) {
        return;
    }
    
    free(cookie->name);
    free(cookie->value);
    free(cookie->domain);
    free(cookie->path);
    free(cookie);
}

/**
 * Set the domain for the cookie
 */
void setCookieDomain(Cookie *cookie, const char *domain)
{
    if (cookie == NULL) {
        return;
    }
    
    free(cookie->domain);
    cookie->domain = domain ? strdup(domain) : NULL;
}

/**
 * Set the path for the cookie
 */
void setCookiePath(Cookie *cookie, const char *path)
{
    if (cookie == NULL) {
        return;
    }
    
    free(cookie->path);
    cookie->path = path ? strdup(path) : NULL;
}

/**
 * Set the expiration time for the cookie
 */
void setCookieExpires(Cookie *cookie, time_t expires)
{
    if (cookie == NULL) {
        return;
    }
    
    cookie->expires = expires;
}

/**
 * Set the secure flag for the cookie
 */
void setCookieSecure(Cookie *cookie, int secure)
{
    if (cookie == NULL) {
        return;
    }
    
    cookie->secure = secure;
}

/**
 * Set the HttpOnly flag for the cookie
 */
void setCookieHttpOnly(Cookie *cookie, int httpOnly)
{
    if (cookie == NULL) {
        return;
    }
    
    cookie->httpOnly = httpOnly;
}

/**
 * Get the cookie value
 */
const char* getCookieValue(const Cookie *cookie)
{
    return cookie ? cookie->value : NULL;
}

/**
 * Get the cookie domain
 */
const char* getCookieDomain(const Cookie *cookie)
{
    return cookie ? cookie->domain : NULL;
}

/**
 * Get the cookie path
 */
const char* getCookiePath(const Cookie *cookie)
{
    return cookie ? cookie->path : NULL;
}

/**
 * Get the cookie expiration time
 */
time_t getCookieExpires(const Cookie *cookie)
{
    return cookie ? cookie->expires : 0;
}

/**
 * Check if the cookie is secure
 */
int isCookieSecure(const Cookie *cookie)
{
    return cookie ? cookie->secure : 0;
}

/**
 * Check if the cookie is HttpOnly
 */
int isCookieHttpOnly(const Cookie *cookie)
{
    return cookie ? cookie->httpOnly : 0;
}

/**
 * Serialize a cookie to a string format suitable for HTTP headers
 */
char* serializeCookie(const Cookie *cookie)
{
    if (cookie == NULL || cookie->name == NULL || cookie->value == NULL) {
        return NULL;
    }
    
    // Calculate required buffer size
    size_t size = strlen(cookie->name) + strlen(cookie->value) + 4; // name=value;
    
    if (cookie->domain) {
        size += strlen("; Domain=") + strlen(cookie->domain);
    }
    if (cookie->path) {
        size += strlen("; Path=") + strlen(cookie->path);
    }
    if (cookie->expires > 0) {
        size += strlen("; Expires=") + 30; // RFC 1123 date format
    }
    if (cookie->secure) {
        size += strlen("; Secure");
    }
    if (cookie->httpOnly) {
        size += strlen("; HttpOnly");
    }
    
    char *serialized = (char *)malloc(size + 1);
    if (serialized == NULL) {
        return NULL;
    }
    
    strcpy(serialized, cookie->name);
    strcat(serialized, "=");
    strcat(serialized, cookie->value);
    
    if (cookie->domain) {
        strcat(serialized, "; Domain=");
        strcat(serialized, cookie->domain);
    }
    if (cookie->path) {
        strcat(serialized, "; Path=");
        strcat(serialized, cookie->path);
    }
    if (cookie->expires > 0) {
        char date_str[30];
        strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cookie->expires));
        strcat(serialized, "; Expires=");
        strcat(serialized, date_str);
    }
    if (cookie->secure) {
        strcat(serialized, "; Secure");
    }
    if (cookie->httpOnly) {
        strcat(serialized, "; HttpOnly");
    }
    
    return serialized;
}

/**
 * Deserialize a cookie from a string format
 */
Cookie* deserializeCookie(const char *serialized)
{
    if (serialized == NULL) {
        return NULL;
    }
    
    // Simple parser for cookie string
    char *copy = strdup(serialized);
    if (copy == NULL) {
        return NULL;
    }
    
    char *name = strtok(copy, "=");
    char *value = strtok(NULL, ";");
    
    if (name == NULL || value == NULL) {
        free(copy);
        return NULL;
    }
    
    // Trim whitespace
    while (*value == ' ') value++;
    
    Cookie *cookie = createCookie(name, value);
    free(copy);
    
    // Parse additional attributes
    char *token = strtok(NULL, ";");
    while (token != NULL) {
        while (*token == ' ') token++;
        
        if (strncmp(token, "Domain=", 7) == 0) {
            setCookieDomain(cookie, token + 7);
        } else if (strncmp(token, "Path=", 5) == 0) {
            setCookiePath(cookie, token + 5);
        } else if (strncmp(token, "Expires=", 8) == 0) {
            // Parse date (simplified - would need full RFC 1123 parser)
            struct tm tm = {0};
            strptime(token + 8, "%a, %d %b %Y %H:%M:%S GMT", &tm);
            setCookieExpires(cookie, mktime(&tm));
        } else if (strcmp(token, "Secure") == 0) {
            setCookieSecure(cookie, 1);
        } else if (strcmp(token, "HttpOnly") == 0) {
            setCookieHttpOnly(cookie, 1);
        }
        
        token = strtok(NULL, ";");
    }
    
    return cookie;
}
