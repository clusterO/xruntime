#ifndef _MIME_HEADER_
#define _MIME_HEADER_

#include <string.h>
#include <ctype.h>

#define DEFAULT_MIME_TYPE "application/octet-stream"

extern char *getMimeType(char *fileName);

#endif