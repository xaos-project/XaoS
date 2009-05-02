
#ifndef UI_WIN32_H
#include <windows.h>
#include <ui.h>
extern int directX;
extern CONST char *helptopic;
extern HWND hWnd;
extern HINSTANCE hInstance;
HMENU win32_createrootmenu(void);
void win32_pressed(int id);
void win32_dorootmenu(struct uih_context *uih, CONST char *name);
void win32_enabledisable(struct uih_context *uih, CONST char *name);
void win32_menu(struct uih_context *c, CONST char *name);
void win32_uninitializewindows(void);
void win32_dialog(struct uih_context *uih, CONST char *name);
void win32_genresources(struct uih_context *uih);
void win32_help(struct uih_context *uih, CONST char *name);
void CenterWindow(HWND hwndChild, HWND hwndParent);
void AboutBox(void);
#endif
