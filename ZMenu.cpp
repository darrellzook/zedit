#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
CZMenu::CZMenu(CZEditFrame * pzef)
	: CMenu(pzef->GetHwnd(), g_fg.m_fCanUseVer5)
{
	m_pzef = pzef;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZMenu::Create(HMENU hMenu, bool fReleaseMenu)
{
	HINSTANCE hinst = (HINSTANCE)GetWindowLong(m_pzef->GetHwnd(), GWL_HINSTANCE);
	HDC hdcT = GetDC(NULL);
	bool fLowColor = GetDeviceCaps(hdcT, BITSPIXEL) == 8;
	ReleaseDC(NULL, hdcT);

	HBITMAP hbmp;
	if (fLowColor)
		hbmp = (HBITMAP)LoadImage(g_fg.m_hinst, MAKEINTRESOURCE(IDB_TOOLBAR), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	else
		hbmp = (HBITMAP)LoadImage(g_fg.m_hinst, MAKEINTRESOURCE(IDB_TOOLBAR_256), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	HBITMAP hbmpDisabled = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_TOOLBAR_DISABLED));
	bool fSuccess = CMenu::Create(hMenu, fReleaseMenu, hbmp, hbmpDisabled);
	DeleteObject(hbmp);
	DeleteObject(hbmpDisabled);
	if (fSuccess && !fReleaseMenu)
	{
		MyItem * pmi;
		MENUITEMINFO mii = { 0 };
		if (g_fg.m_fCanUseVer5)
			mii.cbSize = sizeof(MENUITEMINFO);
		else
			mii.cbSize = CDSIZEOF_STRUCT(MENUITEMINFOA,cch);
		mii.fMask = MIIM_DATA;
		::GetMenuItemInfo(GetSubMenu(0), 0, TRUE, &mii);
		pmi = (MyItem*)mii.dwItemData;
		if (pmi)
		{
			TBBUTTONINFO tbbi = {sizeof(tbbi), TBIF_IMAGE};
			::SendMessage(m_pzef->GetToolbarHwnd(), TB_GETBUTTONINFO, ID_FILE_NEW, (long)&tbbi);
			pmi->iImage = tbbi.iImage;
		}
	}
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZMenu::OnDrawItem(LPDRAWITEMSTRUCT lpdis)
{
	CZDoc * pzd = m_pzef->GetCurrentDoc();
	MyItem * pmi = (MyItem *)lpdis->itemData;

	// Update menu item text if needed
	char * rgsz[11] =
	{
		{""},
		{" Cut"},
		{" Paste"},
		{" Typing"},
		{" Replace"},
		{" Replace All"},
		{" Clear"},
//		{" Spelling Change"},
		{" Increase Indent"},
		{" Decrease Indent"},
		{" Smart Indent"},
		{" Move"}
	};

	switch (lpdis->itemID)
	{
	case ID_EDIT_UNDO:
		// Show proper undo option
		if (pzd->CanUndo())
		{
			::EnableMenuItem(pmi->hMenu, ID_EDIT_UNDO, MF_ENABLED);
			lpdis->itemState &= ~ODS_DISABLED;
			wsprintf(pmi->szItemText, "Undo%s\tCtrl+Z", rgsz[pzd->GetUndoType()]);
		}
		else
		{
			::EnableMenuItem(pmi->hMenu, ID_EDIT_UNDO, MF_GRAYED);
			lpdis->itemState |= ODS_DISABLED;
			lstrcpy(pmi->szItemText, "Can't &Undo");
		}
		break;

	case ID_EDIT_REDO:
		// Show proper redo option
		if (pzd->CanRedo())
		{
			::EnableMenuItem(pmi->hMenu, ID_EDIT_REDO, MF_ENABLED);
			lpdis->itemState &= ~ODS_DISABLED;
			wsprintf(pmi->szItemText, "Redo%s\tCtrl+Y", rgsz[pzd->GetRedoType()]);
		}
		else
		{
			::EnableMenuItem(pmi->hMenu, ID_EDIT_REDO, MF_GRAYED);
			lpdis->itemState |= ODS_DISABLED;
			lstrcpy(pmi->szItemText, "Can't &Redo");
		}
		break;
	}
	return CMenu::OnDrawItem(lpdis);
}