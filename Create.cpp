#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


/***********************************************************************************************
	CZEditFrame methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Create and initialize the toolbar.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::CreateToolBars()
{
	TBBUTTON rgtb[] =
	{
		{0,  ID_FILE_NEW,			TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{1,  ID_FILE_OPEN, 			TBSTATE_ENABLED,	TBSTYLE_BUTTON | TBSTYLE_DROPDOWN},
		{0,  0,						TBSTATE_ENABLED,	TBSTYLE_SEP},
		{2,  ID_FILE_SAVE, 			TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{3,  ID_FILE_SAVEALL,		TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{4,  ID_FILE_CLOSE,			TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{5,  ID_FILE_CLOSEALL, 		TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{0,  0,						TBSTATE_ENABLED,	TBSTYLE_SEP},
		{6,  ID_FILE_PRINT,			TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{0,  0,						TBSTATE_ENABLED,	TBSTYLE_SEP},
		{7,  ID_EDIT_CUT,			0, 					TBSTYLE_BUTTON},
		{8,  ID_EDIT_COPY, 			0, 					TBSTYLE_BUTTON},
		{9,  ID_EDIT_PASTE,			0, 					TBSTYLE_BUTTON},
		{0,  0,						TBSTATE_ENABLED,	TBSTYLE_SEP},
		{10, ID_EDIT_UNDO, 			0, 					TBSTYLE_BUTTON},
		{11, ID_EDIT_REDO, 			0, 					TBSTYLE_BUTTON},
		{0,  0,						TBSTATE_ENABLED,	TBSTYLE_SEP},
		{12, ID_EDIT_FIND, 			TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{13, ID_EDIT_FINDPREV, 		0, 					TBSTYLE_BUTTON},
		{14, ID_EDIT_FINDNEXT, 		0, 					TBSTYLE_BUTTON},
		{0,  0,						TBSTATE_ENABLED,	TBSTYLE_SEP},
		{15, ID_TOOLS_INSERTCHAR,	TBSTATE_ENABLED,	TBSTYLE_BUTTON},
		{0,  0,						TBSTATE_ENABLED,	TBSTYLE_SEP},
		{16, ID_HELP_ABOUT,			TBSTATE_ENABLED,	TBSTYLE_BUTTON}
	};

	char * kpszTips[] = {
		"New (Ctrl+N)",
		"Open (Ctrl+O)",
		"Save (Ctrl+S)",
		"Save All",
		"Close",
		"Close All",
		"Print (Ctrl+P)",
		"Cut (Ctrl+X)",
		"Copy (Ctrl+C)",
		"Paste (Ctrl+V)",
		"Undo (Ctrl+Z)",
		"Redo (Ctrl+Y)",
		"Find (Ctrl+F)",
		"Find Previous (Shift+F3)",
		"Find Next (F3)",
		"Insert Characters",
		"About ZEdit"};

	// Create rebar.
	m_hwndRebar = ::CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER |
		RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_AUTOSIZE | CCS_NODIVIDER |
		CCS_NOPARENTALIGN | RBS_DBLCLKTOGGLE, 0, 0, 0, 0, m_hwnd, 0, g_fg.m_hinst, 0);
	if (!m_hwndRebar)
		return false;

	// Create toolbar and attach to rebar.
	m_hwndTool = ::CreateToolbarEx(m_hwndRebar, WS_VISIBLE | TBSTYLE_FLAT |
		TBSTYLE_TOOLTIPS | CCS_NODIVIDER | TBSTYLE_ALTDRAG | CCS_NORESIZE,
		0, kcButtons, g_fg.m_hinst, IDB_TOOLBAR,
		(LPCTBBUTTON)&rgtb, kcButtons + 7, 16, 15, 0, 0, sizeof(TBBUTTON));
	if (!m_hwndTool)
		return false;

	::SendMessage(m_hwndTool, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
	REBARBANDINFO rbbi = { sizeof(rbbi), RBBIM_CHILD | RBBIM_STYLE | RBBIM_CHILDSIZE,
		RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS };
	// For some stupid reason, the size is not calculated correctly as of VS 2008, so we
	// need to fix it. Without setting the size here to 80, the RB_INSERTBAND call below fails.
	rbbi.cbSize = 80;
	rbbi.hwndChild = m_hwndTool;
	RECT rect;
	::GetClientRect(m_hwndTool, &rect);
	rbbi.cxMinChild = 0;//rect.right - rect.left;//200;
	rbbi.cyMinChild = 21;

	if (!::SendMessage(m_hwndRebar, RB_INSERTBAND, -1, (LPARAM)&rbbi))
		return false;

	// Add the disabled image list to the toolbar.
	HIMAGELIST himl = ImageList_Create(16, 15, ILC_COLORDDB | ILC_MASK, 25, 0);
	if (!himl)
		return false;
	HBITMAP hbmpImage = (HBITMAP)::LoadImage(g_fg.m_hinst,
		MAKEINTRESOURCE(IDB_TOOLBAR_DISABLED), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	if (!hbmpImage)
	{
		::DeleteObject(hbmpImage);
		ImageList_Destroy(himl);
		return false;
	}
	ImageList_AddMasked(himl, hbmpImage, 0xC0C0C0);
	::DeleteObject(hbmpImage);
	::SendMessage(m_hwndTool, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himl);
	HDC hdcT = ::GetDC(NULL);
	bool fLowColor = ::GetDeviceCaps(hdcT, BITSPIXEL) == 8;
	::ReleaseDC(NULL, hdcT);
	if (!fLowColor)
	{
		// Add the better 256-color images to the toolbar
		HIMAGELIST himl = ImageList_Create(16, 15, ILC_COLORDDB | ILC_MASK, 25, 0);
		if (!himl)
			return false;
		HBITMAP hbmpImage = (HBITMAP)::LoadImage(g_fg.m_hinst,
			MAKEINTRESOURCE(IDB_TOOLBAR_256), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
		if (!hbmpImage)
		{
			::DeleteObject(hbmpImage);
			ImageList_Destroy(himl);
			return false;
		}
		ImageList_AddMasked(himl, hbmpImage, 0xC0C0C0);
		::DeleteObject(hbmpImage);
		::SendMessage(m_hwndTool, TB_SETIMAGELIST, 0, (LPARAM)himl);
	}

	DWORD dwNewStyle = ::GetWindowLong(m_hwndTool, GWL_STYLE);
	::SetWindowLong(m_hwndTool, GWL_STYLE, dwNewStyle & ~TBSTYLE_TRANSPARENT);
	if (g_fg.m_wpOldToolProc)
	{
		::SetWindowLong(m_hwndTool, GWL_WNDPROC, (long)NewToolProc);
	}
	else
	{
		g_fg.m_wpOldToolProc = (WNDPROC)::SetWindowLong(m_hwndTool, GWL_WNDPROC,
			(long)NewToolProc);
	}

	// Set up the menu toolbar and attach it to the rebar
	m_pzm = new CZMenu(this);
	if (!m_pzm || !m_pzm->Create(::GetMenu(m_hwnd), false))
		return false;

	m_pzmContext = new CZMenu(this);
	HMENU hMenu = ::LoadMenu(g_fg.m_hinst, MAKEINTRESOURCE(IDR_CONTEXT));
	if (!m_pzmContext || !m_pzmContext->Create(hMenu, true))
		return false;

	// Initialize tooltip strings
	HWND hwndTip = (HWND)::SendMessage(m_hwndTool, TB_GETTOOLTIPS, 0, 0);
	int nTips = ::SendMessage(hwndTip, TTM_GETTOOLCOUNT, 0, 0);
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	for (int i = 1; i < nTips; i++)
	{
		::SendMessage(hwndTip, TTM_ENUMTOOLS, i, (long)&ti);
		ti.lpszText = kpszTips[i - 1];
		::SendMessage(hwndTip, TTM_SETTOOLINFO, 0, (long)&ti);
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	Create and initialize the status bar.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::CreateStatus()
{
	m_hwndStatus = ::CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		CCS_BOTTOM | SBARS_SIZEGRIP | SBT_TOOLTIPS, NULL, m_hwnd, 0);
	if (!m_hwndStatus)
		return false;
	::SendMessage(m_hwndStatus, WM_SETFONT, (long)GetStockObject(DEFAULT_GUI_FONT),
		MAKELPARAM(true, 0));

	int rgnWidth[] = {0, 0, 0, 0, 0, -1};
	::SendMessage(m_hwndStatus, SB_SETPARTS, sizeof(rgnWidth) / sizeof(int), (long)rgnWidth);
	SetStatusText(ksboMessage, "Ready");

	SetStatusText(ksboWrap, "Wrap On");

	if (g_fg.m_wpOldStatusProc)
	{
		::SetWindowLong(m_hwndStatus, GWL_WNDPROC, (long)NewStatusProc);
	}
	else
	{
		g_fg.m_wpOldStatusProc = (WNDPROC)::SetWindowLong(m_hwndStatus, GWL_WNDPROC,
			(long)NewStatusProc);
	}
	return true;
}


/***********************************************************************************************
	CZTab methods.
***********************************************************************************************/

WNDPROC CZTab::m_wpOldCaretProc = NULL;

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZTab::CZTab()
{
	m_pzef = NULL;
	m_hwnd = NULL;
	m_himl = NULL;
	m_prgdxp = NULL;
	m_cItems = 0;
	m_iItemSel = -1;
	m_dxpScrollOffset = 0;
	m_dxpMaxScrollOffset = 0;
	m_hwndToolTip = NULL;
	m_nScrollType = kScrollNone;

	m_fDraggingFile = false;
	m_fOutsideStartTab = false;
	m_hcurNew = NULL;
	m_hcurMove = NULL;
	m_fdt = kfdtNone;
	m_iFileDrag = -1;
	m_iFileDrop = -1;
	m_hwndCaret = NULL;
	m_wpOldCaretProc = NULL;
	m_pzefOver = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZTab::~CZTab()
{
	DeleteAllItems();
	if (m_himl)
	{
		ImageList_Destroy(m_himl);
		m_himl = NULL;
	}
	if (m_hwndToolTip)
	{
		::DestroyWindow(m_hwndToolTip);
		m_hwndToolTip = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	Creates a tab control.
----------------------------------------------------------------------------------------------*/
bool CZTab::Create(CZEditFrame * pzef)
{
	if (m_hwnd)
		return false;

	AssertPtr(pzef);
	m_pzef = pzef;

	RECT rc;
	::GetClientRect(pzef->GetHwnd(), &rc);
	m_hwnd = ::CreateWindow("DRZ_TAB", "", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS, 0, 26, rc.right, rc.bottom - 46, pzef->GetHwnd(), NULL,
		g_fg.m_hinst, NULL);
	if (!m_hwnd)
		return false;

	m_hwndToolTip = ::CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP,
		0, 0, 0, 0, NULL, NULL, g_fg.m_hinst, NULL);

	m_himl = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 10);
	if (!m_himl)
		return false;
	ImageList_SetBkColor(m_himl, CLR_NONE);

	::SetWindowLong(m_hwnd, GWL_USERDATA, (LPARAM)this);

	return true;
}


/*----------------------------------------------------------------------------------------------
	Draws the I-beam at the current drop location when dragging a tab.
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZTab::CaretWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_PAINT)
	{
		PAINTSTRUCT ps;
		::BeginPaint(hwnd, &ps);
		HDC hdc = ps.hdc;
		RECT rc;
		::GetClientRect(hwnd, &rc);
		int dxp = rc.right - rc.left;
		int dyp = rc.bottom - rc.top - 1;
		::MoveToEx(hdc, 0, 0, NULL);
		::LineTo(hdc, dxp, 0); // top horizontal line
		::MoveToEx(hdc, 1, 1, NULL);
		::LineTo(hdc, dxp - 1, 1); // second top horizontal line
		::MoveToEx(hdc, 2, 2, NULL);
		::LineTo(hdc, 2, dyp - 1); // first vertical line
		::MoveToEx(hdc, 3, 2, NULL);
		::LineTo(hdc, 3, dyp - 1); // second vertical line
		::MoveToEx(hdc, 1, dyp - 1, NULL);
		::LineTo(hdc, dxp - 1, dyp - 1); // second bottom horizontal line
		::MoveToEx(hdc, 0, dyp, NULL);
		::LineTo(hdc, dxp, dyp); // bottom horizontal line
		::EndPaint(hwnd, &ps);
	}
	return ::CallWindowProc(m_wpOldCaretProc, hwnd, uMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	Window procedure for the main tab control.
----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK CZTab::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CZTab * pzt = (CZTab *)::GetWindowLong(hwnd, GWL_USERDATA);

	switch (iMsg)
	{
	case WM_ERASEBKGND:
		return 0;

	case WM_PAINT:
		return pzt->OnPaint();

	case WM_SIZE:
		::InvalidateRect(hwnd, NULL, false);
		::UpdateWindow(hwnd);
		return 0;

	case WM_LBUTTONDOWN:
		return pzt->OnLButtonDown(wParam, lParam);

	case WM_MOUSEMOVE:
		return pzt->OnMouseMove(wParam, lParam);

	case WM_LBUTTONUP:
		return pzt->OnLButtonUp(wParam, lParam);

	case WM_RBUTTONUP:
		return pzt->OnRButtonUp(wParam, lParam);

	case WM_TIMER:
		return pzt->OnTimer(wParam, lParam);

	case WM_MOUSELEAVE:
		return pzt->OnMouseLeave(wParam, lParam);

	case WM_KEYDOWN:
		return pzt->OnKeyDown(wParam, lParam);
	}

	return ::DefWindowProc(hwnd, iMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	if (m_nScrollType != kScrollNone)
	{
		::KillTimer(m_hwnd, knScrollTimerID);
		m_nScrollType = kScrollNone;
		::InvalidateRect(m_hwnd, NULL, false);
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	if (m_fDraggingFile && wParam == VK_ESCAPE)
	{
		// Cancel the drag.
		m_fdt = kfdtNone;
		OnLButtonUp(0, 0);
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnTimer(WPARAM wParam, LPARAM lParam)
{
	if (wParam == knScrollTimerID)
	{
		int dxpScrollOffset = m_dxpScrollOffset;
		if (m_nScrollType == kScrollLeft)
			dxpScrollOffset -= 5;
		else if (m_nScrollType == kScrollRight)
			dxpScrollOffset += 5;
		else
		{
			::KillTimer(m_hwnd, knScrollTimerID);
			return 0;
		}
		if (dxpScrollOffset < 0)
			dxpScrollOffset = 0;
		if (dxpScrollOffset > m_dxpMaxScrollOffset)
			dxpScrollOffset = m_dxpMaxScrollOffset;
		if (dxpScrollOffset != m_dxpScrollOffset)
		{
			m_dxpScrollOffset = dxpScrollOffset;
			::InvalidateRect(m_hwnd, NULL, false);
			::UpdateWindow(m_hwnd);
		}
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hwnd, &ps);

	RECT rc, rcTabs;
	::GetClientRect(m_hwnd, &rc);
	rc.bottom = rc.top + kdypHeight;
	GetTabRect(&rcTabs);

	HDC hdcMem = ::CreateCompatibleDC(NULL);
	if (hdcMem)
	{
		HBITMAP hbmp = ::CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
		if (hbmp)
		{
			COLORREF crText = RGB(100, 100, 100);
			COLORREF crSelText = RGB(0, 0, 0);
			COLORREF crBkColor = RGB(247, 243, 233);

			HBITMAP hbmpOld = (HBITMAP)::SelectObject(hdcMem, hbmp);

			::SetBkColor(hdcMem, crBkColor);
			::ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
			::SetBkMode(hdcMem, TRANSPARENT);

			LOGFONT lf;
			HFONT hfont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
			::GetObject(hfont, sizeof(lf), &lf);
			lf.lfWeight = FW_SEMIBOLD;
			HFONT hfontSel = ::CreateFontIndirect(&lf);
			HPEN hpen = ::CreatePen(PS_SOLID, 1, crText);
			HPEN hpenSel = ::CreatePen(PS_SOLID, 1, crSelText);
			::SetTextColor(hdcMem, crText);
			::SetBkColor(hdcMem, ::GetSysColor(COLOR_3DFACE));

			RECT rcBorder = { rc.left, rc.bottom - 2, rc.right, rc.bottom };
			::ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rcBorder, NULL, 0, NULL);
			::SetRect(&rcBorder, 0, 0, rc.right, rc.bottom);
			::DrawEdge(hdcMem, &rcBorder, EDGE_ETCHED, BF_LEFT | BF_RIGHT);

			HPEN hpenOld = (HPEN)::SelectObject(hdcMem, hpen);
			HFONT hfontOld = (HFONT)::SelectObject(hdcMem, hfont);

			if (m_prgdxp)
				delete m_prgdxp;
			m_prgdxp = new int[m_cItems + 1];

			// Remove all the tabs from the tooltip control.
			TOOLINFO ti = { sizeof(ti), TTF_SUBCLASS };
			ti.hwnd = m_hwnd;
			int cTool = ::SendMessage(m_hwndToolTip, TTM_GETTOOLCOUNT, 0, 0);
			for (int iTool = 0; iTool < cTool; iTool++)
			{
				ti.uId = iTool;
				::SendMessage(m_hwndToolTip, TTM_DELTOOL, 0, (LPARAM)&ti);
			}

			::IntersectClipRect(hdcMem, rcTabs.left, rcTabs.top, rcTabs.right, rcTabs.bottom);

			// Draw the tabs.
			m_prgdxp[0] = -m_dxpScrollOffset + rcTabs.left;
			CZDoc * pzd = m_pzef->GetFirstDoc();
			for (int iItem = 0; iItem < m_cItems; iItem++, pzd = pzd->Next())
			{
				int & xpLeft = m_prgdxp[iItem];
				int & xpRight = m_prgdxp[iItem + 1];

				AssertPtr(pzd);
				char * pszPath = pzd->GetFilename();
				char * pszFile = strrchr(pszPath, '\\');
				char szFilename[MAX_PATH];
				strcpy_s(szFilename, pszFile ? pszFile + 1 : pszPath);
				if (pzd->GetModify())
					strcat_s(szFilename, " *");
				int cch = strlen(szFilename);

				if (iItem == m_iItemSel)
				{
					::SelectObject(hdcMem, hpenSel);
					::SelectObject(hdcMem, hfontSel);
					::SetTextColor(hdcMem, crSelText);
				}

				RECT rcItem;
				::SetRect(&rcItem, xpLeft, rcTabs.top, rcTabs.right, rcTabs.bottom);
				::DrawText(hdcMem, szFilename, cch, &rcItem,
					DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
				xpRight = rcItem.right + (kdxpMargin * 3) + 16;

				::SetRect(&rcItem, xpLeft, rcTabs.top, xpRight, rcTabs.bottom);
				if (rcItem.right > rcTabs.right)
					rcItem.right = rcTabs.right;
				if (iItem == m_iItemSel)
					::ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rcItem, NULL, 0, NULL);

				// Add this tab to the tooltip control.
				ti.lpszText = pszPath;
				ti.uId = iItem;
				ti.rect = rcItem;
				::SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

				ImageList_Draw(m_himl, pzd->GetImageIndex(), hdcMem,
					xpLeft + kdxpMargin, 2, ILD_NORMAL);

				rcItem.left += (kdxpMargin * 2) + 16;

				::DrawText(hdcMem, szFilename, cch, &rcItem,
					DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_LEFT);

				if (iItem == m_iItemSel)
				{
					::MoveToEx(hdcMem, xpRight, 1, NULL);
					::LineTo(hdcMem, xpRight, kdypHeight - 2);

					::SelectObject(hdcMem, hfont);
					::SelectObject(hdcMem, hpen);
					::SetTextColor(hdcMem, crText);
				}
				else
				{
					::MoveToEx(hdcMem, xpRight, 2, NULL);
					::LineTo(hdcMem, xpRight, kdypHeight - 3);
				}
			}

			::SelectClipRgn(hdcMem, NULL);

			m_dxpMaxScrollOffset = (m_prgdxp[m_cItems] - m_prgdxp[0]) - (rcTabs.right - rcTabs.left) + 1;
			if (m_dxpMaxScrollOffset < 0)
				m_dxpMaxScrollOffset = 0;

			bool fRedraw = false;
			if (m_dxpScrollOffset > m_dxpMaxScrollOffset)
			{
				fRedraw = true;
				m_dxpScrollOffset = m_dxpMaxScrollOffset;
				::InvalidateRect(m_hwnd, NULL, false);
			}
			else
			{
				bool fLeftEnabled = m_dxpScrollOffset > 0;
				bool fRightEnabled = m_dxpScrollOffset < m_dxpMaxScrollOffset;
				DrawArrow(hdcMem, true, rcTabs.right + 4, 6, fLeftEnabled);
				DrawArrow(hdcMem, false, rcTabs.right + 18, 6, fRightEnabled);

				bool fButtonDown = ::GetAsyncKeyState(VK_LBUTTON) < 0;
				if (m_nScrollType == kScrollLeft && fLeftEnabled)
				{
					RECT rcT = { rcTabs.right, 2, rcTabs.right + 14, rcTabs.bottom - 1 };
					::DrawEdge(hdcMem, &rcT, fButtonDown ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_SOFT);
				}
				else if (m_nScrollType == kScrollRight && fRightEnabled)
				{
					RECT rcT = { rcTabs.right + 14, 2, rcTabs.right + 28, rcTabs.bottom - 1 };
					::DrawEdge(hdcMem, &rcT, fButtonDown ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_SOFT);
				}

				::BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
				::ValidateRect(m_hwnd, NULL);
			}

			::SelectObject(hdcMem, hpenOld);
			::SelectObject(hdcMem, hbmpOld);
			::SelectObject(hdcMem, hfontOld);
			::DeleteObject(hpen);
			::DeleteObject(hpenSel);
			::DeleteObject(hfontSel);

			::DeleteObject(hbmp);
		}
		::DeleteDC(hdcMem);
	}

	::EndPaint(m_hwnd, &ps);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZTab::DrawArrow(HDC hdc, bool fLeft, int xp, int yp, bool fFilledIn)
{
	if (fLeft)
	{
        ::MoveToEx(hdc, xp + 4, yp, NULL);
		::LineTo(hdc, xp, yp + 4);
		::LineTo(hdc, xp + 4, yp + 8);
		::LineTo(hdc, xp + 4, yp + 1);
		if (fFilledIn)
		{
			::LineTo(hdc, xp + 1, yp + 4);
			::LineTo(hdc, xp + 3, yp + 6);
			::LineTo(hdc, xp + 3, yp + 3);
			::LineTo(hdc, xp + 1, yp + 5);
		}
		else
		{
			// The triangle won't be closed if we don't do this.
			::LineTo(hdc, xp + 4, yp);
		}
	}
	else
	{
        ::MoveToEx(hdc, xp, yp, NULL);
		::LineTo(hdc, xp + 4, yp + 4);
		::LineTo(hdc, xp, yp + 8);
		::LineTo(hdc, xp, yp + 1);
		if (fFilledIn)
		{
			::LineTo(hdc, xp + 3, yp + 4);
			::LineTo(hdc, xp + 1, yp + 6);
			::LineTo(hdc, xp + 1, yp + 3);
			::LineTo(hdc, xp + 3, yp + 5);
		}
		else
		{
			// The triangle won't be closed if we don't do this.
			::LineTo(hdc, xp, yp);
		}
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	if (m_nScrollType != kScrollNone)
	{
		::SetTimer(m_hwnd, knScrollTimerID, 10, NULL);
		return 0;
	}

	POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
	m_iFileDrag = HitTest(pt);
	if (m_iFileDrag == -1)
		return 0;
	m_iFileDrop = m_iFileDrag;
	m_pzefOver = m_pzef;

	::SetCapture(m_hwnd);

	m_fDraggingFile = true;
	m_fOutsideStartTab = false;
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	if (!m_fDraggingFile)
	{
		// See if we're over the scroll arrows.
		RECT rc;
		::GetClientRect(m_hwnd, &rc);
		POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) }; 
		int nScrollType = kScrollNone;
		if (pt.x > rc.right - 28)
		{
			if (pt.y >= 3 && pt.y <= rc.bottom - 3)
			{
				if (pt.x > rc.right - 14)
					nScrollType = kScrollRight;
				else
					nScrollType = kScrollLeft;
			}
			if (m_nScrollType == kScrollNone && nScrollType != kScrollNone)
			{
				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hwnd, 0 };
				::TrackMouseEvent(&tme);
			}
		}
		if (m_nScrollType != nScrollType)
		{
			m_nScrollType = nScrollType;
			::InvalidateRect(m_hwnd, NULL, false);
		}
		return 0;
	}

	m_nScrollType = kScrollNone;
	POINT pt;
	if (!::GetCursorPos(&pt))
		return 0;

	if (!m_fOutsideStartTab)
	{
		// See if we've moved the mouse out of the tab we clicked on.
		RECT rc;
		if (!GetItemRect(m_iFileDrag, &rc))
			return 0;
		::MapWindowPoints(m_hwnd, NULL, (POINT *)&rc, 2);
		if (::PtInRect(&rc, pt))
			return 0;

		m_fOutsideStartTab = true;

		::SetFocus(m_hwnd);
		m_hcurNew = ::LoadCursor(g_fg.m_hinst, MAKEINTRESOURCE(IDC_NEWZEDIT));
		m_hcurMove = ::LoadCursor(g_fg.m_hinst, MAKEINTRESOURCE(IDC_MOVEFILE));
		m_fdt = kfdtNone;
		if (m_hwndCaret == NULL)
		{
			m_hwndCaret = ::CreateWindow("STATIC", NULL, WS_POPUP, 0, 0, 0, 0, m_hwnd,
				NULL, NULL, NULL);
			if (m_wpOldCaretProc)
			{
				::SetWindowLong(m_hwndCaret, GWL_WNDPROC, (long)CaretWndProc);
			}
			else
			{
				m_wpOldCaretProc = (WNDPROC)::SetWindowLong(m_hwndCaret, GWL_WNDPROC,
					(long)CaretWndProc);
			}
		}
	}

	// Get the top-most window that the cursor is over.
	HWND hwndOver = ::WindowFromPoint(pt);
	HWND hwndT = ::GetParent(hwndOver);
	while (hwndT)
	{
		hwndOver = hwndT;
		hwndT = ::GetParent(hwndOver);
	}

	// See if it's one of our windows.
	char rgchClass[MAX_PATH];
	if (!::GetClassName(hwndOver, rgchClass, sizeof(rgchClass) / sizeof(char)))
		return 0;
	m_pzefOver = NULL;
	if (strcmp(rgchClass, g_pszAppName) == 0)
		m_pzefOver = (CZEditFrame *)::GetWindowLong(hwndOver, GWL_USERDATA);
	if (m_pzefOver)
	{
		m_fdt = kfdtMove;
		::SetCursor(m_hcurMove);

		CZTab * pzt = m_pzefOver->GetTab();
		HWND hwndTab = pzt->GetHwnd();
		::ScreenToClient(hwndTab, &pt);

		RECT rc;
		int iFileSel = pzt->GetCurSel();
		pzt->GetItemRect(iFileSel, &rc);
		int iFile = 0;
		if (pt.y < rc.top || pt.y > rc.bottom)
		{
			// Put it after the current tab.
			iFile = iFileSel;
		}
		else
		{
			// Find out which tab we're currently over.
			iFile = pzt->HitTest(pt);

			if (iFile == -1)
			{
				if (pt.x < rc.left)
				{
					// Put it before the first tab.
					iFile = -1;
				}
				else
				{
					// Put it after the last tab.
					iFile = pzt->GetItemCount() - 1;
				}
			}
			else
			{
				pzt->GetItemRect(iFile, &rc);
				if (pt.x <= ((rc.right - rc.left) / 2 + rc.left))
				{
					// We're less than halfway past the tab, so put the caret before the tab.
					iFile--;
				}
			}
		}

		POINT ptCaret;
		if (iFile < 0)
		{
			ptCaret.x = 0;
		}
		else
		{
			pzt->GetItemRect(iFile, &rc);
			ptCaret.x = rc.right - 2;
		}
		m_iFileDrop = iFile + 1;
		ptCaret.y = rc.top - 1;

		::ClientToScreen(hwndTab, &ptCaret);
		if (m_pzef == m_pzefOver)
		{
			// This window is currently topmost, so we want to make the cursor show on top of it.
			::SetWindowPos(m_hwndCaret, HWND_TOPMOST, ptCaret.x, ptCaret.y,
				6, rc.bottom - rc.top + 2, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS);
		}
		else
		{
			// If the mouse position is outside the rectangle of this window, but the caret
			// is inside the rectangle of this window, we don't want it to show on top of
			// this window, so we place it in the z-order below the current window.
			::SetWindowPos(m_hwndCaret, m_pzef->GetHwnd(), ptCaret.x, ptCaret.y,
				6, rc.bottom - rc.top + 2, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS);
		}
		::InvalidateRect(m_hwndCaret, NULL, true);
	}
	else
	{
		m_fdt = kfdtNewWindow;
		::SetCursor(m_hcurNew);
		::ShowWindow(m_hwndCaret, SW_HIDE);
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_nScrollType != kScrollNone)
	{
		::KillTimer(m_hwnd, knScrollTimerID);
		::InvalidateRect(m_hwnd, NULL, true);
		return 0;
	}

	if (!m_fDraggingFile)
		return 0;

	::ReleaseCapture();
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));

	if (m_hcurNew)
	{
		::DestroyCursor(m_hcurNew);
		m_hcurNew = NULL;
	}
	if (m_hcurMove)
	{
		::DestroyCursor(m_hcurMove);
		m_hcurMove = NULL;
	}
	m_fDraggingFile = false;
	m_fOutsideStartTab = false;

	CZDoc * pzd = m_pzef->GetFirstDoc();
	int iT = m_iFileDrag;
	while (iT-- > 0 && pzd)
		pzd = pzd->Next();
	AssertPtr(pzd);
	CZDoc * pzdPrev = pzd->Prev();
	CZDoc * pzdNext = pzd->Next();

	if (m_pzef == m_pzefOver &&
		(m_iFileDrag == m_iFileDrop || m_iFileDrag == m_iFileDrop - 1))
	{
		m_pzef->SelectFile(m_iFileDrag);
		m_fdt = kfdtNone;
	}

	if (m_hwndCaret)
	{
		::SetWindowLong(m_hwndCaret, GWL_WNDPROC, (long)m_wpOldCaretProc);
		::DestroyWindow(m_hwndCaret);
		m_hwndCaret = NULL;
	}

	if (m_fdt == kfdtNewWindow)
	{
		// We want to create a new ZEdit window and move the
		// selected tab to that window.
		Assert(m_pzefOver == NULL);
		m_pzefOver = new CZEditFrame();
		if (m_pzef->RemoveFile(m_iFileDrag, pzd, pzdPrev, pzdNext) &&
			m_pzefOver->Create(NULL, pzd, false))
		{
			// Position the new window based on the drop position.
			POINT ptDrop;
			::GetCursorPos(&ptDrop);
			::SetWindowPos(m_pzefOver->GetHwnd(), NULL, ptDrop.x, ptDrop.y, 0, 0,
				SWP_NOSIZE | SWP_NOZORDER);

			g_pzea->AddFrame(m_pzefOver);
			g_pzea->SetCurrentFrame(m_pzefOver);
			::SetForegroundWindow(m_pzefOver->GetHwnd());

			// RemoveFile hides the previous frame window if it was the active
			// window, so we need to redisplay it here.
			::ShowWindow(pzd->GetFrame()->GetHwnd(), SW_SHOW);

			if (m_pzef != m_pzefOver)
			{
				// Show the top-level window.
				HWND hwnd = m_pzefOver->GetHwnd();
				::ShowWindow(hwnd, SW_SHOW);
				::UpdateWindow(hwnd);
			}
		}
	}
	else if (m_fdt == kfdtMove)
	{
		if (m_pzef == m_pzefOver)
		{
			// We dropped the file in the same window that we dragged it from.
			if (m_iFileDrop > m_iFileDrag)
				m_iFileDrop--;
			if (m_pzef->RemoveFile(m_iFileDrag, pzd, pzdPrev, pzdNext) &&
				m_pzef->InsertFile(m_iFileDrop, pzd, pzd->GetFilename()))
			{
				m_pzef->SelectFile(m_iFileDrop);
			}
		}
		else
		{
			// We dropped the file in a different window than we dragged it from.
			if (m_pzef->RemoveFile(m_iFileDrag, pzd, pzdPrev, pzdNext))
			{
				pzd->SetImageIndex(-1);
				if (m_pzefOver->InsertFile(m_iFileDrop, pzd, pzd->GetFilename()))
				{
					pzd->AttachToMainFrame(m_pzefOver);
					m_pzefOver->SelectFile(m_iFileDrop);
				}
			}
		}
	}

	if (m_fdt != kfdtNone)
		::SetFocus(m_pzefOver->GetHwnd());

	m_fdt = kfdtNone;

	m_iFileDrag = -1;
	m_iFileDrop = -1;
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZTab::OnRButtonUp(WPARAM wParam, LPARAM lParam)
{
	POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) }; 
	int iTab = HitTest(pt);
	if (iTab != -1)
	{
		HMENU hMenu = m_pzef->GetContextMenu()->GetSubMenu(kcmTabs);
		CZDoc * pzd = m_pzef->GetDoc(iTab);
		DWORD dwAttributes = ::GetFileAttributes(pzd->GetFilename());
		::EnableMenuItem(hMenu, ID_TABS_OPENFOLDER,
			dwAttributes == -1 ? MF_DISABLED : MF_ENABLED);

		::ClientToScreen(m_hwnd, &pt);
		int nCommand = ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON |
			TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, m_pzef->GetHwnd(), NULL);
		if (nCommand == 0)
			return FALSE;

		AssertPtr(pzd);
		switch (nCommand)
		{
		default:
			break;
		case ID_TABS_CLOSE:
			return m_pzef->CloseFile(iTab);
		case ID_TABS_CLOSEALL:
			return m_pzef->CloseAll(false);
		case ID_TABS_CLOSEEXCEPT:
			return m_pzef->CloseAll(false, iTab);
		case ID_TABS_SAVE:
			return m_pzef->SaveFile(pzd, iTab, false, NULL);
		case ID_TABS_SAVEAS:
			return m_pzef->SaveFile(pzd, iTab, true, NULL);
		case ID_TABS_SAVEALL:
			return m_pzef->SaveAll();
		case ID_TABS_OPENFOLDER:
			{
				char rgch[MAX_PATH * 2];
				sprintf_s(rgch, "/select,\"%s\"", pzd->GetFilename());
				::ShellExecute(NULL, "open", "explorer.exe", rgch, NULL, SW_RESTORE);
				return 0;
			}
		case ID_TABS_SWITCHTO:
			m_pzef->SelectFile(iTab);
			break;
		case ID_TABS_PROPERTIES:
			return m_pzef->OnFileProperties(pzd);
		}
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	iItem is 0 based.
	If fNewWindow is true, iItem can be between 0 and m_cItems.
	If fNewWindow is false, iItem can be between 0 and m_cItems - 1.
----------------------------------------------------------------------------------------------*/
void CZTab::SetItemImage(int iItem, char * pszFilename, bool fNewWindow)
{
	Assert(iItem >= 0 && (iItem < m_cItems || (fNewWindow && iItem <= m_cItems)));

	if (fNewWindow)
		m_cItems++;
	FindIcon(iItem, pszFilename, fNewWindow);

	if (m_pzef->GetTabMgrHwnd())
	{
		// Update the list of open files.
		CZDoc * pzd = m_pzef->GetDoc(iItem);
		HWND hwndList = ::GetDlgItem(m_pzef->GetTabMgrHwnd(), IDC_LISTVIEW);
		LVITEM lvi = { LVIF_TEXT | LVIF_IMAGE };
		lvi.pszText = pszFilename;
		lvi.iImage = pzd->GetImageIndex();
		lvi.iItem = iItem;
		if (fNewWindow)
			ListView_InsertItem(hwndList, &lvi);
		else
			ListView_SetItem(hwndList, &lvi);
		ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
	}

	Refresh();
}


/*----------------------------------------------------------------------------------------------
	iItem is 0 based.
----------------------------------------------------------------------------------------------*/
void CZTab::RemoveItem(int iItem)
{
	Assert(iItem >= 0 && iItem < m_cItems);

	if (m_iItemSel >= --m_cItems)
		m_iItemSel = m_cItems - 1;

	CZDoc * pzd = m_pzef->GetDoc(iItem);
	int iImageIndex = pzd->GetImageIndex();
	ImageList_Remove(m_himl, iImageIndex);
	int cImages = ImageList_GetImageCount(m_himl);

	HWND hwndList = NULL;
	if (m_pzef->GetTabMgrHwnd())
		hwndList = ::GetDlgItem(m_pzef->GetTabMgrHwnd(), IDC_LISTVIEW);

	// Update all the doc image indexes.
	for (pzd = m_pzef->GetFirstDoc(); pzd; pzd = pzd->Next())
	{
		int iT = pzd->GetImageIndex();
		if (iT > iImageIndex)
		{
			pzd->SetImageIndex(iT - 1);

			if (hwndList)
			{
				LVITEM lvi = { LVIF_IMAGE };
				lvi.iImage = iT - 1;
				ListView_SetItem(hwndList, &lvi);
			}
		}
	}

	if (hwndList)
		ListView_DeleteItem(hwndList, iItem);

	Refresh(false);
}


/*----------------------------------------------------------------------------------------------
	Get the icon for the specified file and add it to the imagelist.
	If iItem = -1, add the icon to the end of the imagelist.
----------------------------------------------------------------------------------------------*/
void CZTab::FindIcon(int iItem, char * pszFilename, bool fNewWindow)
{
	AssertPtr(pszFilename);

	SHFILEINFO fi = { 0 };
	::SHGetFileInfo(pszFilename, FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	if (!fi.hIcon)
	{
		USHORT nIcon = 0;
		char szIconFile[MAX_PATH];
		lstrcpy(szIconFile, pszFilename);
		fi.hIcon = ::ExtractAssociatedIcon(g_fg.m_hinst, szIconFile, &nIcon);
	}
	if (fi.hIcon)
	{
		CZDoc * pzd = m_pzef->GetDoc(iItem);
		int nIndex = fNewWindow ? -1 : pzd->GetImageIndex();
		nIndex = ImageList_ReplaceIcon(m_himl, nIndex, fi.hIcon);
		::DestroyIcon(fi.hIcon);
		if (nIndex != -1)
			pzd->SetImageIndex(nIndex);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZTab::DeleteAllItems()
{
	m_cItems = 0;
	m_iItemSel = -1;
	ImageList_RemoveAll(m_himl);
	if (m_prgdxp)
	{
		delete m_prgdxp;
		m_prgdxp = NULL;
	}

	Refresh();
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CZTab::HitTest(POINT pt)
{
	for (int iItem = 0; iItem < m_cItems; iItem++)
	{
		if (pt.x >= m_prgdxp[iItem] && pt.x < m_prgdxp[iItem + 1])
			return iItem;
	}
	return -1;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZTab::GetItemRect(int iItem, RECT * prc)
{
	AssertPtr(prc);
	if (iItem < 0 || iItem >= m_cItems || !m_prgdxp)
		return false;

	::SetRect(prc, m_prgdxp[iItem], 0, m_prgdxp[iItem + 1], kdypHeight);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZTab::SetCurSel(int iItem)
{
	m_iItemSel = iItem;

	// Make sure this file is visible.
	RECT rc, rcTabs;
	if (!GetItemRect(iItem, &rc))
		return;
	GetTabRect(&rcTabs);
	if (rc.left < rcTabs.left)
		m_dxpScrollOffset -= (rcTabs.left - rc.left);
	else if (rc.right > rcTabs.right - 15)
		m_dxpScrollOffset += (rc.right - rcTabs.right + 15);
	if (m_dxpScrollOffset < 0)
		m_dxpScrollOffset = 0;

	Refresh();
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZTab::GetClientRect(RECT * prc)
{
	AssertPtr(prc);
	::GetClientRect(m_hwnd, prc);
	prc->top += kdypHeight;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZTab::GetTabRect(RECT * prc)
{
	RECT rc;
	::GetClientRect(m_hwnd, &rc);
	rc.bottom = rc.top + kdypHeight;
	::SetRect(prc, rc.left + 2, rc.top + 1, rc.right - 2 - 28, rc.bottom - 2);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZTab::Refresh(bool fUpdateNow)
{
	::InvalidateRect(m_hwnd, NULL, false);
	if (fUpdateNow)
		::UpdateWindow(m_hwnd);
}