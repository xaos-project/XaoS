#ifndef EXPRTK_H
#define EXPRTK_H
#include "aconfig.h"
#ifdef USE_EXPRTK
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double re;
    double im;
} exprtk_cmplx;

void exprtk_setexpr(const char* expr);
void exprtk_init();
exprtk_cmplx exprtk_eval(double zre, double zim, double cre, double cim);
#ifdef __cplusplus
}
#endif
#endif
#endif
