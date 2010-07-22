/* This file actually implements three drivers (win32 and DirectX 
   windowed/fullscreen) drivers, because they have a lot of common stuff. */
#include <config.h>
#ifdef WIN32_DRIVER
#define _WIN32_WINNT 0x0501 /* Enable access to Windows XP APIs */
#include <windows.h>
#ifdef HTML_HELP
#include <htmlhelp.h>
#endif
#ifdef DDRAW_DRIVER
#include <ddraw.h>
#endif
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <xmenu.h>
#include <xerror.h>
#include "ui.h"
#include "ui_win32.h"
#include "resource.h"
#include "xio.h"
#include <cursor.h>

#define MAXRESOLUTIONS 256
#define DXWIDTH 320
#define DXHEIGHT 200
#define DXBPP 8
#define WWIDTH (480+4*48)
#define WHEIGHT (360+4*36)

#define DXFULLSCREEN 1
#define DXWINDOWED   2

#define MOUSEWIDTH 16
#define MOUSEHEIGHT 16

#ifdef HAVE_GETTEXT

#include <libintl.h>

#include <locale.h>

#else

#define gettext(STRING) STRING

#endif



HINSTANCE hInstance;
HWND hWnd;
CONST char *helptopic = "main";

struct ui_driver win32_driver;
struct ui_driver dxw_driver, dxf_driver;

static int initialized = 0, needredraw = 0;

static char *helpname = NULL;

/* This variables are used by drivers in initializetion */
static int windowpos;
static CONST char *size = "520x430";
static CONST char *dxsize = "320x200x8";


/* Display information */
static int displayX, displayY, bitDepth;
static int lineSize;
static int fontWidth, fontHeight;
static HPALETTE hPalette = NULL;
static char *buffer1;
static char *buffer2;
static int currentbuff = 0;
static BITMAPINFO *bmp = NULL;

/* Mouse */
static int mouseX, mouseY, mouseButtons = 0;
static int captured = 0, tmpcaptured = 0;

/* Keyboard state */
static int altPressed;
static int arrowsPressed;

/* WindowProc comunication variables */
static BOOL closeFlag;
static int resized = 0;
static int active = 1;

/* Curent class information */
static HICON hIcon;
static HICON hIconSm;
static HFONT hFont = NULL;

/* DirectX stuff */
#ifdef DDRAW_DRIVER
int directX = 0;
static LPDIRECTDRAWPALETTE dxPalette = NULL;
static LPDIRECTDRAW lpDD = NULL;
static LPDIRECTDRAW2 lpDD2 = NULL;
static LPDIRECTDRAWSURFACE lpSurfaces[2], BackSurface[2];
static DDSURFACEDESC surface[2];
static CONST char *mousepointer = mouse_pointer_data;
static char *storeddata = NULL;
static HMODULE hModule = NULL;
static int oldmouseX, oldmouseY;
#else
#define directX 0
#endif
static PUCHAR backpalette[256][4];
static HMODULE hModule2;
static RECT rcWindow;
static RECT rcViewport;
static RECT rcScreen;
static int MyHelpMsg;
/*clipboard*/
static WORD clipboard_format;

/* forward declarations */
#ifdef DDRAW_DRIVER
static void DeInitDD(void);
static void PaintDD(void);
static void UpdateMouseDD(void);
#endif
static void Paint(HDC hDC);
static void CalculateBITMAPINFO(void);
static void win32_display(void);
static void DeInitWindow(void);

#ifdef DDRAW_DRIVER
  /* FIXME: In windowed mode we don't support 8bpp yet! */
#define DXSUPPORTEDDEPTH(fullscreen,depth) \
  (!(depth < 8 || (!fullscreen && depth != 16 && depth !=24 && depth != 32)))

static char *store(char *data, int depth, int lpitch, int width,
		   int height, int xpos, int ypos)
{
    int d = depth / 8;
    char *store = malloc(d * MOUSEWIDTH * MOUSEHEIGHT);
    int y;
    if (xpos + MOUSEWIDTH > width)
	xpos = width - MOUSEWIDTH;
    if (ypos + MOUSEHEIGHT > height)
	ypos = height - MOUSEHEIGHT;
    if (xpos < 0)
	xpos = 0;
    if (ypos < 0)
	ypos = 0;
    for (y = 0; y < MOUSEHEIGHT; y++)
	memcpy(store + d * MOUSEWIDTH * y,
	       data + xpos * d + (ypos + y) * lpitch, MOUSEWIDTH * d);
    return store;
}

static void
restore(char *data, CONST char *store, int depth, int lpitch, int width,
	int height, int xpos, int ypos)
{
    int y;
    int d = depth / 8;
    if (xpos + MOUSEWIDTH > width)
	xpos = width - MOUSEWIDTH;
    if (ypos + MOUSEHEIGHT > height)
	ypos = height - MOUSEHEIGHT;
    if (xpos < 0)
	xpos = 0;
    if (ypos < 0)
	ypos = 0;
    for (y = 0; y < MOUSEHEIGHT; y++)
	memcpy(data + xpos * d + (ypos + y) * lpitch,
	       store + d * MOUSEWIDTH * y, MOUSEWIDTH * d);
}

static void
drawmouse(char *data, CONST char *mouse, int depth, int lpitch, int width,
	  int height, int xpos, int ypos)
{
    int x, y, z, c;
    int d = depth / 8;
    for (y = 0; y < MOUSEWIDTH; y++)
	for (x = 0; x < MOUSEWIDTH; x++)
	    if (mouse[x + MOUSEWIDTH * y] && x + xpos > 0
		&& (x + xpos) < width && y + ypos > 0
		&& y + ypos < height) {
		c = mouse[x + MOUSEWIDTH * y] == 2 ? (d ==
						      1 ? 1 : 255) : 0;
		for (z = 0; z < d; z++)
		    data[z + d * (x + xpos) + (y + ypos) * lpitch] = c;
	    }
}
#endif
static void getdimens(float *width, float *height)
{
    HDC hDC = GetDC(hWnd);
    *width = GetDeviceCaps(hDC, HORZSIZE) / 10.0;
    *height = GetDeviceCaps(hDC, VERTSIZE) / 10.0;
    if (*width > 100 || *width < 1)
	*width = 29.0;
    if (*height > 100 || *height < 1)
	*height = 21.0;
    ReleaseDC(hWnd, hDC);
}

static void getres(float *width, float *height)
{
    HDC hDC = GetDC(hWnd);
    *width = 2.54 / GetDeviceCaps(hDC, LOGPIXELSX);
    *height = 2.54 / GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(hWnd, hDC);
}

/******************************************************************************
                             Win32 driver helper routines
 */

static LRESULT CALLBACK WindowProc(HWND hWnd,	// handle to window
				   UINT uMsg,	// message identifier
				   WPARAM wParam,	// first message parameter
				   LPARAM lParam	// second message parameter
    )
{
    PAINTSTRUCT paintStruct;
    HDC hDC;
    if (uMsg == (unsigned int) MyHelpMsg) {
	win32_help(NULL, helptopic);
	return 0;
    }
    switch (uMsg) {
    case WM_COMMAND:
	win32_pressed(wParam);
	break;
    case WM_SIZE:
	// resize window
	if (directX == DXFULLSCREEN)
	    return 0;
	if (LOWORD(lParam) == 0 && HIWORD(lParam) == 0) {
	    active = 0;
	    break;
	}			/*Minimized window */
	active = 1;
	if (displayX != LOWORD(lParam) || displayY != HIWORD(lParam))
	    resized = 1;
	displayX = LOWORD(lParam);
	displayY = HIWORD(lParam);
	break;
    case WM_DISPLAYCHANGE:
	if (directX == DXFULLSCREEN)
	    return 0;
	mouseButtons = 0;
	resized = 1;
	hDC = GetDC(hWnd);
	bitDepth = GetDeviceCaps(hDC, BITSPIXEL);
	ReleaseDC(hWnd, hDC);
	break;
    case WM_CLOSE:
	// close window
	closeFlag = TRUE;
	return 0;
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDOWN:
	// handle mouse move and mouse buttons
	mouseButtons = wParam;
	if (!captured) {
	    if (mouseButtons && !tmpcaptured)
		SetCapture(hWnd), tmpcaptured = 1;
	    if (!mouseButtons && tmpcaptured)
		ReleaseCapture(), tmpcaptured = 0;
	}
	mouseX = (short) LOWORD(lParam);
	mouseY = (short) HIWORD(lParam);
#ifdef DDRAW_DRIVER
	if (directX == DXFULLSCREEN) {
	    POINT p;
	    GetCursorPos(&p);
	    mouseX = p.x;
	    mouseY = p.y;
	    UpdateMouseDD();
	}
#endif
	break;
    case WM_PAINT:
	// redraw screen
	if (directX == DXFULLSCREEN)
	    return 0;
	needredraw = 1;
	if (GetUpdateRect(hWnd, NULL, FALSE)) {
	    HDC hDC = BeginPaint(hWnd, &paintStruct);
	    if (hDC) {
#ifdef DDRAW_DRIVER
		if (directX)
		    PaintDD();
		else
#endif
		    Paint(hDC);
		EndPaint(hWnd, &paintStruct);
	    }
	}
	return 0;
    case WM_QUERYNEWPALETTE:
	// windows calls this when window is reactivated.
	if (directX == DXFULLSCREEN)
	    return 0;
	hDC = GetDC(hWnd);
#ifdef DDRAW_DRIVER
	if (directX == DXWINDOWED) {
	    if (dxPalette) {
		IDirectDrawSurface_SetPalette(lpSurfaces[0], dxPalette);
		IDirectDrawPalette_SetEntries(dxPalette, 0, 0, 255,
					      (PALETTEENTRY *)
					      backpalette);
	    }
	} else
#endif
	{
	    SelectPalette(hDC, hPalette, FALSE);
	    RealizePalette(hDC);
	}
	ReleaseDC(hWnd, hDC);
	return TRUE;
    case WM_MOVE:
	if (directX != DXFULLSCREEN) {
	    GetWindowRect(hWnd, &rcWindow);
	    GetClientRect(hWnd, &rcViewport);
	    GetClientRect(hWnd, &rcScreen);
	    ClientToScreen(hWnd, (POINT *) & rcScreen.left);
	    ClientToScreen(hWnd, (POINT *) & rcScreen.right);
	}
	break;
    case WM_SETCURSOR:
	if (directX == DXFULLSCREEN) {
	    SetCursor(NULL);
	    return TRUE;
	}
	break;
#ifdef DDRAW_DRIVER
    case WM_ACTIVATEAPP:
	{
	    int oldactive = active;
	    mouseButtons = 0;
	    if (directX == DXFULLSCREEN) {
		needredraw = 1;
		active = (wParam == WA_ACTIVE)
		    || (wParam == WA_CLICKACTIVE) /*(BOOL) wParam */ ;
		PaintDD();
		if (!oldactive && active && captured)
		    SetCursor(NULL), SetCapture(hWnd);
		if (oldactive && !active && captured)
		    ReleaseCapture();
		return 0L;
	    }
	}
#endif
	break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*Create Xaos Window. It is either used for normal window mode or
   as basis for DirectX */
static int InitWindow(void)
{
    int width = CW_USEDEFAULT, height = CW_USEDEFAULT;
    int xpos = CW_USEDEFAULT, ypos = CW_USEDEFAULT;
    LOGPALETTE *logPalette;
    WNDCLASS wndClass;
    LOGFONT logFont;
    ATOM a;
    HDC hDC;
    TEXTMETRIC textMetric;
    HGLOBAL oldFont;
    RECT r;
    closeFlag = FALSE;

    altPressed = arrowsPressed = 0;
    if (hIcon == NULL)
	hIcon = LoadIcon(hInstance, "BIG");
    mouseButtons = 0;
    mouseX = 0;
    mouseY = 0;
    {
	static FARPROC proc;
	if (hModule2 == NULL) {
	    hModule2 = LoadLibrary("user32");
	    proc = GetProcAddress(hModule2, "RegisterClassExA");
	}
	if (proc != NULL) {
	    WNDCLASSEX ExWndClass;
	    memset(&ExWndClass, 0, sizeof(WNDCLASSEX));
	    if (hIconSm == NULL)
		hIconSm = LoadIcon(hInstance, "SMALL");
	    ExWndClass.hIconSm = hIconSm;
	    memset(&ExWndClass, 0, sizeof(WNDCLASSEX));
	    ExWndClass.style = CS_OWNDC;
	    ExWndClass.cbSize = sizeof(WNDCLASSEX);
	    ExWndClass.lpfnWndProc = WindowProc;
	    ExWndClass.hInstance = hInstance;
	    ExWndClass.hIcon = hIcon;
	    ExWndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	    ExWndClass.lpszClassName = "XaosWindow";
	    ExWndClass.hbrBackground =
		(HBRUSH) GetStockObject(BLACK_BRUSH);
	    a = (ATOM) proc(&ExWndClass);
	} else {
	    memset(&wndClass, 0, sizeof(WNDCLASS));
	    wndClass.style = CS_OWNDC;
	    wndClass.lpfnWndProc = WindowProc;
	    wndClass.hInstance = hInstance;
	    wndClass.hIcon = hIcon;
	    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	    wndClass.lpszClassName = "XaosWindow";
	    wndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	    a = RegisterClass(&wndClass);
	}
    }
    if (!a) {
	x_error("Unable to create windows class");
	return 0;
    }

    /* First time use defaut size, otherwise use saved sizes */
    if (sscanf(size, "%ix%ix", &width, &height) != 2) {
	width = WWIDTH;
	height = WHEIGHT;
    }
    if (windowpos) {
	xpos = rcWindow.left;
	ypos = rcWindow.top;
	width = rcWindow.right - rcWindow.left;
	height = rcWindow.bottom - rcWindow.top;
    }

    /* create main window */
    if (directX == DXFULLSCREEN)
	hWnd =
	    CreateWindowEx(WS_EX_TOPMOST, "XaoSWindow", "XaoS", WS_POPUP,
			   0, 0, GetSystemMetrics(SM_CXSCREEN),
			   GetSystemMetrics(SM_CYSCREEN), NULL, NULL,
			   hInstance, NULL);
    else

	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, "XaoSWindow", "XaoS",
			      WS_OVERLAPPEDWINDOW | WS_EX_LEFTSCROLLBAR,
			      xpos, ypos, width, height, NULL, NULL,
			      hInstance, NULL);

    if (!hWnd) {
	x_error("Unable to create app window");
	return 0;
    }

    clipboard_format = RegisterClipboardFormat("image/x-xaos.position");


    /* create font */
    memset(&logFont, 0, sizeof(LOGFONT));
    hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    logFont.lfHeight = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    logFont.lfWeight = FW_NORMAL;
    logFont.lfPitchAndFamily = FIXED_PITCH;
    strcpy(logFont.lfFaceName, "Courier");

    hFont = CreateFontIndirect(&logFont);
    oldFont = SelectObject(hDC, hFont);
    GetTextMetrics(hDC, &textMetric);
    SelectObject(hDC, oldFont);
    DeleteDC(hDC);

    fontHeight = textMetric.tmHeight;
    fontWidth = textMetric.tmAveCharWidth;

    ShowWindow(hWnd, SW_NORMAL);

    GetClientRect(hWnd, &r);
    displayX = r.right;
    displayY = r.bottom;

    /* create palette */
    CalculateBITMAPINFO();	/* calculate BITMAPINFO structure */
    logPalette = malloc(sizeof(LOGPALETTE) + 4 * 256);
    logPalette->palVersion = 0x300;
    logPalette->palNumEntries = 256;
    memcpy(logPalette->palPalEntry, bmp->bmiColors, 4 * 256);
    hPalette = CreatePalette(logPalette);
    free(logPalette);

    /* select and realize palette */
    hDC = GetDC(hWnd);
    SelectPalette(hDC, hPalette, FALSE);
    RealizePalette(hDC);
    ReleaseDC(hWnd, hDC);

    // increase priority of XaoS
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    MyHelpMsg = RegisterWindowMessage(HELPMSGSTRING);

    return 1;
}

static void DeInitWindow()
{
    if (tmpcaptured)
	ReleaseCapture();
    if (directX != DXFULLSCREEN)
	windowpos = 1, GetWindowRect(hWnd, &rcWindow);
    /* normalize priority */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    if (helpname)
	WinHelp(hWnd, helpname, HELP_QUIT, 0), free(helpname), helpname =
	    NULL;

    /* destroy windows and other objects */
    DeleteObject(hFont);
    DestroyWindow(hWnd);
    UnregisterClass("XaosWindow", hInstance);
    if (bmp) {
	free(bmp);
	bmp = NULL;
    }
    if (hModule2 != NULL)
	FreeLibrary(hModule2);
    hModule2 = NULL;
    win32_uninitializewindows();
}

/* Display buffer to screen */
static void Paint(HDC hDC)
{
    if (!initialized || !buffer1)
	return;
    StretchDIBits(hDC, 0, 0, displayX, displayY,
		  0, 0, displayX, displayY,
		  (currentbuff == 0) ? buffer1 : buffer2,
		  bmp, DIB_RGB_COLORS, SRCCOPY);
    needredraw = 0;
}



static int Init(void)
{
    HDC hDC;
    buffer1 = buffer2 = NULL;

    // get bit depth
    hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    bitDepth = GetDeviceCaps(hDC, BITSPIXEL);
    if (bitDepth < 8)
	bitDepth = 16;
    if (bitDepth < 10)
	bitDepth = 8;
    if (bitDepth >= 10 && bitDepth < 20)
	bitDepth = 16;
    if (bitDepth >= 20 && bitDepth < 28)
	bitDepth = 24;
    if (bitDepth >= 32 && bitDepth < 32)
	bitDepth = 32;
    DeleteDC(hDC);

    // create windows and other objects
    if (!InitWindow())
	return 0;

    CalculateBITMAPINFO();	/* calculate BITMAPINFO structure */


    return 1;
}

static void getmouse(int *mx, int *my, int *mb)
{
    *mb = 0;
    if (mouseButtons & MK_LBUTTON)
	*mb |= 256;
    if (mouseButtons & MK_MBUTTON)
	*mb |= 512;
    if (mouseButtons & MK_RBUTTON)
	*mb |= 1024;
    *mx = mouseX;
    *my = mouseY;
}

static void
Processevents(int wait, int *mx, int *my, int *mb, int *k, int *c)
{
    MSG msg;
    int r;
    if (wait) {
	// wait for message if in wait mode
	r = GetMessage(&msg, hWnd, 0, 0);
	wait = 0;
    } else {
	// don't wait for message
	r = PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE);
    }
#if 0
    if (needredraw) {
	if (directX)
	    PaintDD();
	else
	    win32_display();
    }
#endif
    if (r > 0) {
	if (msg.message == WM_CHAR) {
	    // ascii char
	    *c = msg.wParam;
	}
	if (msg.message == WM_KEYUP)
	    switch (msg.wParam) {
	    case VK_MENU:
		altPressed = 0;
		break;
	    case VK_UP:
		arrowsPressed &= ~4;
		break;
	    case VK_DOWN:
		arrowsPressed &= ~8;
		break;
	    case VK_LEFT:
		arrowsPressed &= ~1;
		break;
	    case VK_RIGHT:
		arrowsPressed &= ~2;
		break;
	    }
	if (msg.message == WM_KEYDOWN) {
	    // any key
	    switch (msg.wParam) {
	    case VK_MENU:
		/*x_message("Alt"); */
		altPressed = 1;
		break;
	    case VK_UP:
		*c = UIKEY_UP;
		arrowsPressed |= 4;
		break;
	    case VK_DOWN:
		*c = UIKEY_DOWN;
		arrowsPressed |= 8;
		break;
	    case VK_LEFT:
		*c = UIKEY_LEFT;
		arrowsPressed |= 1;
		break;
	    case VK_RIGHT:
		*c = UIKEY_RIGHT;
		arrowsPressed |= 2;
		break;
	    case VK_ESCAPE:
		*c = UIKEY_ESC;
		break;
	    case VK_BACK:
		*c = UIKEY_BACKSPACE;
		break;
	    case VK_TAB:
		*c = UIKEY_TAB;
		break;
	    case VK_HOME:
		*c = UIKEY_HOME;
		break;
	    case VK_END:
		*c = UIKEY_END;
		break;
	    case VK_PRIOR:
		*c = UIKEY_PGUP;
		break;
	    case VK_NEXT:
		*c = UIKEY_PGDOWN;
		break;
#ifdef DDRAW_DRIVER
	    case VK_RETURN:
		/*x_message("Enter %i",altPressed); */
		if (altPressed) {
		    HDC hDC;
		    CONST char *cmd;
		    CONST menuitem *item;
		    if (directX == DXFULLSCREEN) {
			int depth;
			cmd = "dX-windowed";
			hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
			depth = GetDeviceCaps(hDC, BITSPIXEL);
			DeleteDC(hDC);
			if (!DXSUPPORTEDDEPTH(0, depth))
			    cmd = "win32";
		    } else {
			cmd = "dX-fullscreen";
		    }
		    item = menu_findcommand(cmd);
		    ui_menuactivate(item, NULL);
		}
		break;
#endif
	    }
	}
	// forward messages to window
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    getmouse(mx, my, mb);	// get mouse position

    *k = arrowsPressed;

    if (closeFlag)
	*c = -2;		// force quit if so requested

}

// calculate BITMAPINFO structure. It is used to copy bitmaps
static void CalculateBITMAPINFO()
{
    int i;
    if (!bmp)
	bmp = (BITMAPINFO *) malloc(sizeof(BITMAPINFOHEADER) + 4 * 256);

    memset(bmp, 0, sizeof(BITMAPINFOHEADER));
    bmp->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmp->bmiHeader.biWidth = displayX;
    bmp->bmiHeader.biHeight = -displayY;
    bmp->bmiHeader.biPlanes = 1;
    bmp->bmiHeader.biBitCount = bitDepth;

    // create default palette
    for (i = 0; i < 256; i++) {
	bmp->bmiColors[i].rgbRed = i;
	bmp->bmiColors[i].rgbGreen = i;
	bmp->bmiColors[i].rgbBlue = i;
    }
}

#ifdef DDRAW_DRIVER

/**************************************************************************************
                             DirectDraw driver helper routines
 */
static char *resstr[MAXRESOLUTIONS];
static struct resolutions {
    int width, height;
} ressize[MAXRESOLUTIONS];
static int nresolutions;
/* callback for DirectX resolutions */
static HRESULT WINAPI
EnumModesCallback(LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext)
{
    if (nresolutions < MAXRESOLUTIONS)
	if (lpDDSurfaceDesc->ddpfPixelFormat.u1.dwRGBBitCount == 8 ||
	    lpDDSurfaceDesc->ddpfPixelFormat.u1.dwRGBBitCount == 16 ||
	    lpDDSurfaceDesc->ddpfPixelFormat.u1.dwRGBBitCount == 24 ||
	    lpDDSurfaceDesc->ddpfPixelFormat.u1.dwRGBBitCount == 32) {
	    int i;
	    char s[20];
	    for (i = 0; i < nresolutions; i++)
		if ((int) ressize[i].width ==
		    (int) lpDDSurfaceDesc->dwWidth
		    && (int) ressize[i].height ==
		    (int) lpDDSurfaceDesc->dwHeight)
		    return DDENUMRET_OK;
	    ressize[nresolutions].width = lpDDSurfaceDesc->dwWidth;
	    ressize[nresolutions].height = lpDDSurfaceDesc->dwHeight;
	    sprintf(s, "%ix%i", lpDDSurfaceDesc->dwWidth,
		    lpDDSurfaceDesc->dwHeight);
	    resstr[nresolutions] = strdup(s);
	    nresolutions++;
	}
    return DDENUMRET_OK;
}

typedef HRESULT WINAPI(*ddrawcreateptr) (GUID FAR * lpGUID,
					 LPDIRECTDRAW FAR * lplpDD,
					 IUnknown FAR * pUnkOuter);
static ddrawcreateptr DirectDrawCreatePtr;

static int ResizeDD(int fullscreen)
{
    HRESULT ddrval;
    DDSURFACEDESC ddsd;
    /*DDCAPS2 ddscaps; */
    LPDIRECTDRAWCLIPPER pClipper;
    int dxwidth;
    int dxheight;
    int dxbpp;

    // free DirectX objects
    if (lpSurfaces[0])
	IDirectDrawSurface_Release(lpSurfaces[0]);
    lpSurfaces[0] = NULL;
    if (dxPalette)
	IDirectDrawPalette_Release(dxPalette);
    dxPalette = NULL;
    /* Set cooperative level */
    ddrval = IDirectDraw2_SetCooperativeLevel(lpDD2, hWnd,
					      fullscreen
					      ? (DDSCL_FULLSCREEN |
						 DDSCL_EXCLUSIVE |
						 DDSCL_ALLOWREBOOT)
					      : DDSCL_NORMAL);
    if (ddrval != DD_OK) {
	DeInitDD();
	x_error("Failed to set cooperative level");
	return 0;
    }

    if (fullscreen) {
	if (sscanf(dxsize, "%ix%ix%i", &dxwidth, &dxheight, &dxbpp) != 3) {
	    dxwidth = DXWIDTH;
	    dxheight = DXHEIGHT;
	    dxbpp = DXBPP;
	}
	displayX = dxwidth;
	displayY = dxheight;
	bitDepth = dxbpp;
	if (bitDepth < 10)
	    bitDepth = 8;
	if (bitDepth >= 10 && bitDepth < 20)
	    bitDepth = 16;
	if (bitDepth >= 20 && bitDepth < 28)
	    bitDepth = 24;
	if (bitDepth >= 32 && bitDepth < 32)
	    bitDepth = 32;

	/* set resolution and bit depth */
	ddrval =
	    IDirectDraw2_SetDisplayMode(lpDD2, displayX, displayY,
					bitDepth, 0, 0);
	if (ddrval != DD_OK) {
	    /* The display mode cannot be changed. 
	       The mode is either not supported or 
	       another application has exclusive mode.

	       Try 320x200x256 and 640x480x256 modes before giving up */
	    displayX = 320;
	    displayY = 200;
	    bitDepth = 8;
	    ddrval =
		IDirectDraw2_SetDisplayMode(lpDD2, displayX, displayY,
					    bitDepth, 0, 0);
	    if (ddrval != DD_OK) {
		displayY = 240;
		if (ddrval != DD_OK) {
		    displayX = 640;
		    displayY = 480;
		    ddrval =
			IDirectDraw2_SetDisplayMode(lpDD2, displayX,
						    displayY, bitDepth, 0,
						    0);
		    if (ddrval != DD_OK) {
			/* Bad luck... give up. */
			DeInitDD();
			return 0;
		    }
		}
	    }
	}
	SetRect(&rcViewport, 0, 0, displayX, displayY);
	rcScreen = rcViewport;
    } else {
	/* Get the dimensions of the viewport and screen bounds */
	GetClientRect(hWnd, &rcViewport);
	GetClientRect(hWnd, &rcScreen);
	ClientToScreen(hWnd, (POINT *) & rcScreen.left);
	ClientToScreen(hWnd, (POINT *) & rcScreen.right);
	/*bitDepth = GetDeviceCaps (hDC, BITSPIXEL); */

	/* Create clipper object for window */
	ddrval = IDirectDraw_CreateClipper(lpDD, 0, &pClipper, NULL);
	if (ddrval != DD_OK) {
	    DeInitDD();
	    x_error("Failed to create clipper object");
	    return 0;
	}
	/* Asociate it */
	IDirectDrawClipper_SetHWnd(pClipper, 0, hWnd);
    }
    /* Create the primary surface with one back buffer */
    CalculateBITMAPINFO();	// calculate BITMAPINFO structure

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    ddrval = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSurfaces[0], NULL);
    if (ddrval != DD_OK) {
	DeInitDD();
	x_error("Failed to create flipping surface");
	return 0;
    }

    if (!fullscreen) {
	IDirectDrawSurface_SetClipper(lpSurfaces[0], pClipper);
	IDirectDrawClipper_Release(pClipper);
	if (IDirectDrawSurface_GetSurfaceDesc(lpSurfaces[0], &ddsd) !=
	    DD_OK) {
	    DeInitDD();
	    x_error("Failed to get pixel format");
	    return 0;
	}
	bitDepth = ddsd.ddpfPixelFormat.u1.dwRGBBitCount;
    }

    if (bitDepth == 8) {
	/* create palette */
	ddrval =
	    IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT,
				      (LPPALETTEENTRY) bmp->bmiColors,
				      &dxPalette, NULL);
	if (ddrval != DD_OK) {
	    DeInitDD();
	    x_error("Failed to create palette");
	    return 0;
	}

	/* set palette */
	IDirectDrawSurface_SetPalette(lpSurfaces[0], dxPalette);
    }
    if (fullscreen)
	SetCursor(NULL);
    needredraw = 1;
    return 1;

}

/* init DirectX */
static int InitDD(int fullscreen)
{
    HRESULT ddrval;
    HDC hDC;
    directX = fullscreen ? DXFULLSCREEN : DXWINDOWED;

    if (!hModule)
	hModule = LoadLibrary("ddraw");
    if (!hModule) {
	/*x_error ("Unable to load DirectX (ddraw.dll)"); */
	return 0;
    }
    /* DirectDraw don't support 16 color modes. Don't even try to initialize
       it then. Also avoid unsupported bit depths in the windowed driver */
    hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    bitDepth = GetDeviceCaps(hDC, BITSPIXEL);
    DeleteDC(hDC);

    if (!DXSUPPORTEDDEPTH(fullscreen, bitDepth))
	return 0;


    DirectDrawCreatePtr =
	(ddrawcreateptr) GetProcAddress(hModule, "DirectDrawCreate");
    if (!DirectDrawCreatePtr) {
	x_error
	    ("Unable to get hook DirectDrawCreate in ddraw.dll. Check your DirectX installation");
	return 0;
    }

    lpDD = NULL;
    lpDD2 = NULL;
    lpSurfaces[0] = NULL;
    lpSurfaces[1] = NULL;
    buffer1 = buffer2 = NULL;

    bitDepth = 8;

    InitWindow();
    UpdateWindow(hWnd);
    SetFocus(hWnd);


    /* contact DirectX */
    ddrval = DirectDrawCreatePtr(NULL, &lpDD, NULL);
    if (ddrval != DD_OK) {
	DeInitDD();
	x_error("Failed to create DirectDraw object");
	return 0;
    }

    /* get IDirectDraw2 interface */
    ddrval =
	IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2,
				   (LPVOID *) & lpDD2);
    if (ddrval != DD_OK) {
	DeInitDD();
	x_error("Failed to get DirectDraw2 object");
	return 0;
    }
    /* enumerate modes */
#ifdef DDRAW_DRIVER
    if (!nresolutions && directX == DXFULLSCREEN)
	IDirectDraw2_EnumDisplayModes(lpDD2, 0, NULL, NULL,
				      EnumModesCallback);
#endif


    if (!ResizeDD(fullscreen))
	return 0;
    if (fullscreen) {
	SetCapture(hWnd);	// make sure no other windows get mouse messages

	captured = 1;
    }

    return 1;
}

/* uninitialize DirectX */
static void DeInitDD(void)
{
    if (captured)
	ReleaseCapture(), captured = 0;	// free mouse

    // free DirectX objects
    if (lpSurfaces[0])
	IDirectDrawSurface_Release(lpSurfaces[0]);
    lpSurfaces[0] = NULL;
    if (BackSurface[0])
	IDirectDrawSurface_Release(BackSurface[0]);
    BackSurface[0] = NULL;
    if (BackSurface[1])
	IDirectDrawSurface_Release(BackSurface[1]);
    BackSurface[1] = NULL;
    if (dxPalette)
	IDirectDrawPalette_Release(dxPalette);
    dxPalette = NULL;
    if (lpDD2)
	IDirectDraw2_Release(lpDD2);
    lpDD2 = NULL;
    if (lpDD)
	IDirectDraw_Release(lpDD);
    lpDD = NULL;
    DeInitWindow();
    if (hModule != NULL)
	FreeLibrary(hModule), hModule = NULL;
    hWnd = NULL;
    directX = 0;
}

static LRESULT CALLBACK WindowProc(HWND hwnd,	// handle to window
				   UINT uMsg,	// message identifier
				   WPARAM wParam,	// first message parameter
				   LPARAM lParam	// second message parameter
    );
static void UpdateMouseDD()
{
    DDSURFACEDESC m_surface;
    PUCHAR dst;
    DWORD ddrval;
    memset(&m_surface, 0, sizeof(DDSURFACEDESC));
    m_surface.dwSize = sizeof(DDSURFACEDESC);
    ddrval = IDirectDrawSurface_Lock(lpSurfaces[0], NULL, &m_surface,
				     DDLOCK_WAIT, NULL);
    if (ddrval != DD_OK) {
	return;
    }

    dst = (PUCHAR) m_surface.lpSurface;
    if (storeddata) {
	restore(dst, storeddata, bitDepth, m_surface.u1.lPitch, displayX,
		displayY, oldmouseX, oldmouseY);
	free(storeddata);
    }
    storeddata =
	store(dst, bitDepth, m_surface.u1.lPitch, displayX, displayY,
	      mouseX, mouseY);
    drawmouse(dst, mousepointer, bitDepth, m_surface.u1.lPitch, displayX,
	      displayY, mouseX, mouseY);
    oldmouseX = mouseX;
    oldmouseY = mouseY;
    IDirectDrawSurface_Unlock(lpSurfaces[0], m_surface.lpSurface);
}

/* Display buffer */
static void PaintDD()
{
    DWORD ddrval;
    if (!IsWindowVisible(hWnd) || !active || !initialized
	|| !BackSurface[0])
	return;
    IDirectDrawSurface_Unlock(BackSurface[0], surface[0].lpSurface);
    IDirectDrawSurface_Unlock(BackSurface[1], surface[1].lpSurface);
    if (directX == DXFULLSCREEN) {
	if (storeddata)
	    free(storeddata), storeddata = NULL;
	storeddata =
	    store(currentbuff ? buffer2 : buffer1, bitDepth, lineSize,
		  displayX, displayY, mouseX, mouseY);
	drawmouse(currentbuff ? buffer2 : buffer1, mousepointer, bitDepth,
		  lineSize, displayX, displayY, mouseX, mouseY);
	ddrval =
	    IDirectDrawSurface_BltFast(lpSurfaces[0], 0, 0,
				       BackSurface[currentbuff], &rcScreen,
				       FALSE);
	restore(currentbuff ? buffer2 : buffer1, storeddata, bitDepth,
		lineSize, displayX, displayY, mouseX, mouseY);
	oldmouseX = mouseX;
	oldmouseY = mouseY;
    } else {
	ddrval = IDirectDrawSurface_Blt(lpSurfaces[0], &rcScreen,
					BackSurface[currentbuff],
					&rcViewport, DDBLT_WAIT, NULL);
    }
    if (ddrval != DD_OK) {
	if ((int) ddrval == (int) DDERR_SURFACELOST) {
	    IDirectDrawSurface_Restore(lpSurfaces[0]);
	    IDirectDrawSurface_Restore(BackSurface[0]);
	    IDirectDrawSurface_Restore(BackSurface[1]);
	    ddrval = IDirectDrawSurface_Blt(lpSurfaces[0], &rcScreen,
					    BackSurface[currentbuff],
					    &rcViewport, DDBLT_WAIT, NULL);
	    //if (ddrval == DDERR_SURFACELOST) resized=1; /*We've lost our fractal*/
	}
    }
    ddrval = IDirectDrawSurface_Lock(BackSurface[0], NULL, &surface[0],
				     DDLOCK_WAIT, NULL);
    ddrval = IDirectDrawSurface_Lock(BackSurface[1], NULL, &surface[1],
				     DDLOCK_WAIT, NULL);
    if (buffer1 != (char *) surface[0].lpSurface ||
	buffer2 != (char *) surface[1].lpSurface) {
	DeInitDD();
	x_fatalerror
	    ("Unexpected event - buffers moved! Please contact authors!");
    }
    needredraw = 0;

}
#endif

/**************************************************************************************
                             Drivers implementation
 */



static void flip_buffers(void)
{
    currentbuff ^= 1;
}


static void processevents(int wait, int *mx, int *my, int *mb, int *k)
{
    int c = -1;
    *mb = 0;
    *k = 0;
    Processevents(wait, mx, my, mb, k, &c);
    if (c > -1) {
	ui_key(c);
    }

    if (c == -2)
	ui_quit();		// -2 signals program exit

    if (resized) {
	ui_resize();		// tell Xaos to resize

    }
}

static void print(int x, int y, CONST char *text)
{
    HDC hDC;
    static char current[256];
    char s[256];
#ifdef DDRAW_DRIVER
    if (directX == DXFULLSCREEN) {
	HGLOBAL oldFont;
	if (IDirectDrawSurface_GetDC(lpSurfaces[0], &hDC) != DD_OK)
	    return;
	SetTextColor(hDC, 0xffffff);
	SetBkColor(hDC, 0x000000);
	oldFont = SelectObject(hDC, hFont);
	ExtTextOut(hDC, x, y, 0, NULL, text, strlen(text), NULL);
	SelectObject(hDC, oldFont);
	IDirectDrawSurface_ReleaseDC(lpSurfaces[0], hDC);
	return;
    }
#endif
    if (!text[0])
	strcpy(s, "XaoS");
    else
	sprintf(s, "XaoS - %s", text);
    if (strcmp(current, s))
	strcpy(current, s), SetWindowText(hWnd, s);
}

static void mousetype(int type)
{
    char *cursor;
    switch (type) {
    default:
    case 0:
	cursor = IDC_ARROW;
	break;
    case 1:
	cursor = IDC_WAIT;
	break;
    case 2:
	cursor = IDC_NO;
	break;
    }
    SetCursor(LoadCursor(NULL, cursor));
}

static void set_palette(ui_palette pal1, int start, int end)
{
    PUCHAR pal = (PUCHAR) pal1;
    HDC hDC;
    int i;
    // store new palette entries locally
    memcpy(backpalette + 4 * start, pal, (end - start) * 4);
    for (i = start; i <= end; i++) {
	bmp->bmiColors[i].rgbRed = *(pal + 4 * (i - start) + 0);
	bmp->bmiColors[i].rgbGreen = *(pal + 4 * (i - start) + 1);
	bmp->bmiColors[i].rgbBlue = *(pal + 4 * (i - start) + 2);
	bmp->bmiColors[i].rgbReserved = 0;
    }
    // update window/screen
#ifdef DDRAW_DRIVER
    if (directX) {
	IDirectDrawPalette_SetEntries(dxPalette, 0, start, end - start + 1,
				      (PALETTEENTRY *) pal);
    } else
#endif
    {
	SetPaletteEntries(hPalette, start, end - start + 1,
			  (PALETTEENTRY *) pal);
	hDC = GetDC(hWnd);
	UnrealizeObject(hPalette);
	RealizePalette(hDC);
	ReleaseDC(hWnd, hDC);
	win32_display();
    }
}

static void win32_copy(struct uih_context *uih)
{
    char *c = ui_getpos();
    HANDLE hData = GlobalAlloc(GMEM_DDESHARE, strlen(c) + 1);
    char *data;
    if (!hData) {
	x_error("Out of memory");
	free(c);
	return;
    }
    if (!(data = GlobalLock(hData))) {
	x_error("Out of memory");
	free(c);
	return;
    }
    memcpy(hData, c, strlen(c) + 1);
    GlobalUnlock(hData);
    if (OpenClipboard(hWnd)) {
	EmptyClipboard();
	SetClipboardData(clipboard_format, hData);
    }
    free(c);
}

static void win32_paste(void)
{
    if (OpenClipboard(hWnd)) {
	HANDLE hClipData;
	char *text;
	if (!(hClipData = GetClipboardData(clipboard_format))) {
	    CloseClipboard();
	    return;
	}
	if (!(text = GlobalLock(hClipData))) {
	    x_error("Out of memory");
	    CloseClipboard();
	}
	ui_loadstr(strdup(text));
	GlobalUnlock(hClipData);
	CloseClipboard();
    }
}

#define MAX_MENUITEMS_I18N 7
static menuitem menuitems_i18n[MAX_MENUITEMS_I18N];

int uiw_no_menuitems_i18n, uiw_no_cutpasteitems_i18n;

static menuitem *cutpasteitems;
static void add_cutpasteitems()
{
    // General method (not needed currently):
    int no_menuitems_i18n = uiw_no_menuitems_i18n;	/* This variable must be local. */
    MENUSEPARATOR_I("edit");
    MENUNOP_I("edit", NULL, gettext("Copy"), "copy", 0, win32_copy);
    MENUNOP_I("edit", NULL, gettext("Paste"), "paste", 0, win32_paste);
    MENUNOP_I("misc", NULL, "Generate .dlg files", "genresources", 0,
	      win32_genresources);
    MENUSEPARATOR_I("helpmenu");
    MENUNOP_I("helpmenu", NULL, gettext("About"), "about", 0, AboutBox);
    no_menuitems_i18n -= uiw_no_menuitems_i18n;
    cutpasteitems = &(menuitems_i18n[uiw_no_menuitems_i18n]);
    uiw_no_cutpasteitems_i18n = no_menuitems_i18n;
    menu_add(cutpasteitems, uiw_no_cutpasteitems_i18n);
    uiw_no_menuitems_i18n += no_menuitems_i18n;

}


static int win32_init(void)
{
    int r;

#ifdef DDRAW_DRIVER
    directX = 0;
#endif
    r = Init();
    if (!r)
	return r;
    win32_driver.textwidth = fontWidth;
    win32_driver.textheight = fontHeight;
    getres(&win32_driver.width, &win32_driver.height);
    win32_createrootmenu();
    uiw_no_menuitems_i18n = 0;
    add_cutpasteitems();
    return r;
}

static void win32_uninitialize(void)
{
    DeInitWindow();
    menu_delete(cutpasteitems, uiw_no_cutpasteitems_i18n);
}

static void win32_getsize(int *width, int *height)
{
    resized = 0;
    *width = displayX;
    *height = displayY;
    switch (bitDepth) {
    case 8:
	win32_driver.imagetype = UI_C256;
	break;
    case 16:
	/* Windows seems to always use 15bpp mode */
	win32_driver.imagetype = UI_TRUECOLOR16;
	win32_driver.rmask = 31 * 32 * 32;
	win32_driver.gmask = 31 * 32;
	win32_driver.bmask = 31;
	break;
    case 24:
	win32_driver.imagetype = UI_TRUECOLOR24;
	win32_driver.rmask = 0xff0000;
	win32_driver.gmask = 0x00ff00;
	win32_driver.bmask = 0x0000ff;
	break;
    case 32:
	win32_driver.imagetype = UI_TRUECOLOR;
	win32_driver.rmask = 0xff0000;
	win32_driver.gmask = 0x00ff00;
	win32_driver.bmask = 0x0000ff;
	break;
    }
    CalculateBITMAPINFO();
}



static void win32_display()
{
    HDC hDC = GetDC(hWnd);
    if (IsWindowVisible(hWnd))
	Paint(hDC);
    ReleaseDC(hWnd, hDC);
}

static int win32_alloc_buffers(char **b1, char **b2, void **data)
{
    currentbuff = 0;
    // calculate DWORD aligned line length
    lineSize = displayX * ((bitDepth + 7) / 8);
    lineSize += 3 - ((lineSize - 1) & 3);

    buffer1 = (char *) malloc(displayY * lineSize);
    buffer2 = (char *) malloc(displayY * lineSize);
    *b1 = buffer1;
    *b2 = buffer2;
    initialized = 1;
    return lineSize;
}

static void win32_free_buffers(char *b1, char *b2)
{
    initialized = 0;
    free(buffer1);
    free(buffer2);
    buffer1 = buffer2 = NULL;
}

static CONST char *CONST dx_depth[] = { "8bpp (256 colors)",
    "16bpp (65536 colors)",
    "24bpp (16777216 colors)",
    "32bpp (16777216 colors)",
    NULL
};

#ifdef DDRAW_DRIVER
static menudialog dx_resdialog[] = {
    DIALOGCHOICE("Resolution", resstr, 0),
    DIALOGCHOICE("Depth", dx_depth, 0),
    {NULL}
};

static menudialog *dx_resizedialog(struct uih_context *c)
{
    int i;
    switch (bitDepth) {
    case 8:
	dx_resdialog[1].defint = 0;
	break;
    case 16:
	dx_resdialog[1].defint = 1;
	break;
    case 24:
	dx_resdialog[1].defint = 2;
	break;
    case 32:
	dx_resdialog[1].defint = 3;
    }
    for (i = 0; i < MAXRESOLUTIONS; i++)
	if (displayX == ressize[i].width && displayY == ressize[i].height) {
	    dx_resdialog[0].defint = i;
	    break;
	}
    return dx_resdialog;
}

static void dx_resize(struct uih_context *c, dialogparam * p)
{
    static char s[10];
    CONST static char *CONST st[] = { "8", "16", "24", "32" };
    sprintf(s, "%sx%s", resstr[p[0].dint], st[p[1].dint]);
    dxsize = s;
    resized = 1;
    ui_call_resize();
}

int uiw_no_resizeitems_i18n;

static menuitem *resizeitems;

static void add_resizeitems()
{
    // General method, it's needed:
    int no_menuitems_i18n = uiw_no_menuitems_i18n;	/* This variable must be local. */
    MENUCDIALOG_I("ui", "=", gettext("Resize"), "resize", 0, dx_resize,
		  dx_resizedialog);
    no_menuitems_i18n -= uiw_no_menuitems_i18n;
    resizeitems = &(menuitems_i18n[uiw_no_menuitems_i18n]);
    uiw_no_resizeitems_i18n = no_menuitems_i18n;
    menu_add(resizeitems, uiw_no_resizeitems_i18n);
    uiw_no_menuitems_i18n += no_menuitems_i18n;

}


static int dx_alloc_buffers(char **b1, char **b2)
{
    DWORD ddrval;
    DDSURFACEDESC ddsd;
    int i;
    currentbuff = 0;
    memset(surface, 0, sizeof(DDSURFACEDESC) * 2);
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    if (IDirectDrawSurface_GetSurfaceDesc(lpSurfaces[0], &ddsd) != DD_OK) {
	DeInitDD();
	x_error("Failed to get pixel format");
	return 0;
    }
    for (i = 0; i < 2; i++) {
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = displayX;
	ddsd.dwHeight = displayY;
	ddsd.ddsCaps.dwCaps =
	    DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	ddrval =
	    IDirectDraw_CreateSurface(lpDD, &ddsd, &BackSurface[i], NULL);
	if (ddrval != DD_OK) {
	    DeInitDD();
	    x_error("Failed to create back surface");
	    return 0;
	}
    }
    for (i = 0; i < 2; i++) {
	surface[i].dwSize = sizeof(DDSURFACEDESC);
	ddrval = IDirectDrawSurface_Lock(BackSurface[i], NULL, surface + i,
					 DDLOCK_WAIT, NULL);
	if (ddrval != DD_OK) {
	    DeInitDD();
	    x_fatalerror("Failed to lock offscreen surfaces");
	}
    }
    buffer1 = *b1 = (char *) surface[0].lpSurface;
    buffer2 = *b2 = (char *) surface[1].lpSurface;
    lineSize = surface[0].u1.lPitch;
    initialized = 1;
    return lineSize;
}

static void dx_free_buffers(char *b1, char *b2)
{
    IDirectDrawSurface_Unlock(BackSurface[0], surface[0].lpSurface);
    IDirectDrawSurface_Unlock(BackSurface[1], surface[1].lpSurface);
    if (BackSurface[0])
	IDirectDrawSurface_Release(BackSurface[0]);
    if (BackSurface[1])
	IDirectDrawSurface_Release(BackSurface[1]);
    BackSurface[0] = NULL;
    BackSurface[1] = NULL;
    initialized = 0;
    buffer1 = buffer2 = NULL;
}

static int dx_imgparams(void)
{
    DDSURFACEDESC s;
    memset(&s, 0, sizeof(s));
    s.dwSize = sizeof(s);
    if (IDirectDrawSurface_GetSurfaceDesc(lpSurfaces[0], &s) != DD_OK) {
	DeInitDD();
	x_error("Failed to get pixel format");
	return 0;
    }
    switch (s.ddpfPixelFormat.u1.dwRGBBitCount) {
    case 8:
	dxw_driver.imagetype = UI_C256;
	dxf_driver.imagetype = UI_C256;
	break;
    case 16:
    case 15:
	dxw_driver.imagetype = UI_TRUECOLOR16;
	dxf_driver.imagetype = UI_TRUECOLOR16;
	break;
    case 24:
	dxw_driver.imagetype = UI_TRUECOLOR24;
	dxf_driver.imagetype = UI_TRUECOLOR24;
	break;
    case 32:
	dxw_driver.imagetype = UI_TRUECOLOR;
	dxf_driver.imagetype = UI_TRUECOLOR;
	break;
    default:
	x_fatalerror
	    ("Unsupported bit depth! Only 8bpp, 16bpp, 24bpp and 32bpp modes supported\n");
	return 0;
    }
    dxw_driver.rmask = s.ddpfPixelFormat.u2.dwRBitMask;
    dxw_driver.gmask = s.ddpfPixelFormat.u3.dwGBitMask;
    dxw_driver.bmask = s.ddpfPixelFormat.u4.dwBBitMask;
    dxf_driver.rmask = s.ddpfPixelFormat.u2.dwRBitMask;
    dxf_driver.gmask = s.ddpfPixelFormat.u3.dwGBitMask;
    dxf_driver.bmask = s.ddpfPixelFormat.u4.dwBBitMask;
    dxf_driver.textwidth = fontWidth;
    dxf_driver.textheight = fontHeight;
    dxw_driver.textwidth = fontWidth;
    dxw_driver.textheight = fontHeight;
    return 1;
}

static int dxw_init(void)
{
    int r;

    r = InitDD(0);
    if (!r)
	return r;

    if (!dx_imgparams())
	return 0;
    win32_createrootmenu();
    getres(&dxw_driver.width, &dxw_driver.height);
    uiw_no_menuitems_i18n = 0;
    add_cutpasteitems();
    return r;
}

static int dxf_init(void)
{
    int r;

    getdimens(&dxf_driver.width, &dxf_driver.height);
    r = InitDD(1);
    if (!r)
	return r;

    if (!dx_imgparams())
	return 0;
    uiw_no_menuitems_i18n = 0;
    add_resizeitems();
    add_cutpasteitems();
    return r;
}



static void dx_uninitialize(void)
{
    if (directX == DXFULLSCREEN)
	menu_delete(resizeitems, uiw_no_resizeitems_i18n);
    menu_delete(cutpasteitems, uiw_no_cutpasteitems_i18n);
    DeInitDD();
}




static void dx_getsize(int *width, int *height)
{
    if (resized) {
	resized = 0;
	if (!ResizeDD(directX == DXFULLSCREEN)) {
	    DeInitDD();
	    x_fatalerror("Failed to resize");
	}
	if (!dx_imgparams()) {
	    DeInitDD();
	    x_fatalerror("Internal program error #34234");
	}
    }
    *width = displayX;
    *height = displayY;
    CalculateBITMAPINFO();
}

static void dx_mousetype(int type)
{
    switch (type) {
    default:
    case 0:
	mousepointer = mouse_pointer_data;
	break;
    case 1:
	mousepointer = wait_pointer_data;
	break;
    case 2:
	mousepointer = replay_pointer_data;
	break;
    }
    UpdateMouseDD();
}
#endif

void win32_help(struct uih_context *c, CONST char *name)
{
#ifdef HTML_HELP
    FILE *f;
    char *n;
    if (helpname == NULL) {
	if (directX == DXFULLSCREEN)
	    ShowWindow(hWnd, SW_MINIMIZE);
	n = xio_fixpath("\01\\help\\xaoshelp.chm");
	if ((f = fopen(n, "r"))) {
	    fclose(f);
	} else {
	    free(n);
	    n = xio_fixpath("\01\\..\\help\\xaoshelp.chm");
	    if ((f = fopen(n, "r"))) {
		fclose(f);
	    } else
		n = strdup("..\\help\\xaoshelp.chm");
	}
	helpname = n;
    }
    HH_AKLINK link;
    link.cbStruct =     sizeof(HH_AKLINK) ;
    link.fReserved =    FALSE ;
    link.pszKeywords =  name ;
    link.pszUrl =       NULL ;
    link.pszMsgText =   NULL ;
    link.pszMsgTitle =  NULL ;
    link.pszWindow =    NULL ;
    link.fIndexOnFail = TRUE ;

    if (!HtmlHelp(hWnd, helpname, HH_ALINK_LOOKUP, (DWORD) &link)) {
	x_error("Could not display help for topic %s from file %s", name, helpname);
    }
#else
    x_error("Help support not included in this executable.");
#endif
}


static struct params params[] = {
    {"", P_HELP, NULL, "Win32 driver options:"},
    {"-size", P_STRING, &size,
     "Window size in format WIDTHxHEIGHT (320x200)"},
    {NULL, 0, NULL, NULL}
};

static struct params dxfparams[] = {
    {"", P_HELP, NULL, "DirectX fullscreen driver options:"},
    {"-mode", P_STRING, &dxsize,
     "Select preffered graphics mode in format WIDTHxHEIGHTxDEPTH  (320x200x8)"},
    {NULL, 0, NULL, NULL}
};

static struct params dxwparams[] = {
    {"", P_HELP, NULL, "DirectX windowed driver options:"},
    {"-size", P_STRING, &size,
     "Window size in format WIDTHxHEIGHT (320x200)"},
    {NULL, 0, NULL, NULL}
};

extern int XaoS_main(int argc, char **argv);
int APIENTRY
WinMain(HINSTANCE hInstance1,
	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    static char name0[256];
    static char *argv[256];
    int argc = 1;
    int i;

    GetModuleFileName(hInstance1, name0, 256);
    /* Allocate everything virtually - be on the safe side */
    argv[0] = strdup(name0);
    lpCmdLine = strdup(lpCmdLine);

    for (i = 0; lpCmdLine[i]; i++) {
	if (lpCmdLine[i] == ' ' || lpCmdLine[i] == '\t')
	    lpCmdLine[i] = 0;
	else if (!i || !lpCmdLine[i - 1])
	    argv[argc] = lpCmdLine + i, argc++;
    }

    /* Attach to parent console if available so output will be visible */
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
	/* make sure stdout is not already redirected before redefining */
	if (_fileno(stdout) == -1 || _get_osfhandle(fileno(stdout)) == -1)
	    freopen("CON", "w", stdout);
    }

    hInstance = hInstance1;
    return XaoS_main(argc, argv);
}

static CONST struct gui_driver win32_gui_driver = {
    win32_dorootmenu,
    win32_enabledisable,
    win32_menu,
    win32_dialog,
    win32_help
};

static CONST struct gui_driver win32_fullscreen_gui_driver = {
    NULL,
    NULL,
    NULL,
    NULL,
    win32_help
};

struct ui_driver win32_driver = {
    "win32",
    win32_init,
    win32_getsize,
    processevents,
    getmouse,
    win32_uninitialize,
    NULL,			//     win32_set_color,
    set_palette,
    print,
    win32_display,
    win32_alloc_buffers,
    win32_free_buffers,
    flip_buffers,
    mousetype,
    NULL,
    16 + 16,
    12,
    params,
    PIXELSIZE | UPDATE_AFTER_PALETTE,
    0.0, 0.0,
    0, 0,
    UI_C256,
    0, 256, 255,
    0, 0, 0,
    &win32_gui_driver
};

#ifdef DDRAW_DRIVER
struct ui_driver dxw_driver = {
    "dX-window",
    dxw_init,
    dx_getsize,
    processevents,
    getmouse,
    dx_uninitialize,
    NULL,			//     dx_set_color,
    set_palette,
    print,
    PaintDD,
    dx_alloc_buffers,
    dx_free_buffers,
    flip_buffers,
    mousetype,
    NULL,
    16 + 16,
    12,
    dxwparams,
    PIXELSIZE,
    0.0, 0.0,
    0, 0,
    UI_C256,
    0, 256, 255,
    0, 0, 0,
    &win32_gui_driver
};

struct ui_driver dxf_driver = {
    "dX-fullscreen",
    dxf_init,
    dx_getsize,
    processevents,
    getmouse,
    dx_uninitialize,
    NULL,			//     dx_set_color,
    set_palette,
    print,
    PaintDD,
    dx_alloc_buffers,
    dx_free_buffers,
    flip_buffers,
    dx_mousetype,
    NULL,
    16 + 16,
    12,
    dxfparams,
    FULLSCREEN | SCREENSIZE,
    0.0, 0.0,
    0, 0,
    UI_C256,
    0, 256, 255,
    0, 0, 0,
    &win32_fullscreen_gui_driver
};
#endif

void x_message(const char *text, ...)
{
    va_list ap;
    char buf[4096];
    va_start(ap, text);
    vsprintf(buf, text, ap);
    if (directX == DXFULLSCREEN)
	ShowWindow(hWnd, SW_MINIMIZE);
    MessageBox(NULL, buf, "XaoS", MB_OK | MB_ICONINFORMATION);
    va_end(ap);
}

void x_error(const char *text, ...)
{
    va_list ap;
    char buf[4096];
    va_start(ap, text);
    vsprintf(buf, text, ap);
    if (directX == DXFULLSCREEN)
	ShowWindow(hWnd, SW_MINIMIZE);
    MessageBox(NULL, buf, "XaoS have problem", MB_OK | MB_ICONEXCLAMATION);
    va_end(ap);
}

void x_fatalerror(const char *text, ...)
{
    va_list ap;
    char buf[4096];
    va_start(ap, text);
    vsprintf(buf, text, ap);
    if (directX == DXFULLSCREEN)
	ShowWindow(hWnd, SW_MINIMIZE);
    MessageBox(NULL, buf, "Unrecovable XaoS error", MB_OK | MB_ICONSTOP);
    va_end(ap);
    exit(1);
}


#endif				/* WIN32_DRIVER */
