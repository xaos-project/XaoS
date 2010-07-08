#ifndef XERROR_H
#define XERROR_H
#ifdef __cplusplus
extern "C" {
#endif
void x_message(const char *text, ...);
void x_error(const char *text, ...);
void x_fatalerror(const char *text, ...) NORETURN;
#ifdef __cplusplus
}
#endif
#endif
