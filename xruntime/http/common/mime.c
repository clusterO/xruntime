#include "mime.h"

char *strlwr(char *str)
{
    for (char *p = str; *p != '\0'; p++)
        *p = tolower(*p);

    return str;
}

char *getMimeType(char *filename)
{
    char *ext = strrchr(filename, '.');
    if (ext == NULL)
        return DEFAULT_MIME_TYPE;

    ext++;
    strlwr(ext);

    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0)
        return "text/html";
    if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0)
        return "image/jpg";
    if (strcmp(ext, "css") == 0)
        return "text/css";
    if (strcmp(ext, "js") == 0)
        return "application/javascript";
    if (strcmp(ext, "json") == 0)
        return "application/json";
    if (strcmp(ext, "txt") == 0)
        return "text/plain";
    if (strcmp(ext, "gif") == 0)
        return "image/gif";
    if (strcmp(ext, "png") == 0)
        return "image/png";

    return DEFAULT_MIME_TYPE;
}