#ifndef _COOKIE_HEADER_
#define _COOKIE_HEADER_

typedef struct {
    char *name;
    char *value;
    char *domain;
    char *path;
    time_t expires;
    int secure;
    int httpOnly;
} Cookie;

Cookie* createCookie(const char *name, const char *value);
void freeCookie(Cookie *cookie);
void setCookieDomain(Cookie *cookie, const char *domain);
void setCookiePath(Cookie *cookie, const char *path);
void setCookieExpires(Cookie *cookie, time_t expires);
void setCookieSecure(Cookie *cookie, int secure);
void setCookieHttpOnly(Cookie *cookie, int httpOnly);
const char* getCookieValue(const Cookie *cookie);
const char* getCookieDomain(const Cookie *cookie);
const char* getCookiePath(const Cookie *cookie);
time_t getCookieExpires(const Cookie *cookie);
int isCookieSecure(const Cookie *cookie);
int isCookieHttpOnly(const Cookie *cookie);
char* serializeCookie(const Cookie *cookie);
Cookie* deserializeCookie(const char *serialized);

#endif