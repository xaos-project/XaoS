#ifndef XERROR_H
#define XERROR_H
#ifdef __cplusplus
extern "C"
{
#endif
  void x_message (CONST char *text, ...);
  void x_error (CONST char *text, ...);
  void x_fatalerror (CONST char *text, ...) NORETURN;
#ifdef __cplusplus
}
#endif
#endif
