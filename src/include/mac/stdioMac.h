// stdioMac.h

#include <stdio.h>

FILE *fopenMac (const char *, const char *, OSType creator, OSType fileType);
size_t fwriteMac (const void *, size_t, size_t, FILE *);
int fputcMac (int, FILE *);
int fcloseMac (FILE *);

// Convert Mac filespec to UNIX/DOS-style pathspec.
unsigned char *FilespecFromSFReply (SFReply * sf, unsigned char *result);
