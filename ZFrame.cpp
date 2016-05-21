#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZFrame::CZFrame()
{
	m_hwnd = m_hwndParent = NULL;
	m_pzef = NULL;
	m_pzd = NULL;
	m_fBinary = false;
	m_fResizing = false;
	m_nTopHalfPercent = 5000;
	m_fIsSplit = false;
	m_rgpzv[0] = NULL;
	m_rgpzv[1] = NULL;
	m_rghwndHScroll[0] = NULL;
	m_rghwndHScroll[1] = NULL;
	m_rghwndVScroll[0] = NULL;
	m_rghwndVScroll[1] = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZFrame::~CZFrame()
{
	::ShowWindow(m_hwnd, SW_HIDE);
	AssertPtr(m_rgpzv[0]);
	delete m_rgpzv[0];
	if (m_fIsSplit)
	{
		AssertPtr(m_rgpzv[1]);
		delete m_rgpzv[1];
	}
	::DestroyWindow(m_hwnd);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZFrame::Create(CZEditFrame * pzef, CZDoc * pzd)
{
	AssertPtr(pzef);
	AssertPtr(pzd);

	m_hwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE, "DRZ_Frame", "", 0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (!m_hwnd)
		return false;
	m_pzd = pzd;

	m_rghwndVScroll[0] = ::CreateWindow("DRZ_Scrollbar", "", WS_VISIBLE | WS_CHILD |
		WS_VSCROLL, 0, 0, 0, 0, m_hwnd, NULL, g_fg.m_hinst, 0);
	m_rghwndHScroll[0] = ::CreateWindow("DRZ_Scrollbar", "", WS_VISIBLE | WS_CHILD |
		WS_HSCROLL, 0, 0, 0, 0, m_hwnd, NULL, g_fg.m_hinst, 0);

	::SetWindowLong(m_rghwndVScroll[0], GWL_USERDATA, (long)this);
	::SetWindowLong(m_rghwndHScroll[0], GWL_USERDATA, (long)this);

	if (g_fg.m_fFlatScrollbars)
	{
		InitializeFlatSB(m_rghwndVScroll[0]);
		InitializeFlatSB(m_rghwndHScroll[0]);
	}

	CZView *& pzv = m_rgpzv[0];
	m_fBinary = m_pzd->GetFileType() == kftBinary;
	if (m_fBinary)
		pzv = new CHexEdit(pzef);
	else
		pzv = new CFastEdit(pzef);
	if (!pzv->Create(this, m_hwnd, WS_CHILD | WS_VISIBLE, 0))
		return false;

	m_pzvCurrent = pzv;

	::SetWindowLong(m_hwnd, GWL_USERDATA, (long)this);

	AttachToMainFrame(pzef);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZFrame::AttachToMainFrame(CZEditFrame * pzef)
{
	::SetParent(m_hwnd, pzef->GetTab()->GetHwnd());
	::SetWindowLong(m_hwnd, GWL_STYLE, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		WS_CLIPCHILDREN);

	RECT rc;
	pzef->GetTab()->GetClientRect(&rc);
	::SetWindowPos(m_hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		SWP_NOZORDER);
	::ShowWindow(m_hwnd, SW_SHOW);

	m_pzef = pzef;

	AssertPtr(m_rgpzv[0]);
	m_rgpzv[0]->SetMainFrame(pzef);
	if (m_fIsSplit)
	{
		AssertPtr(m_rgpzv[1]);
		m_rgpzv[1]->SetMainFrame(pzef);
	}

	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZFrame::SetCurrentView(CZView * pzv)
{
	AssertPtr(pzv);
	AssertPtr(m_pzd);
	Assert(m_rgpzv[0] == pzv || m_rgpzv[1] == pzv);

	m_pzvCurrent = pzv;
	m_pzd->SetCurrentView(pzv);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZFrame::ToggleCurrentView()
{
	AssertPtr(m_pzd);
	AssertPtr(m_pzvCurrent);

	m_fBinary = !m_fBinary;

	Assert(m_rgpzv[0] == m_pzvCurrent || m_rgpzv[1] == m_pzvCurrent);
	int izv = m_rgpzv[1] == m_pzvCurrent;

	// Create the new view.
	CZView *& pzv = m_rgpzv[izv];
	if (m_fBinary)
		pzv = new CHexEdit(m_pzef);
	else
		pzv = new CFastEdit(m_pzef);
	if (!pzv->Create(this, m_hwnd, WS_CHILD, 0))
		return false;

	// Move the new view to the same position as the current view.
	RECT rc;
	::GetWindowRect(m_pzvCurrent->GetHwnd(), &rc);
	::MapWindowPoints(NULL, m_hwnd, (POINT *)&rc, 2);
	::MoveWindow(pzv->GetHwnd(), rc.left, rc.top, rc.right - rc.left,
		rc.bottom - rc.top, true);
	::ShowWindow(pzv->GetHwnd(), SW_SHOW);

	// Set the selection to the selection of the old view.
	UINT ichSelStart, ichSelStop;
	m_pzvCurrent->GetSelection(&ichSelStart, &ichSelStop);
	pzv->SetSelection(ichSelStart, ichSelStop, true, false);

	// Delete the old view.
	delete m_pzvCurrent;
	m_pzvCurrent = pzv;
	m_pzd->SetCurrentView(pzv);
	::SetFocus(pzv->GetHwnd());
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZFrame::SplitWindow(bool fHorizontal, bool fKeepTopLeft)
{
	m_fIsSplit = !m_fIsSplit;

	if (m_fIsSplit)
	{
		m_fHorizontalSplit = fHorizontal;

		if (m_rghwndVScroll[1] == NULL)
		{
			m_rghwndVScroll[1] = ::CreateWindow("DRZ_Scrollbar", "", WS_CHILD |
				WS_VSCROLL, 0, 0, 0, 0, m_hwnd, NULL, g_fg.m_hinst, 0);
			::SetWindowLong(m_rghwndVScroll[1], GWL_USERDATA, (long)this);
			if (g_fg.m_fFlatScrollbars)
				InitializeFlatSB(m_rghwndVScroll[1]);
		}
		if (m_rghwndHScroll[1] == NULL)
		{
			m_rghwndHScroll[1] = ::CreateWindow("DRZ_Scrollbar", "", WS_CHILD |
				WS_HSCROLL, 0, 0, 0, 0, m_hwnd, NULL, g_fg.m_hinst, 0);
			::SetWindowLong(m_rghwndHScroll[1], GWL_USERDATA, (long)this);
			if (g_fg.m_fFlatScrollbars)
				InitializeFlatSB(m_rghwndHScroll[1]);
		}

		// Create the new view.
		CZView *& pzv = m_rgpzv[1];
		Assert(pzv == NULL);
		if (m_fBinary)
			pzv = new CHexEdit(m_pzef);
		else
			pzv = new CFastEdit(m_pzef);
		if (!pzv->Create(this, m_hwnd, WS_CHILD, 0))
			return false;

		SCROLLINFO si = { sizeof(si), SIF_ALL };
		GetScrollInfo(m_rgpzv[0], SB_VERT, &si);
		SetScrollInfo(m_rgpzv[1], SB_VERT, &si);
		::ShowWindow(m_rghwndVScroll[1], SW_SHOW);
		OnVScroll(SB_THUMBTRACK, si.nPos, m_rghwndVScroll[1]);

		GetScrollInfo(m_rgpzv[0], SB_HORZ, &si);
		SetScrollInfo(m_rgpzv[1], SB_HORZ, &si);
		::ShowWindow(m_rghwndHScroll[1], SW_SHOW);
		OnHScroll(SB_THUMBTRACK, si.nPos, m_rghwndHScroll[1]);

		::ShowWindow(pzv->GetHwnd(), SW_SHOW);
	}
	else
	{
		Assert(m_rgpzv[0] != NULL && m_rgpzv[1] != NULL);
		int izv = !fKeepTopLeft;
		delete m_rgpzv[1 - izv];

		m_pzvCurrent = m_rgpzv[izv];
		m_pzd->SetCurrentView(m_pzvCurrent);

		m_rgpzv[0] = m_pzvCurrent;
		m_rgpzv[1] = NULL;

		if (izv == 1)
		{
			// Swap the scrollbar handles.
			HWND hwndT = m_rghwndVScroll[0];
			m_rghwndVScroll[0] = m_rghwndVScroll[1];
			m_rghwndVScroll[1] = hwndT;
			hwndT = m_rghwndHScroll[0];
			m_rghwndHScroll[0] = m_rghwndHScroll[1];
			m_rghwndHScroll[1] = hwndT;
		}
		::ShowWindow(m_rghwndVScroll[1], SW_HIDE);
		::ShowWindow(m_rghwndHScroll[1], SW_HIDE);
	}

	RECT rc;
	::GetClientRect(m_hwnd, &rc);
	OnSize(SIZE_RESTORED, rc.right - rc.left, rc.bottom - rc.top);

	return m_fIsSplit;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZFrame::NotifyDelete(UINT ichMin, UINT ichStop, bool fRedraw)
{
	AssertPtr(m_rgpzv[0]);
	m_rgpzv[0]->NotifyDelete(ichMin, ichStop, fRedraw);
	if (m_fIsSplit)
	{
		AssertPtr(m_rgpzv[1]);
		m_rgpzv[1]->NotifyDelete(ichMin, ichStop, fRedraw);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZFrame::NotifyInsert(UINT ichMin, UINT cch)
{
	AssertPtr(m_rgpzv[0]);
	m_rgpzv[0]->NotifyInsert(ichMin, cch);
	if (m_fIsSplit)
	{
		AssertPtr(m_rgpzv[1]);
		m_rgpzv[1]->NotifyInsert(ichMin, cch);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CZFrame::SetScrollInfo(CZView * pzv, int nBar, SCROLLINFO * psi)
{
	AssertPtr(pzv);
	AssertPtr(psi);
	Assert(nBar == SB_VERT || nBar == SB_HORZ);
	Assert(m_rgpzv[0] == pzv || m_rgpzv[1] == pzv);

	SCROLLINFO si = *psi;

	int izv = m_rgpzv[1] == pzv;
	RECT & rc = m_rgrcClient[izv];
	HWND hwndScroll = (nBar == SB_VERT) ? m_rghwndVScroll[izv] : m_rghwndHScroll[izv];
	if (nBar == SB_VERT)
		si.fMask |= SIF_DISABLENOSCROLL;

	int nRet;
	if (g_fg.m_fFlatScrollbars)
		nRet = FlatSB_SetScrollInfo(hwndScroll, nBar, &si, true);
	else
		nRet = ::SetScrollInfo(hwndScroll, nBar, &si, true);

	if (nBar == SB_HORZ)
	{
		// Hide the horizontal scrollbar if we can.
		RECT rc;
		::GetClientRect(m_hwnd, &rc);
		OnSize(SIZE_RESTORED, rc.right - rc.left, rc.bottom - rc.top);
	}
	return nRet;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CZFrame::GetScrollInfo(CZView * pzv, int nBar, SCROLLINFO * psi)
{
	AssertPtr(pzv);
	AssertPtr(psi);
	Assert(nBar == SB_VERT || nBar == SB_HORZ);
	Assert(m_rgpzv[0] == pzv || m_rgpzv[1] == pzv);

	int izv = m_rgpzv[1] == pzv;
	HWND hwndScroll = NULL;
	if (nBar == SB_VERT)
		hwndScroll = m_rghwndVScroll[izv];
	else
		hwndScroll = m_rghwndHScroll[izv];

	if (g_fg.m_fFlatScrollbars)
		return FlatSB_GetScrollInfo(hwndScroll, nBar, psi);
	return ::GetScrollInfo(hwndScroll, nBar, psi);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZFrame::OnColumnMarkerDrag()
{
	AssertPtr(m_rgpzv[0]);
	m_rgpzv[0]->OnColumnMarkerDrag();
	if (m_fIsSplit)
	{
		AssertPtr(m_rgpzv[1]);
		m_rgpzv[1]->OnColumnMarkerDrag();
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZFrame::OnFontChanged()
{
	AssertPtr(m_rgpzv[0]);
	m_rgpzv[0]->OnFontChanged();
	if (m_fIsSplit)
	{
		AssertPtr(m_rgpzv[1]);
		m_rgpzv[1]->OnFontChanged();
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZFrame::OnSize(UINT nType, int cx, int cy)
{
	RECT & rc0 = m_rgrcClient[0];
	RECT & rc1 = m_rgrcClient[1];

	int dxpScroll = ::GetSystemMetrics(SM_CXVSCROLL);
	int dypScroll = ::GetSystemMetrics(SM_CYHSCROLL);

	AssertPtr(m_rgpzv[0]);
	if (m_fIsSplit)
	{
		AssertPtr(m_rgpzv[1]);
		// This gets called when we're first creating the second view.
		// Since it gets resized correctly later, return without doing
		// anything now.
		if (!::IsWindowVisible(m_rgpzv[1]->GetHwnd()))
			return 0;

		if (m_fHorizontalSplit)
		{
			int dypTop = (cy - kdzpSeparator) * m_nTopHalfPercent / 10000;
			::SetRect(&rc0, 0, 0, cx, dypTop);
			::SetRect(&rc1, 0, dypTop + kdzpSeparator, cx, cy);
		}
		else
		{
			int dxpLeft = (cx - kdzpSeparator) * m_nTopHalfPercent / 10000;
			::SetRect(&rc0, 0, 0, dxpLeft, cy);
			::SetRect(&rc1, dxpLeft + kdzpSeparator, 0, cx, cy);
		}
	}
	else
	{
		::SetRect(&rc0, 0, 0, cx, cy);
		::SetRect(&rc1, 0, 0, 0, 0);
	}

	RECT rcBottom;
	::GetClientRect(m_rghwndHScroll[0], &rcBottom);
	if (rcBottom.bottom - rcBottom.top > 1)
	{
		::ShowWindow(m_rghwndHScroll[0], SW_HIDE);
		rc0.bottom += dypScroll;
	}
	else
	{
		::ShowWindow(m_rghwndHScroll[0], SW_SHOW);
	}
	if (m_fIsSplit)
	{
		::GetClientRect(m_rghwndHScroll[1], &rcBottom);
		if (rcBottom.bottom - rcBottom.top > 1)
		{
			::ShowWindow(m_rghwndHScroll[1], SW_HIDE);
			rc1.bottom += dypScroll;
		}
		else
		{
			::ShowWindow(m_rghwndHScroll[1], SW_SHOW);
		}
	}

	// If we're not split, move the top right scrollbar down a little to make room
	// for the splitter bar at the top right.
	int dypOffset = 0;
	if (!m_fIsSplit)
		dypOffset = kdypSplitter;

	::MoveWindow(m_rghwndVScroll[0], rc0.right - dxpScroll - 1, rc0.top + dypOffset,
		dxpScroll + 1, rc0.bottom - rc0.top - dypScroll - dypOffset, TRUE);
	::MoveWindow(m_rghwndHScroll[0], rc0.left, rc0.bottom - dypScroll - 1,
		rc0.right - rc0.left - dxpScroll, dypScroll + 1, TRUE);
	::MoveWindow(m_rgpzv[0]->GetHwnd(), rc0.left, rc0.top,
		rc0.right - rc0.left - dxpScroll, rc0.bottom - rc0.top - dypScroll, TRUE);
	if (m_fIsSplit)
	{
		::MoveWindow(m_rghwndVScroll[1], rc1.right - dxpScroll - 1, rc1.top,
			dxpScroll + 1, rc1.bottom - rc1.top - dypScroll, TRUE);
		::MoveWindow(m_rghwndHScroll[1], rc1.left, rc1.bottom - dypScroll - 1,
			rc1.right - rc1.left - dxpScroll, dypScroll + 1, TRUE);
		::MoveWindow(m_rgpzv[1]->GetHwnd(), rc1.left, rc1.top,
			rc1.right - rc1.left - dxpScroll, rc1.bottom - rc1.top - dypScroll, TRUE);
	}

	::InvalidateRect(m_hwnd, NULL, false);
	::UpdateWindow(m_hwnd);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZFrame::OnMouseMove(UINT nFlags, POINT pt)
{
	if (m_fHorizontalSplit)
		::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
	else
		::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));

	if (m_fResizing)
	{
		RECT rc;
		::GetClientRect(m_hwnd, &rc);
		int nTopHalfPercent;
		if (m_fHorizontalSplit)
			nTopHalfPercent = pt.y * 10000 / (rc.bottom - rc.top);
		else
			nTopHalfPercent = pt.x * 10000 / (rc.right - rc.left);
		if (nTopHalfPercent < 100)
			nTopHalfPercent = 0;
		else if (nTopHalfPercent > 9900)
			nTopHalfPercent = 10000;
		m_nTopHalfPercent = nTopHalfPercent;
		OnSize(SIZE_RESTORED, rc.right - rc.left, rc.bottom - rc.top);
	}

	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZFrame::OnLButtonDown(UINT nFlags, POINT pt)
{
	m_fResizing = true;
	::SetCapture(m_hwnd);

	if (!m_fIsSplit)
	{
		// We're clicking on the splitter at the top-right instead of on the bar
		// between the two child windows, so we need to create the second window now.
		m_nTopHalfPercent = 0;
		SplitWindow(true, true);
	}

	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZFrame::OnLButtonUp(UINT nFlags, POINT pt)
{
	m_fResizing = false;
	::ReleaseCapture();

	if (m_nTopHalfPercent < 200 || m_nTopHalfPercent > 9800)
	{
		SplitWindow(false, m_nTopHalfPercent > 9800);
		m_nTopHalfPercent = 5000;
	}

	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZFrame::OnHScroll(UINT nSBCode, UINT nPos, HWND hwndScroll)
{
	Assert(m_rghwndHScroll[0] == hwndScroll || m_rghwndHScroll[1] == hwndScroll);
	int izv = m_rghwndHScroll[1] == hwndScroll;
	return m_rgpzv[izv]->OnHScroll(nSBCode, nPos, hwndScroll);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZFrame::OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScroll)
{
	Assert(m_rghwndVScroll[0] == hwndScroll || m_rghwndVScroll[1] == hwndScroll);
	int izv = m_rghwndVScroll[1] == hwndScroll;
	return m_rgpzv[izv]->OnVScroll(nSBCode, nPos, hwndScroll);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CZFrame::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hwnd, &ps);

	RECT rcClient;
	::GetClientRect(m_hwnd, &rcClient);

	::SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
	::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ps.rcPaint, NULL, 0, NULL);

	RECT rc;
	if (m_fIsSplit)
	{
		if (m_fHorizontalSplit)
		{
			int dypTop = (rcClient.bottom - kdzpSeparator) * m_nTopHalfPercent / 10000;
			::SetRect(&rc, 0, dypTop, rcClient.right, dypTop + kdzpSeparator);
		}
		else
		{
			int dxpLeft = (rcClient.right - kdzpSeparator) * m_nTopHalfPercent / 10000;
			::SetRect(&rc, dxpLeft, 0, dxpLeft + kdzpSeparator, rcClient.bottom);
		}
		::DrawEdge(hdc, &rc, EDGE_BUMP, BF_TOP | BF_BOTTOM | BF_SOFT);
	}
	else
	{
		int dxpScroll = ::GetSystemMetrics(SM_CXVSCROLL);
		::SetRect(&rc, rcClient.right - dxpScroll, 0, rcClient.right, kdypSplitter);
		::DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_MIDDLE);
	}

	::EndPaint(m_hwnd, &ps);

	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK CZFrame::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CZFrame * pzf = (CZFrame *)::GetWindowLong(hwnd, GWL_USERDATA);
	AssertPtrN(pzf);
	if (pzf)
	{
		switch (iMsg)
		{
		case WM_SETFOCUS:
			return (LRESULT)::SetFocus(pzf->GetCurrentView()->GetHwnd());

		case WM_SIZE:
			return pzf->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));

		case WM_LBUTTONDOWN:
			{
				POINT point = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
				return pzf->OnLButtonDown(wParam, point);
			}

		case WM_LBUTTONUP:
			{
				POINT point = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
				return pzf->OnLButtonUp(wParam, point);
			}

		case WM_MOUSEMOVE:
			{
				POINT point = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
				return pzf->OnMouseMove(wParam, point);
			}

		case WM_HSCROLL:
			return pzf->OnHScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

		case WM_VSCROLL:
			return pzf->OnVScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

		case WM_PAINT:
			return pzf->OnPaint();
		}
	}
	return ::DefWindowProc(hwnd, iMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK CZFrame::ScrollWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CZFrame * pzf = (CZFrame *)::GetWindowLong(hwnd, GWL_USERDATA);
	AssertPtrN(pzf);

	if (pzf)
	{
		if (iMsg == WM_VSCROLL)
			return pzf->OnVScroll(LOWORD(wParam), HIWORD(wParam), hwnd);
		if (iMsg == WM_HSCROLL)
			return pzf->OnHScroll(LOWORD(wParam), HIWORD(wParam), hwnd);
	}

	return ::DefWindowProc(hwnd, iMsg, wParam, lParam);
}