#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


// Also defined in HexEdit.cpp.
inline UINT CHexEdit::GetLineCount()
{
	AssertPtr(m_pzd);
	return (m_pzd->GetCharCount() / m_cchPerLine) + 1;
}

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	AssertPtr(m_pzef);

	m_pzd->OnChar(m_ichSelMin, m_ichSelLim, nChar, m_fOvertype, m_fOnlyEditing);

	if (nChar == VK_ESCAPE || nChar == VK_BACK)
		return 0;
	if (nChar == 10 || nChar == 13)
	{
		m_fOnlyEditing = false;
		m_ichDragStart++;
	}
	m_ichSelMin = m_ichSelLim = ++m_ichDragStart;
	SetSelection(m_ichSelMin, m_ichSelLim, true, true);
	m_pzef->UpdateTextPos();
	m_pzef->m_pfd->ResetFind();
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnContextMenu(HWND hwnd, POINT pt)
{
	AssertPtr(m_pzd);

	m_fOnlyEditing = false;
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	POINT ptClient = pt;
	MapWindowPoints(NULL, m_hwnd, &ptClient, 1);
	if (ptClient.y <= knWedgeHeight || ptClient.y >= rc.bottom - knWedgeHeight)
	{
		int iColumn = g_fg.GetColumnMarker(ptClient);
		if (iColumn != -1)
		{
			Assert((UINT)iColumn < (UINT)g_fg.m_vpcm[iColumn]);
			HMENU hMenu = m_pzef->GetContextMenu()->GetSubMenu(kcmColumn);
			int nID = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON |
				TPM_RETURNCMD, pt.x, pt.y, 0, m_pzef->GetHwnd(), NULL);
			if (nID == ID_COLUMNS_DELETE)
			{
				delete g_fg.m_vpcm[iColumn];
				g_fg.m_vpcm.erase(g_fg.m_vpcm.begin() + iColumn);
			}
			else if (nID == ID_COLUMNS_COLOR)
			{
				COLORREF rgcr[16];
				memset(rgcr, 0xFF, sizeof(rgcr));
				CHOOSECOLOR cc = {sizeof(cc), m_hwnd};
				cc.Flags = CC_RGBINIT | CC_ANYCOLOR;
				cc.rgbResult = g_fg.m_vpcm[iColumn]->m_cr;
				cc.lpCustColors = rgcr;
				if (ChooseColor(&cc))
					g_fg.m_vpcm[iColumn]->m_cr = cc.rgbResult;
			}
			else if (nID == ID_COLUMNS_EDIT)
			{
				DialogBox(g_fg.m_hinst, MAKEINTRESOURCE(IDD_COLUMNMARKER), m_hwnd, &CZEditFrame::ColumnProc);
			}
			if (nID != 0)
			{
				// TODO: Invalidate all windows.
				HDC hdc = GetDC(m_hwnd);
				DrawSelection(hdc);
				ReleaseDC(m_hwnd, hdc);
			}
			return 0;
		}
	}
	HMENU hMenu = m_pzef->GetContextMenu()->GetSubMenu(kcmEdit);
	SendMessage(m_pzef->GetHwnd(), WM_INITMENUPOPUP, (WPARAM)hMenu, 0);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0,
		m_pzef->GetHwnd(), NULL);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CHexEdit::OnFontChanged()
{
	// Remove the current caret
	::HideCaret(m_hwnd);

	// Redraw the screen with the new font
	RECT rc;
	::GetClientRect(m_hwnd, &rc);
	RecalculateLines(rc.bottom - rc.top);
	::InvalidateRect(m_hwnd, NULL, false);
	::UpdateWindow(m_hwnd);

	// Recreate the new caret with the proper size and position
	m_ptCaretPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	::CreateCaret(m_hwnd, NULL, 2, g_fg.m_tmFixed.tmHeight);
	::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	::ShowCaret(m_hwnd);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnHScroll(UINT nSBCode, UINT nPos, HWND hwndScroll)
{
	if (!m_hwnd)
		return 1;

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS | SIF_TRACKPOS;
	m_pzf->GetScrollInfo(this, SB_HORZ, &si);
	RECT rc;
	::GetClientRect(m_hwnd, &rc);

	switch (nSBCode)
	{
		case SB_LINELEFT:
			OffsetRect(&m_rcScreen, g_fg.m_tmFixed.tmMaxCharWidth, 0);
			if (m_rcScreen.left > g_fg.m_rcMargin.left)
				m_rcScreen = g_fg.m_rcMargin;
			break;

		case SB_LINERIGHT:
			OffsetRect(&m_rcScreen, -g_fg.m_tmFixed.tmMaxCharWidth, 0);
			if ((UINT)g_fg.m_rcMargin.left - m_rcScreen.left > m_nTotalWidth)
				OffsetRect(&m_rcScreen, g_fg.m_rcMargin.left - m_rcScreen.left - m_nTotalWidth, 0);
			break;

		case SB_PAGELEFT:
			OffsetRect(&m_rcScreen, rc.right, 0);
			if (m_rcScreen.left > g_fg.m_rcMargin.left)
				m_rcScreen = g_fg.m_rcMargin;
			break;

		case SB_PAGERIGHT:
			OffsetRect(&m_rcScreen, -rc.right, 0);
			if ((UINT)g_fg.m_rcMargin.left - m_rcScreen.left > m_nTotalWidth)
				OffsetRect(&m_rcScreen, g_fg.m_rcMargin.left - m_rcScreen.left - m_nTotalWidth, 0);
			break;

		case SB_THUMBTRACK:
			m_rcScreen = g_fg.m_rcMargin;
			OffsetRect(&m_rcScreen, -si.nTrackPos, 0);
			break;

		default:
			return 0;
	}

	si.nPos = g_fg.m_rcMargin.left - m_rcScreen.left;
	m_pzf->SetScrollInfo(this, SB_HORZ, &si);
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
	m_ptCaretPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	AssertPtr(m_pzd);
	if (!m_hwnd)
		return 1;
	bool fShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	bool fControl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	bool fRedraw = false;
	bool fSelection = m_ichSelMin != m_ichSelLim;
	bool fMoveDown = true;
	UINT cchTotal = m_pzd->GetCharCount();

	switch (nChar)
	{
	case VK_BACK:
		{
			if (m_ichSelMin == 0)
				return 1;
			UINT iStart = m_ichSelMin;
			if (m_ichSelMin == m_ichSelLim)
				iStart--;
			UINT cch = m_ichSelLim - iStart;
			CUndoRedo * pur = new CUndoRedo(kurtDelete, iStart, cch, 0);
			if (pur)
			{
				m_pzd->GetText(iStart, cch, &pur->m_pzdo);
				m_pzd->PushUndoEntry(pur);
			}
			m_pzd->DeleteText(iStart, m_ichSelLim, false);
			SetSelection(m_ichSelMin, m_ichSelMin, true, false);
			if (fSelection)
				ShowCaret(m_hwnd);
			return 0;
		}

	case VK_LEFT:
		fMoveDown = false;
		if (m_ichDragStart == m_ichSelLim)
		{
			if (m_ichSelMin > 0 && (fShift || !fSelection))
				m_ichSelMin--;
		}
		else if (fShift)
		{
			fMoveDown = true;
			m_ichSelLim--;
		}
		break;

	case VK_RIGHT:
		if (m_ichDragStart == m_ichSelMin)
		{
			if (m_ichSelLim < cchTotal && (fShift || !fSelection))
				m_ichSelLim++;
		}
		else if (fShift)
		{
			m_ichSelMin++;
			fMoveDown = false;
		}
		break;

	case VK_UP:
		{
			if (fControl)
			{
				OnVScroll(SB_LINEUP, 0, 0);
				return 1;
			}
			UINT * piChange = &m_ichSelMin;
			fMoveDown = false;
			if (fShift)
			{
				piChange = m_ichSelLim == m_ichDragStart ? &m_ichSelMin : &m_ichSelLim;
				if (piChange == &m_ichSelLim)
					fMoveDown = true;
			}
			if (*piChange < m_cchPerLine)
				*piChange = 0;
			else
				*piChange -= m_cchPerLine;
			break;
		}

	case VK_DOWN:
		{
			RECT rc;
			GetClientRect(m_hwnd, &rc);
			if (fControl)
			{
				OnVScroll(SB_LINEDOWN, 0, 0);
				return 1;
			}
			UINT * piChange = &m_ichSelLim;
			if (fShift)
			{
				piChange = m_ichSelMin == m_ichDragStart ? &m_ichSelLim : &m_ichSelMin;
				if (piChange == &m_ichSelMin)
					fMoveDown = false;
			}
			if (*piChange < cchTotal - m_cchPerLine)
				*piChange += m_cchPerLine;
			else
				*piChange = cchTotal;
			break;
		}

	// TODO: Make sure this works properly.
	case VK_HOME:
		if (fControl)
		{
			m_ichTopLine = 0;
			m_ichSelMin = 0;
			fRedraw = true;
			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS;
			si.nPos = 0;
			m_pzf->SetScrollInfo(this, SB_VERT, &si);
			::InvalidateRect(m_hwnd, NULL, FALSE);
		}
		else
		{
			UINT ich;
			if (fShift)
				ich = m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin;
			else
				ich = m_ichSelMin;
			m_ichSelMin = (ich / m_cchPerLine) * m_cchPerLine;
			/*if (m_fEndOfLine && m_ichSelMin >= 16)
				m_ichSelMin -= 16;
			m_fEndOfLine = false;*/
			// TODO: Scroll the screen if the cursor is not visible
		}
		fMoveDown = false;
		break;

	// TODO: Make sure this works properly.
	case VK_END:
		if (fControl)
		{
			m_ichTopLine = CharFromLine(GetLineCount() - m_cli - 2);
			m_ichSelLim = cchTotal;
			fRedraw = true;
			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS;
			si.nPos = LineFromChar(m_ichTopLine);
			m_pzf->SetScrollInfo(this, SB_VERT, &si);
		}
		else
		{
			UINT ich;
			if (fShift)
				ich = m_ichDragStart == m_ichSelLim ? m_ichSelMin : m_ichSelLim;
			else
				ich = m_ichSelLim;
			m_ichSelLim = ((ich / m_cchPerLine) * m_cchPerLine) + m_cchPerLine;
			/*if (fShift)
				m_ichSelLim++;*/
			if (m_ichSelLim > cchTotal)
				m_ichSelLim = cchTotal; // shouldn't ever happen
			// TODO: Scroll the screen if the cursor is not visible
		}
		break;

	case VK_PRIOR: // page up
		{
			OnVScroll(SB_PAGEUP, 0, 0);
			if (LineFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin) < m_cli)
				m_ichSelMin = 0;
			else
				m_ichSelMin = CharFromPoint(m_ptCaretPos, true);
			fMoveDown = false;
			break;
		}

	case VK_NEXT: // page down
		{
			OnVScroll(SB_PAGEDOWN, 0, 0);
			m_ichSelLim = CharFromPoint(m_ptCaretPos, true);
			break;
		}

	case VK_INSERT:
		m_fOvertype = m_fOvertype ? false : true;
		m_fOnlyEditing = false;
		return 0;

	case VK_DELETE:
		{
			if (m_ichSelMin >= cchTotal)
				return 1;
			UINT cch = max(1, m_ichSelLim - m_ichSelMin);
			CUndoRedo * pur = new CUndoRedo(kurtDelete, m_ichSelMin, cch, 0);
			if (pur)
			{
				m_pzd->GetText(m_ichSelMin, cch, &pur->m_pzdo);
				m_pzd->PushUndoEntry(pur);
			}
			m_pzd->DeleteText(m_ichSelMin, m_ichSelMin + cch, false);
			SetSelection(m_ichSelMin, m_ichSelMin, true, false);
			if (fSelection)
				ShowCaret(m_hwnd);
			return 0;
		}

	default:
		return 0;
	}

	if (fMoveDown)
	{
		if (fShift)
			m_ichSelMin = m_ichDragStart;
		else
			m_ichDragStart = m_ichSelMin = m_ichSelLim;
	}
	else
	{
		if (fShift)
			m_ichSelLim = m_ichDragStart;
		else
			m_ichDragStart = m_ichSelLim = m_ichSelMin;
	}
	if (m_ichSelMin > m_ichDragStart)
	{
		m_ichSelLim = m_ichSelMin;
		m_ichSelMin = m_ichDragStart;
	}
	if (m_ichSelLim < m_ichDragStart)
	{
		m_ichSelMin = m_ichSelLim;
		m_ichSelLim = m_ichDragStart;
	}

	if (nChar == VK_END)
		HideCaret(m_hwnd);
	if (m_ichSelMin == m_ichDragStart)
		SetSelection(m_ichSelMin, m_ichSelLim, true, false);
	else
		SetSelection(m_ichSelLim, m_ichSelMin, true, false);
	if (nChar == VK_END)
	{
		int nCharWidth = g_fg.m_tmFixed.tmAveCharWidth;
		// TODO: This caused problems when going to the very end of a file (Ctrl + End).
		// It also caused problems when the cursor was in the text part, not the binary part.
		//Assert(m_ptCaretPos.x == m_rcScreen.left + (m_nByteOffset * nCharWidth));
		m_ptCaretPos.y -= g_fg.m_tmFixed.tmHeight;
		m_ptCaretPos.x += m_cchPerLine * nCharWidth * 3 - nCharWidth;
		SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
		ShowCaret(m_hwnd);
	}
	if (fSelection && m_ichSelMin == m_ichSelLim)
		ShowCaret(m_hwnd);
	else if (!fSelection && m_ichSelMin != m_ichSelLim)
		HideCaret(m_hwnd);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnKillFocus(HWND hwndGetFocus)
{
	if (!m_hwnd)
		return 1;

	m_fOnlyEditing = false;
	HideCaret(m_hwnd);
	DestroyCaret();
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnLButtonDblClk(UINT nFlags, POINT point)
{
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnLButtonDown(UINT nFlags, POINT point)
{
	if (!m_hwnd)
		return 1;

	m_pzf->SetCurrentView(this);
	m_pzef->UpdateTextPos();
	::SetFocus(m_hwnd);
	m_fOnlyEditing = false;

	if (m_pzd->GetCharCount() == 0)
		return 0;

	// Start selecting text
	::SetCapture(m_hwnd);
	UINT iOldSelStart = m_ichSelMin;
	UINT iOldSelStop = m_ichSelLim;

	if (nFlags & MK_SHIFT)
	{
		UINT ich = CharFromPoint(point, true);
		if (ich < m_ichDragStart)
		{
			m_ichSelMin = ich;
			m_ichSelLim = m_ichDragStart;
		}
		else
		{
			m_ichSelMin = m_ichDragStart;
			m_ichSelLim = ich;
		}
	}
	else
	{
		m_ichDragStart = CharFromPoint(point, true);
		m_ichSelMin = m_ichSelLim = m_ichDragStart;
	}

	// TODO: Does this work when er are horizontally scrolled to the right?
	if ((UINT)point.x - m_rcScreen.left + g_fg.m_rcMargin.left >= m_nTextOffset * g_fg.m_tmFixed.tmAveCharWidth)
		m_fCaretInText = true;
	else
		m_fCaretInText = false;

	m_fSelecting = true;
	if (iOldSelStart != iOldSelStop || m_ichSelMin != m_ichSelLim)
	{
		::InvalidateRect(m_hwnd, NULL, FALSE);
		::UpdateWindow(m_hwnd);
	}
	::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);

	if (iOldSelStart != iOldSelStop && m_ichSelMin == m_ichSelLim)
		::ShowCaret(m_hwnd);
	else if (iOldSelStart == iOldSelStop && m_ichSelMin != m_ichSelLim)
		::HideCaret(m_hwnd);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnLButtonUp(UINT nFlags, POINT point)
{
	if (!m_hwnd)
		return 1;

	m_pzef->UpdateTextPos();
	m_pzef->m_pfd->ResetFind();

	if (m_fSelecting)
	{
		UINT iOldSelStart = m_ichSelMin;
		UINT iOldSelStop = m_ichSelLim;
		if (m_mt != kmtNone)
		{
			::KillTimer(m_hwnd, m_mt);
			m_mt = kmtNone;
		}
		m_fSelecting = false;
		::ReleaseCapture();
		m_ichSelLim = CharFromPoint(point, true);
		if (m_ichSelLim > m_ichDragStart)
		{
			m_ichSelMin = m_ichDragStart;
		}
		else if (m_ichSelLim < m_ichDragStart)
		{
			m_ichSelMin = m_ichSelLim;
			m_ichSelLim = m_ichDragStart;
		}
		if (iOldSelStart != iOldSelStop || m_ichSelMin != m_ichSelLim)
			::InvalidateRect(m_hwnd, NULL, FALSE);
		::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
		if (iOldSelStart != iOldSelStop && m_ichSelMin == m_ichSelLim)
			::ShowCaret(m_hwnd);
		else if (iOldSelStart == iOldSelStop && m_ichSelMin != m_ichSelLim)
			::HideCaret(m_hwnd);
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnMButtonDown(UINT nFlags, POINT point)
{
	return 1;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnMouseMove(UINT nFlags, POINT point)
{
	if (!m_hwnd)
		return 1;

	RECT rc;
	::GetClientRect(m_hwnd, &rc);

	if (!m_fSelecting)
	{
		UINT x = point.x - m_rcScreen.left + g_fg.m_rcMargin.left;
		if (x < m_nByteOffset * g_fg.m_tmFixed.tmAveCharWidth ||
			x > (m_nTextOffset + m_cchPerLine) * g_fg.m_tmFixed.tmAveCharWidth)
		{
			::SetCursor(::LoadCursor(NULL, IDC_ARROW));
		}
		else
		{
			::SetCursor(g_fg.m_hCursor[kcsrIBeam]);
		}
	}
	else
	{
		::SetCursor(g_fg.m_hCursor[kcsrIBeam]);
		UINT iOldSelStart = m_ichSelMin;
		UINT iOldSelStop = m_ichSelLim;

		RECT rcScroll;
		int dypOffset = g_fg.m_tmFixed.tmHeight;
		::SetRect(&rcScroll, rc.left, rc.top + dypOffset + m_rcScreen.top, 
			rc.right, rc.bottom - dypOffset - m_rcScreen.bottom);

		if (!PtInRect(&rcScroll, point))
		{
			int nDifference = 0;
			if (point.x < 0)
			{
				OnHScroll(SB_LINELEFT, 0, NULL);
				point.x = 0;
			}
			else if (point.x > rcScroll.right)
			{
				OnHScroll(SB_LINERIGHT, 0, NULL);
				point.x = rcScroll.right;
			}
			if (point.y < 0)
			{
				nDifference = -point.y;
				OnVScroll(SB_LINEUP, 0, NULL);
				point.y = 0;
			}
			if (point.y > rcScroll.bottom)
			{
				nDifference = point.y - rcScroll.bottom;
				OnVScroll(SB_LINEDOWN, 0, NULL);
				point.y = rcScroll.bottom;
			}
			if (nDifference < 20)
			{
				if (m_mt != kmtSlow)
					KillTimer(m_hwnd, m_mt);
				m_mt = (MouseTimer)SetTimer(m_hwnd, kmtSlow, 100, NULL);
			}
			else if (nDifference < 80)
			{
				if (m_mt != kmtMedium)
					KillTimer(m_hwnd, m_mt);
				m_mt = (MouseTimer)SetTimer(m_hwnd, kmtMedium, 50, NULL);
			}
			else
			{
				if (m_mt != kmtFast)
					KillTimer(m_hwnd, m_mt);
				m_mt = (MouseTimer)SetTimer(m_hwnd, kmtFast, 1, NULL);
			}
		}
		else if (m_mt != kmtNone)
		{
			KillTimer(m_hwnd, m_mt);
			m_mt = kmtNone;
		}
		
		m_ichSelLim = CharFromPoint(point, true);
		if (m_ichSelLim >= m_ichDragStart)
		{
			m_ichSelMin = m_ichDragStart;
		}
		else
		{
			m_ichSelMin = m_ichSelLim;
			m_ichSelLim = m_ichDragStart;
		}
		HDC hdc = GetDC(m_hwnd);
		/*::SetTextColor(hdc, g_fg.m_cr[kclrText]);
		SetBkColor(hdc, g_fg.m_cr[kclrWindow]);*/
		DrawSelection(hdc);
		ReleaseDC(m_hwnd, hdc);

		if (iOldSelStart == iOldSelStop && m_ichSelMin != m_ichSelLim)
			HideCaret(m_hwnd);
		else if (iOldSelStart != iOldSelStop && m_ichSelMin == m_ichSelLim)
			ShowCaret(m_hwnd);
		return 0;
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnMouseWheel(UINT nFlags, short zDelta, POINT point)
{
	// TODO: Handle the CTRL key.
	RECT rc;
	GetWindowRect(m_hwnd, &rc);
	if (PtInRect(&rc, point))
	{
		for (int i = 0; i < 3; i++)
		{
			if (zDelta > 0)
				OnVScroll(SB_LINEUP, 0, NULL);
			else
				OnVScroll(SB_LINEDOWN, 0, NULL);
		}
	}
	else
	{
		SendMessage(WindowFromPoint(point), WM_MOUSEWHEEL, MAKEWPARAM(nFlags, zDelta),
			MAKELPARAM(point.x, point.y));
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnPaint()
{
	if (!m_hwnd)
		return 1;

	PAINTSTRUCT ps;
	::BeginPaint(m_hwnd, &ps);
	HDC hdc = ps.hdc;
	InitializeDC(hdc);

	RECT rcClient, rc;
	::GetClientRect(m_hwnd, &rcClient);

	if (m_pzd && !m_pzd->GetAccessError() && m_pzd->GetCharCount())
	{
		// Blank out the left margin.
		::SetRect(&rc, 0, 0, m_rcScreen.left, rcClient.bottom);
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

		// Blank out the right margin.
		::SetRect(&rc, rcClient.right - m_rcScreen.right, 0, rcClient.right, rcClient.bottom);
		int dxp = m_rcScreen.left + (m_nTotalWidth * g_fg.m_tmFixed.tmAveCharWidth);
		if (rc.left > dxp)
			rc.left = dxp;
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

		// Blank out the top margin.
		::SetRect(&rc, 0, 0, rcClient.right, m_rcScreen.top);
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

		// Blank out the bottom margin.
		::SetRect(&rc, 0, rcClient.bottom - m_rcScreen.bottom, rcClient.right, rcClient.bottom);
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

		// Calculate the valid rectangle to draw in.
		::SetRect(&rc, m_rcScreen.left, m_rcScreen.top,
			rcClient.right - m_rcScreen.right, rcClient.bottom - m_rcScreen.bottom);
		::SelectClipRgn(hdc, NULL);
		::IntersectClipRect(hdc, g_fg.m_rcMargin.left, g_fg.m_rcMargin.top,
			rcClient.right - g_fg.m_rcMargin.right, rcClient.bottom - g_fg.m_rcMargin.bottom);
	}

	::SetTextColor(hdc, g_fg.m_cr[kclrText]);
	::SetBkColor(hdc, g_fg.m_cr[kclrWindow]);
	ShowText(hdc, rc);
	::EndPaint(m_hwnd, &ps);

	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnSetFocus(HWND hwndLoseFocus)
{
	if (!m_hwnd || !m_pzd)
		return 1;

	UpdateWindow(m_hwnd);
	m_ptCaretPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	CreateCaret(m_hwnd, NULL, 2, g_fg.m_tmFixed.tmHeight);
	SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	if (m_ichSelMin == m_ichSelLim)
		ShowCaret(m_hwnd);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnSize(UINT nType, int cx, int cy)
{
	AssertPtrN(m_pzd);
	if (!m_pzd || !m_hwnd || cx == 0 || cy == 0)
		return 1;

	RecalculateLines(cy);

	// Update scrollbars.
	SCROLLINFO si = { sizeof(si), SIF_PAGE | SIF_RANGE };
	m_pzf->GetScrollInfo(this, SB_VERT, &si);
	si.nPage = m_cli;
	si.nMax = GetLineCount();
	m_pzf->SetScrollInfo(this, SB_VERT, &si);
	si.nPage = cx - g_fg.m_rcMargin.left - g_fg.m_rcMargin.right;
	si.nMax = m_nTotalWidth * g_fg.m_tmFixed.tmAveCharWidth;
	m_pzf->SetScrollInfo(this, SB_HORZ, &si);

	::InvalidateRect(m_hwnd, NULL, FALSE);
	::UpdateWindow(m_hwnd);
	m_ptCaretPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	if (::GetFocus() == m_hwnd)
		::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnTimer(UINT nIDEvent)
{
	POINT point;
	GetCursorPos(&point);
	RECT rc;
	GetWindowRect(m_hwnd, &rc);
	point.x -= rc.left;
	point.y -= rc.top;
	return OnMouseMove(0, point);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CHexEdit::OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScroll)
{
	AssertPtr(m_pzd);
	if (!m_hwnd)
		return 1;

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS | SIF_TRACKPOS;
	m_pzf->GetScrollInfo(this, SB_VERT, &si);
	int nLineOffset = 0;

	switch (nSBCode)
	{
		case SB_LINEUP:
			if (m_ichTopLine == 0)
				return 0;
			nLineOffset = -1;
			break;

		case SB_LINEDOWN:
			if (m_ichTopLine + (m_cchPerLine * (m_cli - 1)) > m_pzd->GetCharCount())
				return 0;
			nLineOffset = 1;
			break;

		case SB_PAGEUP:
			if (LineFromChar(m_ichTopLine) >= m_cli)
				nLineOffset = -(int)m_cli;
			else
				m_ichTopLine = 0;
			break;

		case SB_PAGEDOWN:
			// TODO: Catch paging below the end of file
			nLineOffset = m_cli;
			break;

		case SB_THUMBTRACK:
			m_ichTopLine = CharFromLine(si.nTrackPos);
			break;

		default:
			return 0;
	}

	m_ichTopLine += nLineOffset * m_cchPerLine;
	si.nPos = LineFromChar(m_ichTopLine);

	HideCaret(m_hwnd);
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
	m_ptCaretPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	if (m_ichSelMin == m_ichSelLim)
		ShowCaret(m_hwnd);
	m_pzf->SetScrollInfo(this, SB_VERT, &si);
	return 0;
}