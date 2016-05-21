// Header file for Menu.cpp

#ifndef _MENU_H_
#define _MENU_H_

#include <windows.h>
#include <commctrl.h>

typedef struct
{
	HMENU hMenu;
	char szItemText[MAX_PATH];
	int iImage;
} MyItem;

#define ICON_WIDTH 16
#define ICON_HEIGHT 15

class CMenu
{
friend BOOL CALLBACK NewMenuToolProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

public:
	CMenu(HWND hwndParent, bool fCanUseVer5);
	~CMenu();
	bool Create(HMENU hMenu, bool fReleaseMenu, HBITMAP hbmpImages, HBITMAP hbmpDisabledImages);
	HMENU GetMenu()
		{ return m_hMenu; }
	HMENU GetSubMenu(int iSubMenu)
		{ return ::GetSubMenu(m_hMenu, iSubMenu); }

	virtual LRESULT OnMeasureItem(MEASUREITEMSTRUCT * pmis);
	virtual LRESULT OnDrawItem(DRAWITEMSTRUCT * pdis);

protected:
	HWND m_hwndParent;
	HDC m_hdcMem;
	HBITMAP m_hbmpCheck;
	HIMAGELIST m_himl;
	HIMAGELIST m_himlDisabled;
	HMENU m_hMenu;
	bool m_fReleaseMenu;
	bool m_fCanUseVer5;

	int GetImageFromID(UINT id);
	void DrawIcon(HDC hdc, int nXDest, int nYDest, int nIndex, bool fEnabled);
};

#endif // !_MENU_H_