#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


/***********************************************************************************************
	CHexEdit methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CHexEdit::CHexEdit(CZEditFrame * pzef) : CZView(pzef)
{
	m_cli = 0;
	Initialize();
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CHexEdit::~CHexEdit()
{
	if (m_hwnd)
		::DestroyWindow(m_hwnd);
	if (m_pds)
		m_pds->Release();
}


/*----------------------------------------------------------------------------------------------
	HexEdit window procedure.
----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK CHexEdit::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CHexEdit * phe = (CHexEdit *)::GetWindowLong(hwnd, GWL_USERDATA);
	AssertPtrN(phe);

#ifndef _DEBUG
	__try
#endif
	{
		if (phe)
		{
			switch (iMsg)
			{
				case WM_CHAR:
					phe->OnChar(wParam, LOWORD(lParam), HIWORD(lParam));
					return 0;

				case WM_CONTEXTMENU:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					phe->OnContextMenu((HWND)wParam, pt);
					break;
				}

				case WM_KEYDOWN:
					return phe->OnKeyDown(wParam, LOWORD(lParam), HIWORD(lParam));

				case WM_KILLFOCUS:
					return phe->OnKillFocus((HWND)wParam);

				case WM_LBUTTONDOWN:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					phe->OnLButtonDown(wParam, pt);
					break;
				}

				case WM_LBUTTONUP:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					phe->OnLButtonUp(wParam, pt);
					break;
				}

				case WM_MOUSEMOVE:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					return phe->OnMouseMove(wParam, pt);
				}

				case WM_MOUSEWHEEL:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					return phe->OnMouseWheel(LOWORD(wParam), HIWORD(wParam), pt);
				}

				case WM_PAINT:
					return phe->OnPaint();

				case WM_SETFOCUS:
					return phe->OnSetFocus((HWND)wParam);

				case WM_SIZE:
					return phe->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));

				case WM_SYSCOLORCHANGE:
					// TODO
					break;

				case WM_TIMER:
					return phe->OnTimer(wParam);
			}
		}
	}
#ifndef _DEBUG
	__except(CZDoc::AccessDeniedHandler(GetExceptionCode(), NULL, phe))
	{
		return 0;
	}
#endif
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CHexEdit::Initialize()
{
	CZView::Initialize();
	m_fOnlyEditing = false;
	m_ichMaxTopLine = m_ichTopLine = m_iprTopLine = 0;
	m_cchPerLine = 16;
	m_nByteOffset = 10;
	m_nTextOffset = m_nByteOffset + (m_cchPerLine * 3) + 1;
	m_nTotalWidth = m_nTextOffset + m_cchPerLine;
	m_ptCaretPos.x = g_fg.m_rcMargin.left + m_nByteOffset * g_fg.m_tmFixed.tmAveCharWidth;
	m_ptCaretPos.y = g_fg.m_rcMargin.top;
	m_fCaretInText = false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CHexEdit::InitializeDC(HDC hdc)
{
	Assert(hdc);
	::SelectObject(hdc, g_fg.m_hFontFixed);
	::SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_UPDATECP);
	::SetBkColor(hdc, 0xFFFFFF);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CHexEdit::CharFromLine(UINT iLine)
{
	return min(iLine, GetLineCount()) * m_cchPerLine;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CHexEdit::CharFromPoint(POINT point, bool fUpdateCaretPos)
{
	if (!m_hwnd || m_pzd->GetAccessError())
		return 0;

	POINT ptNew = m_ptCaretPos;
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	point.x = min(rect.right - m_rcScreen.right, max(m_rcScreen.left, point.x));
	point.y = min(rect.bottom - m_rcScreen.bottom, max(m_rcScreen.top, point.y)) - m_rcScreen.top;

	// Find the number of lines from the top of the client area
	int iLine = point.y / g_fg.m_tmFixed.tmHeight;
	ptNew.y = iLine * g_fg.m_tmFixed.tmHeight + m_rcScreen.top;

	int nAveCharWidth = g_fg.m_tmFixed.tmAveCharWidth;
	int nOffset = m_rcScreen.left - g_fg.m_rcMargin.left;
	int nByteOffset = nAveCharWidth * m_nByteOffset + nOffset;
	int nTextOffset = nAveCharWidth * m_nTextOffset + nOffset;
	int nTotalWidth = nAveCharWidth * m_nTotalWidth + nOffset;
	if (point.x < nByteOffset)
		point.x = nByteOffset;
	if (point.x > nTotalWidth)
		point.x = nTotalWidth;

	int cch;
	if (point.x >= nTextOffset)
	{
		cch = (point.x - nTextOffset) / nAveCharWidth;
		ptNew.x = m_rcScreen.left + nTextOffset + (cch * nAveCharWidth);
	}
	else
	{
		cch = (point.x - nByteOffset + nAveCharWidth) / (nAveCharWidth * 3);
		ptNew.x = m_rcScreen.left + nByteOffset + (cch * 3 * nAveCharWidth);
	}
	if (cch < 0)
		cch = 0;
	if (fUpdateCaretPos)
		m_ptCaretPos = ptNew;
	UINT ich = m_ichTopLine + (iLine * m_cchPerLine) + cch;
	UINT cchTotal = m_pzd->GetCharCount();
	if (ich > cchTotal)
	{
		if (fUpdateCaretPos)
			m_ptCaretPos = PointFromChar(cchTotal);
		return cchTotal;
	}
	return ich;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CHexEdit::Create(CZFrame * pzf, HWND hwndParent, UINT nStyle, UINT nExtendedStyle)
{
	AssertPtr(pzf);
	m_hwnd = ::CreateWindowEx(nExtendedStyle, "DRZ_HexEdit", "", nStyle,
		0, 0, 0, 0, hwndParent, NULL, g_fg.m_hinst, this);
	if (!m_hwnd)
		return false;

	if (FAILED(CZDropSource::Create(&m_pds)))
		return false;
	m_pzf = pzf;
	m_pzd = pzf->GetDoc();

	::SetWindowLong(m_hwnd, GWL_USERDATA, (long)this);

	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_PAGE | SIF_POS | SIF_RANGE };
	m_pzf->SetScrollInfo(this, SB_VERT, &si);
	m_pzf->SetScrollInfo(this, SB_HORZ, &si);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CHexEdit::DrawLine(ParaDrawInfo * ppdi)
{
	const char * kpszHexChar = "0123456789ABCDEF";
	AssertPtr(m_pzd);
	AssertPtr(ppdi);
	if (m_pzd->GetAccessError() || !ppdi->hdcMem || !ppdi->pdp || !ppdi->pvStart)
		return false;
	char * pszLine = new char[m_nTotalWidth];
	if (!pszLine)
		return false;
	sprintf(pszLine, "%08X  ", LineFromChar(ppdi->ich) * m_cchPerLine);
	char * prgch = pszLine + 10;
	char * prgchText = prgch + (m_cchPerLine * 3) + 1;
	int nShift = m_pzd->IsAnsi(ppdi->pdp, ppdi->iprInPart) ? 0 : 1;
	void * pvStop = (char *)ppdi->pdp->rgpv[ppdi->iprInPart] + (CharsInPara(ppdi->pdp, ppdi->iprInPart) << nShift);
	BYTE ch;
	for (UINT ich = 0; ich < m_cchPerLine; ich++)
	{
		if (ppdi->pdp && ppdi->pvStart < pvStop)
		{
			ch = *((BYTE *)ppdi->pvStart);
			*prgch++ = kpszHexChar[ch / 16];
			*prgch++ = kpszHexChar[ch % 16];
			*prgch++ = ' ';
			if (iscntrl(ch))
				*prgchText++ = '.';
			else
				*prgchText++ = ch;
			ppdi->pvStart = (char *)ppdi->pvStart + 1;
			if (ppdi->pvStart >= pvStop)
			{
				if (++ppdi->iprInPart >= ppdi->pdp->cpr)
				{
					ppdi->pdp = ppdi->pdp->pdpNext;
					ppdi->iprInPart = 0;
				}
				if (ppdi->pdp)
				{
					nShift = m_pzd->IsAnsi(ppdi->pdp, ppdi->iprInPart) ? 0 : 1;
					ppdi->pvStart = ppdi->pdp->rgpv[ppdi->iprInPart];
					pvStop = (char *)ppdi->pvStart + (CharsInPara(ppdi->pdp, ppdi->iprInPart) << nShift);
				}
			}
		}
		else
		{
			if (ich == 0)
			{
				delete pszLine;
				return false;
			}
			*prgch++ = ' ';
			*prgch++ = ' ';
			*prgch++ = ' ';
			*prgchText++ = ' ';
		}
	}
	ppdi->ich += m_cchPerLine;
	*prgch = ' ';

	TextOut(ppdi->hdcMem, 0, 0, pszLine, prgchText - pszLine);
	delete pszLine;
	::MoveToEx(ppdi->hdcMem, ppdi->rcBounds.left,
		ppdi->rcBounds.top + (++ppdi->cli * ppdi->dypLine), NULL);
	if (ppdi->cli > ppdi->cLineLimit)
		return false;
	return (ppdi->pdp != NULL);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CHexEdit::DrawSelection(HDC hdcScreen)
{
	if (m_pzd->GetAccessError())
		return;

	RECT rect;
	::GetClientRect(m_hwnd, &rect);
	::SelectClipRgn(hdcScreen, NULL);  // Remove any previous clip region.

	HRGN hrgnSelByte = NULL;
	HRGN hrgnSelText = NULL;
	if (m_ichSelMin != m_ichSelLim)
	{
		// Get the coordinates of the selection (in the text column)
		bool fCaretInText = m_fCaretInText;
		m_fCaretInText = true;
		POINT ptStart = PointFromChar(m_ichSelMin);
		POINT ptStop = PointFromChar(m_ichSelLim);
		POINT ptT;
		m_fCaretInText = fCaretInText;

		if (ptStop.y >= g_fg.m_rcMargin.top && ptStart.y < rect.bottom)
		{
			// Hilight the selection that is visible in the memory DC.
			int nCharHeight = g_fg.m_tmFixed.tmHeight;
			int nCharWidth = g_fg.m_tmFixed.tmAveCharWidth;
			int nTextOffset = g_fg.m_rcMargin.left + m_nTextOffset * nCharWidth;
			int nByteOffset = g_fg.m_rcMargin.left + m_nByteOffset * nCharWidth;
			int cchBeforeSel = m_ichSelMin % m_cchPerLine;
			if (ptStart.y < g_fg.m_rcMargin.top)
			{
				ptStart.x = nTextOffset;
				ptStart.y = g_fg.m_rcMargin.top;
				cchBeforeSel = 0;
			}
			int nSelWidth = (m_cchPerLine - cchBeforeSel) * nCharWidth;
			if (ptStop.y > rect.bottom)
				ptStop.y = rect.bottom - g_fg.m_rcMargin.bottom;
			ptT = ptStart;

			HRGN hrgnT = NULL;

			if (ptT.y < ptStop.y)
			{
				// Draw the first (partial) line.
				hrgnSelText = ::CreateRectRgn(ptT.x, ptT.y, ptT.x + nSelWidth, ptT.y + nCharHeight);
				int nT = nByteOffset + cchBeforeSel * 3 * nCharWidth;
				hrgnSelByte = ::CreateRectRgn(nT, ptT.y, nT + nSelWidth * 3 - nCharWidth, ptT.y + nCharHeight);

				ptT.x = nTextOffset;
				ptT.y += nCharHeight;
				cchBeforeSel = 0;
			}
			nSelWidth = m_cchPerLine * nCharWidth;
			while (ptT.y < ptStop.y)
			{
				hrgnT = ::CreateRectRgn(nTextOffset, ptT.y, nTextOffset + nSelWidth, ptT.y + nCharHeight);
				if (hrgnSelText)
				{
					::CombineRgn(hrgnSelText, hrgnSelText, hrgnT, RGN_OR);
					::DeleteObject(hrgnT);
				}
				else
					hrgnSelText = hrgnT;

				hrgnT = ::CreateRectRgn(nByteOffset, ptT.y, nByteOffset + nSelWidth * 3 - nCharWidth,
					ptT.y + nCharHeight);
				if (hrgnSelByte)
				{
					::CombineRgn(hrgnSelByte, hrgnSelByte, hrgnT, RGN_OR);
					::DeleteObject(hrgnT);
				}
				else
					hrgnSelByte = hrgnT;

				ptT.y += nCharHeight;
				cchBeforeSel = 0;
			}
			nSelWidth = ((m_ichSelLim % m_cchPerLine) - cchBeforeSel) * nCharWidth;
			Assert(nSelWidth >= 0);
			if (nSelWidth)
			{
				hrgnT = ::CreateRectRgn(ptT.x, ptT.y, ptT.x + nSelWidth, ptT.y + nCharHeight);
				if (hrgnSelText)
				{
					::CombineRgn(hrgnSelText, hrgnSelText, hrgnT, RGN_OR);
					::DeleteObject(hrgnT);
				}
				else
					hrgnSelText = hrgnT;

				int nT = nByteOffset + (cchBeforeSel * nCharWidth * 3);
				hrgnT = ::CreateRectRgn(nT, ptT.y, nT + nSelWidth * 3 - nCharWidth,
					ptT.y + nCharHeight);
				if (hrgnSelByte)
				{
					::CombineRgn(hrgnSelByte, hrgnSelByte, hrgnT, RGN_OR);
					::DeleteObject(hrgnT);
				}
				else
					hrgnSelByte = hrgnT;
			}
		}
	}

	// We now have four HRGNs:
	//   m_hrgnSelText and m_hrgnSelByte contain the region of the previous selections,
	//      or NULL if there wasn't one.
	//   hrgnSelByte and hrgnSelText contain the region of the new selection.
	// We want to ignore the intersection of these regions, then invert the rest of both regions.

	// Invert the text selection.
	HRGN hrgnToInvert = NULL;
	if (!m_hrgnSelText)
		hrgnToInvert = hrgnSelText;
	else if (!hrgnSelText)
		hrgnToInvert = m_hrgnSelText;
	else
	{
		hrgnToInvert = ::CreateRectRgn(0, 0, 0, 0);
		::CombineRgn(hrgnToInvert, hrgnSelText, m_hrgnSelText, RGN_XOR);
	}
	if (hrgnToInvert)
	{
		::InvertRgn(hdcScreen, hrgnToInvert);
		if (hrgnToInvert != hrgnSelText)
		{
			// Only delete the region if it's not the new region we're going to save.
			::DeleteObject(hrgnToInvert);
		}
	}
	// Keep track of the new selection region.
	if (m_hrgnSelText)
		::DeleteObject(m_hrgnSelText);
	m_hrgnSelText = hrgnSelText;

	// Invert the byte selection.
	if (!m_hrgnSelByte)
		hrgnToInvert = hrgnSelByte;
	else if (!hrgnSelByte)
		hrgnToInvert = m_hrgnSelByte;
	else
	{
		hrgnToInvert = ::CreateRectRgn(0, 0, 0, 0);
		::CombineRgn(hrgnToInvert, hrgnSelByte, m_hrgnSelByte, RGN_XOR);
	}
	if (hrgnToInvert)
	{
		::InvertRgn(hdcScreen, hrgnToInvert);
		if (hrgnToInvert != hrgnSelByte)
		{
			// Only delete the region if it's not the new region we're going to save.
			::DeleteObject(hrgnToInvert);
		}
	}
	// Keep track of the new selection region.
	if (m_hrgnSelByte)
		::DeleteObject(m_hrgnSelByte);
	m_hrgnSelByte = hrgnSelByte;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
// Also defined in HE_Virtual.cpp.
inline UINT CHexEdit::GetLineCount()
{
	AssertPtr(m_pzd);
	return (m_pzd->GetCharCount() / m_cchPerLine) + 1;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
SelectionType CHexEdit::GetSelection(UINT * piStart, UINT * piStop)
{
	if (piStart)
		*piStart = m_ichSelMin;
	if (piStop)
		*piStop = m_ichSelLim;
	if (m_ichSelMin != m_ichDragStart)
		return kstSelAfter;
	else if (m_ichSelMin != m_ichSelLim)
		return kstSelBefore;
	else
		return kstNone;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
char * CHexEdit::GetCurPosString()
{
	strcpy(m_szCurPos, "Fix this.");
	return m_szCurPos;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CHexEdit::LineFromChar(UINT ich)
{
	if (ich == (UINT)-1)
		ich = m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin;
	return min(ich, m_pzd->GetCharCount()) / m_cchPerLine;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CHexEdit::NotifyDelete(UINT ichMin, UINT ichStop, bool fRedraw)
{
	/*	There are six possible combinations of ichMin, ichStop,
		m_ichSelMin, and m_ichSelLim. These are given below: (D-delete, S-selection)
		1)	D--D  S--S
		2)	D--S--D--S
		3)	D--S--S--D
		4)	S--D--D--S
		5)	S--D--S--D
		6)	S--S  D--D */
	if (ichMin == ichStop)
		return;
	bool fCaretAtEnd = false;
	if (m_ichDragStart == m_ichSelMin)
		fCaretAtEnd = true;
	UINT cch = ichStop - ichMin;
	if (ichMin < m_ichSelMin)
	{
		if (ichStop <= m_ichSelMin)
		{
			// Case 1
			m_ichSelMin -= cch;
			m_ichSelLim -= cch;
		}
		else
		{
			m_ichSelMin = ichMin;
			if (ichStop < m_ichSelLim)
			{
				// Case 2
				m_ichSelLim -= cch;
			}
			else
			{
				// Case 3
				m_ichSelLim = ichMin;
			}
		}
	}
	else if (ichMin < m_ichSelLim) // otherwise Case 6
	{
		if (ichStop < m_ichSelLim)
		{
			// Case 4
			m_ichSelLim -= cch;
		}
		else
		{
			// Case 5
			m_ichSelLim = ichMin;
		}
	}

	m_ichDragStart = fCaretAtEnd ? m_ichSelMin : m_ichSelLim;
	if (ichMin < m_ichTopLine)
		m_ichTopLine -= cch;

	if (!fRedraw)
		return;

	m_ptCaretPos = PointFromChar(fCaretAtEnd ? m_ichSelLim : m_ichSelMin);
	if (GetFocus() == m_hwnd)
		SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);

	// TODO
	/*if (ichMin >= m_ichTopLine && ichMin <= m_prgnLineInfo[m_cli][klpChar])
	{
		InvalidateRect(m_hwnd, NULL, FALSE);
		//UpdateWindow(m_hwnd);
	}*/

	// TODO: Fix the vertical scrollbar
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CHexEdit::NotifyInsert(UINT ichMin, UINT cch)
{
	if (!cch)
		return;
	if (ichMin < m_ichSelLim)
	{
		m_ichSelLim += cch;
		if (ichMin < m_ichSelMin)
			m_ichSelMin += cch;
	}

	if (ichMin < m_ichTopLine)
		m_ichTopLine += cch;
	// TODO
	/*if (ichMin >= m_ichTopLine && ichMin <= m_prgnLineInfo[m_cli][klpChar])
	{
		InvalidateRect(m_hwnd, NULL, FALSE);
		//UpdateWindow(m_hwnd);
	}*/

	// TODO: Fix the vertical scrollbar
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
POINT CHexEdit::PointFromChar(UINT ich)
{
	AssertPtr(m_pzd);
	int nAveCharWidth = g_fg.m_tmFixed.tmAveCharWidth;
	POINT point = {m_rcScreen.left + (m_nByteOffset * nAveCharWidth), m_rcScreen.top};
	if (m_pzd->GetCharCount() == 0 || m_pzd->GetAccessError())
		return point;
	UINT iLine = LineFromChar(ich);
	UINT iFirstLine = LineFromChar(m_ichTopLine);
	if (iLine < iFirstLine)
	{
		point.y = -g_fg.m_tmFixed.tmHeight;
	}
	else if (iLine > iFirstLine + m_cli)
	{
		RECT rect;
		GetClientRect(m_hwnd, &rect);
		point.y = rect.bottom;
	}
	else
	{
		UINT iLineOffset = iLine - iFirstLine;
		point.y += iLineOffset * g_fg.m_tmFixed.tmHeight;
		UINT iLineChar = CharFromLine(iLine);
		if (m_fCaretInText)
		{
			point.x = m_rcScreen.left + (m_nTextOffset + ich - iLineChar) * nAveCharWidth;
		}
		else
		{
			Assert((UINT)(ich - iLineChar) < m_cchPerLine);
			point.x += (ich - iLineChar) * (nAveCharWidth * 3);
		}
	}
	return point;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CHexEdit::Print(PrintInfo * ppi)
{
	// TODO
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CHexEdit::RecalculateLines(int dyp)
{
	Assert(m_pzd);

	int dypFont = g_fg.m_tmFixed.tmHeight;
	int cLines = 0;
	if (dypFont)
		cLines = (dyp - g_fg.m_rcMargin.top - g_fg.m_rcMargin.bottom) / dypFont;
	m_cli = cLines;
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CHexEdit::SetSelection(UINT ichMin, UINT ichSelStop, bool fScroll, bool fForceRedraw)
{
	AssertPtr(m_pzd);
	UINT cchTotal = m_pzd->GetCharCount();
	if (ichMin > cchTotal)
		ichMin = cchTotal;
	if (ichSelStop > cchTotal)
		ichSelStop = cchTotal;
	int fShowCaret = 0; // from non-selection to non-selection
	if (ichMin == ichSelStop && m_ichSelMin != m_ichSelLim) // from selection to non-selection
		fShowCaret = 1; // Show the cursor
	else if (ichMin != ichSelStop)
	{
		if (m_ichSelMin == m_ichSelLim) // from non-selection to selection
			fShowCaret = 2; // Hide the cursor
		else // from selection to selection
			fShowCaret = 3; // Don't do anything to the cursor
	}
	if (ichMin <= ichSelStop)
	{
		m_ichSelMin = ichMin;
		m_ichSelLim = ichSelStop;
		m_ichDragStart = ichMin;
	}
	else
	{
		m_ichSelMin = ichSelStop;
		m_ichSelLim = ichMin;
		m_ichDragStart = ichMin;
	}
	bool fRedraw = true;
	if (fScroll)
	{
		UINT iLineStart = LineFromChar(m_ichSelMin);
		UINT iLineStop = LineFromChar(m_ichSelLim);
		UINT iFirstLine = LineFromChar(m_ichTopLine);
		UINT ich = (m_ichDragStart == m_ichSelMin) ? m_ichSelLim : m_ichSelMin;
		if (ich < m_ichTopLine || ich > m_ichTopLine + (m_cchPerLine * m_cli - m_cchPerLine))
		{
			UINT iLine = LineFromChar(ich);
			if (iLine > m_cli / 2)
				iLine -= m_cli / 2;
			else
				iLine = 0;
			m_ichTopLine = CharFromLine(min(iLine, GetLineCount() - m_cli + 1));
		}
		SCROLLINFO si = {sizeof(SCROLLINFO), SIF_PAGE | SIF_POS | SIF_RANGE};
		si.nPos = LineFromChar(m_ichTopLine);
		si.nMax = GetLineCount();
		si.nPage = m_cli;
		m_pzf->SetScrollInfo(this, SB_VERT, &si);
	}
	if (fShowCaret == 0 || fShowCaret == 2) // The cursor is currently showing, so hide it
		HideCaret(m_hwnd);
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
	m_ptCaretPos = PointFromChar(ichSelStop);
	SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	if (fShowCaret == 0 || fShowCaret == 1) // The caret needs to be showing
		ShowCaret(m_hwnd);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CHexEdit::ShowCharAtLine(UINT ipr, UINT ich, int iLine)
{
	AssertPtr(m_pzd);
	if (m_pzd->GetAccessError() || !m_pzd->GetCharCount())
		return 0;

	// Find out the line offset from the beginning of the para
	if (iLine < 0)
		return m_ichTopLine;
	UINT * prgichPos = new UINT[m_cli + 1];
	if (!prgichPos)
		return 0;
	UINT iFirstPara = ich / m_cchPerLine;
	if (iFirstPara > (UINT)iLine)
		iFirstPara -= iLine;
	return iFirstPara * m_cchPerLine;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CHexEdit::ShowText(HDC hdcScreen, RECT & rect)
{
	if (m_hrgnSelText)
	{
		::DeleteObject(m_hrgnSelText);
		m_hrgnSelText = NULL;
	}
	if (m_hrgnSelByte)
	{
		::DeleteObject(m_hrgnSelByte);
		m_hrgnSelByte = NULL;
	}

	if (!m_pzd || m_pzd->GetAccessError() || !m_pzd->GetCharCount())
	{
		::GetClientRect(m_hwnd, &rect);
		::SetBkColor(hdcScreen, g_fg.m_cr[kclrWindow]);
		::ExtTextOut(hdcScreen, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
		::TextOut(hdcScreen, m_rcScreen.left, m_rcScreen.top, "00000000", 8);
		return;
	}

	::MoveToEx(hdcScreen, rect.left, rect.top, NULL);
	ParaDrawInfo pdi;
	DocPart * pdp = m_pzd->GetPart(m_ichTopLine, false, &pdi.pvStart, &pdi.iprInPart);
	pdi.ich = m_ichTopLine;
	pdi.hdcMem = hdcScreen;
	pdi.cli = 0;
	pdi.cLineLimit = m_cli;
	pdi.dypLine = g_fg.m_tmFixed.tmHeight;
	pdi.pdp = pdp;
	pdi.rcBounds = rect;
	pdi.rcScreen = m_rcScreen;

	while (DrawLine(&pdi) && pdi.cli <= pdi.cLineLimit);
	// TODO: See if this is necessary.
	if (!(m_pzd->GetCharCount() % m_cchPerLine))
	{
		char szText[9];
		sprintf(szText, "%08X", LineFromChar(pdi.ich) * m_cchPerLine);
		::TextOut(hdcScreen, 0, 0, szText, 8);
	}

	// Blank out any white space at the bottom of the window.
	POINT pt;
	::MoveToEx(hdcScreen, 0, 0, &pt);
	RECT rcBottom = { rect.left, pt.y, rect.right, rect.bottom };
	::SetBkColor(hdcScreen, g_fg.m_cr[kclrWindow]);
	::ExtTextOut(hdcScreen, 0, 0, ETO_OPAQUE, &rcBottom, NULL, 0, NULL);

	DrawSelection(hdcScreen);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CHexEdit::UpdateScrollBars()
{
	AssertPtr(m_pzd);

	// Update scrollbars
	SCROLLINFO si = {sizeof(si), SIF_PAGE | SIF_RANGE};
	UINT cch = m_pzd->GetCharCount();
	m_ichMaxTopLine = ShowCharAtLine(m_pzd->GetParaCount(), cch, m_cli - 2);
	si.nPage = cch - m_ichMaxTopLine;
	si.nMax = m_ichMaxTopLine ? cch - 1 : 0;
	m_pzf->SetScrollInfo(this, SB_VERT, &si);

	// Update scrollbars
	//SCROLLINFO si = {sizeof(si), SIF_PAGE | SIF_RANGE};
	// TODO
	/*si.nPage = m_cli;
	si.nMax = m_pzd->GetParaCount() + si.nPage;
	m_pzf->SetScrollInfo(this, SB_VERT, &si);

	RECT rect;
	GetClientRect(m_hwnd, &rect);
	si.nPage = rect.right;
	si.nMax = m_dwCharsInLongestLine * g_fg.m_tm.tmMaxCharWidth + si.nPage;
	m_pzf->SetScrollInfo(this, SB_HORZ, &si);*/
}