#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

// This file implements icons on a menu
#include "Menu.h"
#include "Resource.h"
#include "afxres.h"
#include <stdio.h>

CMenu * pMenu = NULL;

const UINT g_rgButtonInfo[][2] =
{
	{ID_FILE_NEW, 0},
	{ID_FILE_OPEN, 1},
	{ID_FILE_SAVE, 2},
	{ID_TABS_SAVE, 2},
	{ID_FILE_SAVEALL, 3},
	{ID_TABS_SAVEALL, 3},
	{ID_FILE_CLOSE, 4},
	{ID_TABS_CLOSE, 4},
	{ID_FILE_CLOSEALL, 5},
	{ID_TABS_CLOSEALL, 5},
	{ID_FILE_PRINT, 6},
	{ID_EDIT_CUT, 7},
	{ID_EDIT_COPY, 8},
	{ID_EDIT_PASTE, 9},
	{ID_EDIT_UNDO, 10},
	{ID_EDIT_REDO, 11},
	{ID_EDIT_FIND, 12},
	{ID_EDIT_FINDPREV, 13},
	{ID_EDIT_FINDNEXT, 14},
	{ID_TOOLS_INSERTCHAR, 15},
	{ID_HELP_ABOUT, 16},
#ifdef SPELLING
	{ID_TOOLS_SPELL, 17},
#endif // SPELLING
	{ID_TOOLS_SORT, 18},
	{ID_OPTIONS_FONT, 19},
	{ID_OPTIONS_TAB, 20},
	{ID_WINDOW_NEXT, 21},
	{ID_WINDOW_PREV, 22},
	{ID_WINDOW_MANAGER, 23},
	{ID_FILE_PROPERTIES, 24},
	{ID_TABS_PROPERTIES, 24},
};


/***********************************************************************************************
	CMenu methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CMenu::CMenu(HWND hwndParent, bool fCanUseVer5)
{
	m_hwndParent = hwndParent;
	m_himl = NULL;
	m_himlDisabled = NULL;
	m_hdcMem = NULL;
	m_hbmpCheck = NULL;
	pMenu = this;
	m_fReleaseMenu = false;
	m_fCanUseVer5 = fCanUseVer5;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CMenu::~CMenu()
{
	HMENU hMenuPopup;
	MENUITEMINFO mii = { 0 };
	if (m_fCanUseVer5)
		mii.cbSize = sizeof(MENUITEMINFO);
	else
		mii.cbSize = CDSIZEOF_STRUCT(MENUITEMINFOA,cch);
	MyItem * pmi;

	mii.fMask = MIIM_DATA;
	int nMenuBarItemCount = ::GetMenuItemCount(m_hMenu);
	for (int i = 0; i < nMenuBarItemCount; i++)
	{
		hMenuPopup = GetSubMenu(i);
		int cMenuItem = ::GetMenuItemCount(hMenuPopup);
		for (int j = 0; j < cMenuItem; j++)
		{
			if (::GetMenuItemID(hMenuPopup, j) > 0)
			{
				::GetMenuItemInfo(hMenuPopup, j, TRUE, &mii);
				pmi = (MyItem *)mii.dwItemData;
				if (pmi)
					delete pmi;
			}
		}
	}
	::DeleteObject(m_hbmpCheck);
	::DeleteDC(m_hdcMem);
	if (m_fReleaseMenu)
		::DestroyMenu(m_hMenu);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CMenu::GetImageFromID(UINT id)
{
	int cbtn = sizeof(g_rgButtonInfo) / (sizeof(UINT) * 2);
	for (int ibtn = 0; ibtn < cbtn; ibtn++)
	{
		if (g_rgButtonInfo[ibtn][0] == id)
			return g_rgButtonInfo[ibtn][1];
	}
	return -1;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CMenu::Create(HMENU hMenu, bool fReleaseMenu, HBITMAP hbmpImages,
	HBITMAP hbmpDisabledImages)
{
	m_hMenu = hMenu;
	m_fReleaseMenu = fReleaseMenu;
	HMENU hMenuPopup;

	// Create image lists from the toolbar bitmaps passed in
	m_himl = ImageList_Create(ICON_WIDTH, ICON_HEIGHT, ILC_COLOR32, 0, 10);
	if (!m_himl)
		return false;
	ImageList_SetBkColor(m_himl, CLR_NONE);
	ImageList_Add(m_himl, hbmpImages, NULL);

	// Create image lists from the toolbar bitmaps passed in
	m_himlDisabled = ImageList_Create(ICON_WIDTH, ICON_HEIGHT, ILC_COLOR4, 0, 10);
	if (!m_himlDisabled)
		return false;
	ImageList_SetBkColor(m_himlDisabled, CLR_NONE);
	ImageList_Add(m_himlDisabled, hbmpDisabledImages, NULL);

	MENUITEMINFO mii = { 0 };
	if (m_fCanUseVer5)
		mii.cbSize = sizeof(MENUITEMINFO);
	else
		mii.cbSize = CDSIZEOF_STRUCT(MENUITEMINFOA,cch);
	MyItem * pmi;

	// Set the OwnerDraw style on every menu item that contains text
	int iItems = ::GetMenuItemCount(m_hMenu);
	TBBUTTONINFO tbbi = {sizeof(tbbi), TBIF_IMAGE};
	for (int i = 0; i < iItems; i++)
	{
		hMenuPopup = GetSubMenu(i);
		int iSubItems = ::GetMenuItemCount(hMenuPopup);
		for (int j = 0; j < iSubItems; j++)
		{
			if (::GetMenuItemID(hMenuPopup, j) > 0)
			{
				// Find the current text in the menu
				pmi = new MyItem;
				if (pmi)
				{
					mii.fMask = MIIM_TYPE | MIIM_ID;
					mii.cch = MAX_PATH;
					mii.dwTypeData = pmi->szItemText;
					::GetMenuItemInfo(hMenuPopup, j, TRUE, &mii);
					pmi->hMenu = hMenuPopup;
					pmi->iImage = GetImageFromID(mii.wID);
					mii.fMask = MIIM_DATA | MIIM_TYPE;
					mii.fType = MFT_OWNERDRAW;
					mii.dwItemData = (DWORD)pmi;
					::SetMenuItemInfo(hMenuPopup, j, TRUE, &mii);
				}
			}
		}
	}

	// Create the bitmap for a menu check mark
	HDC hdc = ::GetDC(m_hwndParent);
	m_hdcMem = ::CreateCompatibleDC(hdc);
	::ReleaseDC(m_hwndParent, hdc);
	m_hbmpCheck = ::CreateCompatibleBitmap(m_hdcMem, 20, 20);
	::SelectObject(m_hdcMem, m_hbmpCheck);
	RECT rect = {4, 3, 20, 20};
	::PatBlt(m_hdcMem, 0, 0, 20, 20, WHITENESS);
	::DrawFrameControl(m_hdcMem, &rect, DFC_MENU, DFCS_MENUCHECK);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CMenu::OnMeasureItem(MEASUREITEMSTRUCT * pmis)
{
	MyItem * pmi = (MyItem *)pmis->itemData;
	HDC hdc = ::GetDC(m_hwndParent);
	HFONT hfontOld = (HFONT)::SelectObject(hdc, ::GetStockObject(DEFAULT_GUI_FONT));
	RECT rect = {0};
	char * pTab = strchr(pmi->szItemText, '\t');
	SIZE size1;
	SIZE size2 = {0};

	::GetTextExtentPoint32(hdc, pmi->szItemText, strlen(pmi->szItemText), &size1);
	pmis->itemWidth = size1.cx + 40 + (10 * (pTab != 0)) - ::GetSystemMetrics(SM_CXMENUCHECK);
	pmis->itemHeight = max(size1.cy + 6, 19);

	::SelectObject(hdc, hfontOld);
	::ReleaseDC(m_hwndParent, hdc);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CMenu::OnDrawItem(DRAWITEMSTRUCT * pdis)
{
	MyItem * pmi = (MyItem *)pdis->itemData;
	COLORREF crPrevText;
	COLORREF crPrevBkgnd;

	RECT rcText = pdis->rcItem;
	RECT rcPix = { rcText.left, rcText.top, rcText.left + 20, rcText.bottom };
	rcText.left += 23;

	char szText[MAX_PATH];
	lstrcpy(szText, pmi->szItemText);
	char* szPart2 = strstr(szText, "\t");
	if (szPart2)
		*(szPart2++) = '\0';

	bool fHasIcon = (pmi->iImage != -1);
	if (pdis->itemState & ODS_SELECTED)
	{
		crPrevBkgnd = ::SetBkColor(pdis->hDC, ::GetSysColor(COLOR_HIGHLIGHT));
		if (fHasIcon)
		{
			rcText.left -= 3;
			::ExtTextOut(pdis->hDC, 0, 0, ETO_OPAQUE, &rcText, NULL, 0, NULL);
			rcText.left += 3;
		}
		else
		{
			::ExtTextOut(pdis->hDC, 0, 0, ETO_OPAQUE, &pdis->rcItem, NULL, 0, NULL);
		}
	}
	else
	{
		crPrevBkgnd = ::SetBkColor(pdis->hDC, ::GetSysColor(COLOR_MENU));
		::ExtTextOut(pdis->hDC, 0, 0, ETO_OPAQUE, &pdis->rcItem, NULL, 0, NULL);
	}
	::SetBkMode(pdis->hDC, TRANSPARENT);
	rcText.right -= 20;

	if (pdis->itemState & ODS_DISABLED)
	{
		// Draw the left side of the menu.
		if (pdis->itemState & ODS_CHECKED)
		{
			DrawState(pdis->hDC, 0, NULL, MAKELONG(m_hbmpCheck, 0), 0, rcPix.left + 2,
				rcPix.top +1, rcPix.right, rcPix.bottom, DST_BITMAP | DSS_DISABLED);
		}
		else if (fHasIcon)
		{
			DrawIcon(pdis->hDC, rcPix.left + 2, rcPix.top + 1, pmi->iImage, FALSE);
		}
		if (pdis->itemState & ODS_SELECTED)
			crPrevText = ::SetTextColor(pdis->hDC, ::GetSysColor(COLOR_3DLIGHT));
		else
		{
			crPrevText = ::SetTextColor(pdis->hDC, RGB(255, 255, 255));
			::OffsetRect(&rcText, 1, 1);
			// Draw the shadow for the text.
			if (szPart2)
			{
				::DrawText(pdis->hDC, szPart2, lstrlen(szPart2), &rcText,
					DT_NOCLIP | DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
			}
			::DrawText(pdis->hDC, szText, lstrlen(szText), &rcText,
				DT_NOCLIP | DT_SINGLELINE | DT_VCENTER);
			::OffsetRect(&rcText, -1, -1);
			::SetTextColor(pdis->hDC, ::GetSysColor(COLOR_GRAYTEXT));
		}
	}
	else
	{
		// Draw the left side of the menu
		if (fHasIcon || pdis->itemState & ODS_CHECKED)
		{
			if (fHasIcon)
				DrawIcon(pdis->hDC, rcPix.left + 2, rcPix.top + 2, pmi->iImage, true);
			else
			{
				COLORREF oldColor = ::SetBkColor(pdis->hDC, ::GetSysColor(COLOR_MENU));
				::BitBlt(pdis->hDC, rcPix.left, rcPix.top, rcPix.right, rcPix.bottom,
					m_hdcMem, 0, 0, SRCCOPY);
				::SetBkColor(pdis->hDC, oldColor);
			}
			if (pdis->itemState & ODS_SELECTED)
				::DrawEdge(pdis->hDC, &rcPix, BDR_RAISEDINNER, BF_RECT);
		}
		if (pdis->itemState & ODS_SELECTED)
			crPrevText = ::SetTextColor(pdis->hDC, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
		else
			crPrevText = ::SetTextColor(pdis->hDC, ::GetSysColor(COLOR_MENUTEXT));
	}

	// Draw the text
	if (szPart2)
	{
		::DrawText(pdis->hDC, szPart2, lstrlen(szPart2), &rcText,
			DT_NOCLIP | DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
	}
	::DrawText(pdis->hDC, szText, lstrlen(szText), &rcText,
		DT_NOCLIP | DT_SINGLELINE | DT_VCENTER);

	// Restore the original colors.
	::SetTextColor(pdis->hDC, crPrevText);
	::SetBkColor(pdis->hDC, crPrevBkgnd);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CMenu::DrawIcon(HDC hdc, int nXDest, int nYDest, int nIndex, bool fEnabled)
{
	ImageList_Draw(fEnabled ? m_himl : m_himlDisabled, nIndex, hdc, nXDest, nYDest, 0);
}