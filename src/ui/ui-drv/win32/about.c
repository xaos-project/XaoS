#include <windows.h>
#include "fconfig.h"
#include "xerror.h"
#include "ui_win32.h"
#include "about.h"
void CenterWindow(HWND hwndChild, HWND hwndParent)
{
    RECT rChild, rParent, rWorkArea;
    int wChild, hChild, wParent, hParent;
    int xNew, yNew;
    BOOL bResult;

    GetWindowRect(hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    GetWindowRect(hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    bResult = SystemParametersInfo(SPI_GETWORKAREA,	// system parameter to query or set
				   sizeof(RECT), &rWorkArea, 0);
    if (!bResult) {
	rWorkArea.left = rWorkArea.top = 0;
	rWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
	rWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    xNew = rParent.left + ((wParent - wChild) / 2);
    if (xNew < rWorkArea.left) {
	xNew = rWorkArea.left;
    } else if ((xNew + wChild) > rWorkArea.right) {
	xNew = rWorkArea.right - wChild;
    }

    yNew = rParent.top + ((hParent - hChild) / 2);
    if (yNew < rWorkArea.top) {
	yNew = rWorkArea.top;
    } else if ((yNew + hChild) > rWorkArea.bottom) {
	yNew = rWorkArea.bottom - hChild;
    }

    SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0,
		 SWP_NOSIZE | SWP_NOZORDER);
    return;
}


static LPTSTR GetStringRes(int id)
{
    static TCHAR buffer[MAX_PATH];

    buffer[0] = 0;
    LoadString(GetModuleHandle(NULL), id, buffer, MAX_PATH);
    return buffer;
}

static LRESULT CALLBACK
About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HFONT hfontDlg;	// Font for dialog text
    static HFONT hFinePrint;	// Font for 'fine print' in dialog
    DWORD dwVerInfoSize;	// Size of version information block
    LPSTR lpVersion;		// String pointer to 'version' text
    DWORD dwVerHnd = 0;		// An 'ignored' parameter, always '0'
    UINT uVersionLen;
    WORD wRootLen;
    BOOL bRetCode;
    int i;
    char szFullPath[256];
    char szResult[256];
    char szGetName[256];
    DWORD dwVersion;
    char szVersion[40];
    DWORD dwResult;

    switch (message) {
    case WM_INITDIALOG:
	ShowWindow(hDlg, SW_HIDE);

	if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE) {
	    hfontDlg =
		CreateFont(14, 0, 0, 0, 0, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0,
			   0, VARIABLE_PITCH | FF_DONTCARE, "");
	    hFinePrint =
		CreateFont(11, 0, 0, 0, 0, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0,
			   0, VARIABLE_PITCH | FF_DONTCARE, "");
	} else {
	    hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				  VARIABLE_PITCH | FF_SWISS, "");
	    hFinePrint = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				    VARIABLE_PITCH | FF_SWISS, "");
	}

	CenterWindow(hDlg, GetWindow(hDlg, GW_OWNER));
	GetModuleFileName(hInstance, szFullPath, sizeof(szFullPath));
#if 0
	// Now lets dive in and pull out the version information:
	dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
	if (dwVerInfoSize) {
	    LPSTR lpstrVffInfo;
	    HANDLE hMem;
	    hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
	    lpstrVffInfo = GlobalLock(hMem);
	    GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize,
			       lpstrVffInfo);
	    // The below 'hex' value looks a little confusing, but
	    // essentially what it is, is the hexidecimal representation
	    // of a couple different values that represent the language
	    // and character set that we are wanting string values for.
	    // 040904E4 is a very common one, because it means:
	    //   US English, Windows MultiLingual characterset
	    // Or to pull it all apart:
	    // 04------        = SUBLANG_ENGLISH_USA
	    // --09----        = LANG_ENGLISH
	    // --11----        = LANG_JAPANESE
	    // ----04E4 = 1252 = Codepage for Windows:Multilingual

	    lstrcpy(szGetName, GetStringRes(IDS_VER_INFO_LANG));

	    wRootLen = (WORD) lstrlen(szGetName);	// Save this position

	    // Set the title of the dialog:
	    lstrcat(szGetName, "ProductName");
	    bRetCode = VerQueryValue((LPVOID) lpstrVffInfo,
				     (LPSTR) szGetName,
				     (LPVOID) & lpVersion,
				     (UINT *) & uVersionLen);

	    // Notice order of version and string...
	    if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE) {
		lstrcpy(szResult, lpVersion);
		lstrcat(szResult, " ÇÃÉoÅ[ÉWÉáÉìèÓïÒ");
	    } else {
		lstrcpy(szResult, "About ");
		lstrcat(szResult, lpVersion);
	    }

	    // -----------------------------------------------------

	    SetWindowText(hDlg, szResult);

	    // Walk through the dialog items that we want to replace:
	    for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++) {
		GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
		szGetName[wRootLen] = (char) 0;
		lstrcat(szGetName, szResult);
		uVersionLen = 0;
		lpVersion = NULL;
		x_message("lpstrVffInfo %s", lpstrVffInfo);
		bRetCode = VerQueryValue((LPVOID) lpstrVffInfo,
					 (LPSTR) szGetName,
					 (LPVOID) & lpVersion,
					 (UINT *) & uVersionLen);

		if (bRetCode && uVersionLen && lpVersion) {
		    // Replace dialog item text with version info
		    lstrcpy(szResult, lpVersion);
		    SetDlgItemText(hDlg, i, szResult);
		} else {
		    dwResult = GetLastError();

		    wsprintf(szResult, GetStringRes(IDS_VERSION_ERROR),
			     dwResult);
		    SetDlgItemText(hDlg, i, szResult);
		}
		SendMessage(GetDlgItem(hDlg, i), WM_SETFONT,
			    /* (ULONG_PTR) */
			    (unsigned long *) ((i == DLG_VERLAST) ?
					       hFinePrint : hfontDlg),
			    TRUE);
	    }			// for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)


	    GlobalUnlock(hMem);
	    GlobalFree(hMem);
	} else {
	    // No version information available.
	}			// if (dwVerInfoSize)
#endif
	for (i = DLG_VERFIRST; i <= IDC_LICENSE; i++) {
	    SendMessage(GetDlgItem(hDlg, i), WM_SETFONT,
			/* (ULONG_PTR) */
			(unsigned long *) ((i == DLG_VERLAST) ? hFinePrint
					   : hfontDlg), TRUE);
	}			// for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)

	SendMessage(GetDlgItem(hDlg, IDC_LABEL), WM_SETFONT,
		    (WPARAM) hfontDlg, (LPARAM) TRUE);

	// We are  using GetVersion rather then GetVersionEx
	// because earlier versions of Windows NT and Win32s
	// didn't include GetVersionEx:
	dwVersion = GetVersion();

	if (dwVersion < 0x80000000) {
	    // Windows NT
	    wsprintf(szVersion, "Microsoft Windows NT %u.%u (Build: %u)",
		     (DWORD) (LOBYTE(LOWORD(dwVersion))),
		     (DWORD) (HIBYTE(LOWORD(dwVersion))),
		     (DWORD) (HIWORD(dwVersion)));
	} else if (LOBYTE(LOWORD(dwVersion)) < 4) {
	    // Win32s
	    wsprintf(szVersion, "Microsoft Win32s %u.%u (Build: %u)",
		     (DWORD) (LOBYTE(LOWORD(dwVersion))),
		     (DWORD) (HIBYTE(LOWORD(dwVersion))),
		     (DWORD) (HIWORD(dwVersion) & ~0x8000));
	} else {
	    // Windows 95
	    wsprintf(szVersion, "Microsoft Windows 95 %u.%u",
		     (DWORD) (LOBYTE(LOWORD(dwVersion))),
		     (DWORD) (HIBYTE(LOWORD(dwVersion))));
	}

	SetWindowText(GetDlgItem(hDlg, IDC_OSVERSION), szVersion);
	ShowWindow(hDlg, SW_SHOW);
	return (TRUE);

    case WM_COMMAND:
	if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
	    EndDialog(hDlg, TRUE);
	    DeleteObject(hfontDlg);
	    DeleteObject(hFinePrint);
	    return (TRUE);
	}
	break;
    }

    return FALSE;
}

void AboutBox(void)
{
#ifdef DDRAW_DRIVER
    if (directX == 1)
	return;
#endif
    DialogBox(hInstance, "AboutBox", hWnd, (DLGPROC) About);
}
