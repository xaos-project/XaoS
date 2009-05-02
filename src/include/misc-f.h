#ifndef IUTIL_H
#define IUTIL_H
#ifdef __cplusplus
extern "C" {
#endif
    struct image;

    CONST char *writepng(FILE * file, CONST struct image *image);
    void XaoS_srandom(unsigned int x);
    long int XaoS_random(void);
    char *mystrdup(CONST char *);

#ifdef __cplusplus
}
#endif
#endif
