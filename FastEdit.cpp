#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"

/***********************************************************************************************
	DocPart methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
DocPart::DocPart(DocPart * pdpBefore, DocPart * pdpAfter)
{
	cpr = 0;
	cch = 0;
	memset(rgcch, 0, kcprInPart * sizeof(DWORD));
	memset(rgpv, 0, kcprInPart * sizeof(void *));
	pdpPrev = pdpBefore;
	if (pdpPrev)
		pdpPrev->pdpNext = this;
	pdpNext = pdpAfter;
	if (pdpNext)
		pdpNext->pdpPrev = this;
}


/***********************************************************************************************
	CFastEdit methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	FastEdit window procedure.
----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK CFastEdit::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CFastEdit * pfe = (CFastEdit *)::GetWindowLong(hwnd, GWL_USERDATA);
	AssertPtrN(pfe);

#ifndef _DEBUG
	__try
#endif
	{
		if (pfe)
		{
			switch (iMsg)
			{
			case WM_CHAR:
				pfe->OnChar(wParam, LOWORD(lParam), HIWORD(lParam));
				return 0;

			case WM_CONTEXTMENU:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					pfe->OnContextMenu((HWND)wParam, pt);
					break;
				}

			case WM_KEYDOWN:
				return pfe->OnKeyDown(wParam, LOWORD(lParam), HIWORD(lParam));

			case WM_KILLFOCUS:
				return pfe->OnKillFocus((HWND)wParam);

			case WM_LBUTTONDBLCLK:
				{
					POINT pt = { max(0, (short int)LOWORD(lParam)),
						max(0, (short int)HIWORD(lParam)) };
					pfe->OnLButtonDblClk(wParam, pt);
					break;
				}

			case WM_LBUTTONDOWN:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					pfe->OnLButtonDown(wParam, pt);
					break;
				}

			case WM_LBUTTONUP:
				{
					POINT point = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					pfe->OnLButtonUp(wParam, point);
					break;
				}

			case WM_MBUTTONDOWN:
				{
					POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					pfe->OnMButtonDown(wParam, pt);
					break;
				}

			case WM_MOUSEMOVE:
				{
					POINT point = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					return pfe->OnMouseMove(wParam, point);
				}

			case WM_MOUSEWHEEL:
				{
					POINT point = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
					return pfe->OnMouseWheel(LOWORD(wParam), HIWORD(wParam), point);
				}

			case WM_PAINT:
				return pfe->OnPaint();

			case WM_SETFOCUS:
				return pfe->OnSetFocus((HWND)wParam);

			case WM_SIZE:
				return pfe->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));

			case WM_SYSCOLORCHANGE:
				// TODO
				break;

			case WM_TIMER:
				return pfe->OnTimer(wParam);
			}
		}
	}
#ifndef _DEBUG
	__except(CZDoc::AccessDeniedHandler(::GetExceptionCode(), NULL, pfe))
	{
		return 0;
	}
#endif
	return ::DefWindowProc(hwnd, iMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CFastEdit::CFastEdit(CZEditFrame * pzef) : CZView(pzef)
{
	m_prgli = NULL;
	m_cli = 0;
	Initialize();
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CFastEdit::~CFastEdit()
{
	if (m_prgli)
	{
		delete [] m_prgli;
		m_prgli = NULL;
	}
	if (m_hwnd)
	{
		::DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}
	if (m_pds)
	{
		m_pds->Release();
		m_pds = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::Initialize()
{
	CZView::Initialize();
	m_fOnlyEditing = false;
	m_fWrap = true;
	m_ichMaxTopLine = m_ichTopLine = m_iprTopLine = 0;
	m_ptRealPos.x = m_ptCaretPos.x = g_fg.m_rcMargin.left;
	m_ptRealPos.y = m_ptCaretPos.y = g_fg.m_rcMargin.top;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFastEdit::Create(CZFrame * pzf, HWND hwndParent, UINT nStyle, UINT nExtendedStyle)
{
	AssertPtr(pzf);
	m_hwnd = ::CreateWindowEx(nExtendedStyle, "DRZ_FastEdit", "", nStyle,
		0, 0, 0, 0, hwndParent, NULL, g_fg.m_hinst, this);
	if (!m_hwnd)
		return false;
	if (FAILED(CZDropSource::Create(&m_pds)))
		return false;
	m_pzf = pzf;
	m_pzd = pzf->GetDoc();
	::SetWindowLong(m_hwnd, GWL_USERDATA, (long)this);

	SCROLLINFO si = { sizeof(si), SIF_PAGE | SIF_POS | SIF_RANGE };
	m_pzf->SetScrollInfo(this, SB_VERT, &si);
	m_pzf->SetScrollInfo(this, SB_HORZ, &si);
	return true;
}


/*----------------------------------------------------------------------------------------------
	Return a string with the following form giving the current caret position:
		Para X,  Col X
----------------------------------------------------------------------------------------------*/
char * CFastEdit::GetCurPosString()
{
	AssertPtr(m_pzd);

	UINT ich = (m_ichSelMin != m_ichDragStart) ? m_ichSelMin : m_ichSelLim;
	void * pv;
	UINT ipr;
	UINT cprBefore;
	DocPart * pdp = m_pzd->GetPart(ich, false, &pv, &ipr, NULL, &cprBefore);
	int cchSpacesInTab = g_fg.m_ri.m_cchSpacesInTab;

	int iCol = 0;
	if (m_pzd->IsAnsi(pdp, ipr))
	{
		char * prgch = (char *)pdp->rgpv[ipr];
		char * prgchLim = (char *)pv;
		while (prgch < prgchLim)
		{
			if (*prgch++ != '\t')
				iCol++;
			else
				iCol += cchSpacesInTab - (iCol % cchSpacesInTab);
		}
	}
	else
	{
		wchar * prgch = (wchar *)pdp->rgpv[ipr];
		wchar * prgchLim = (wchar *)pv;
		while (prgch < prgchLim)
		{
			if (*prgch++ != '\t')
				iCol++;
			else
				iCol += cchSpacesInTab - (iCol % cchSpacesInTab);
		}
	}
	sprintf_s(m_szCurPos, "Para %d,  Col %d", cprBefore + ipr + 1, iCol + 1);
	return m_szCurPos;
}



// ZZZ
/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFastEdit::DrawPara(ParaDrawInfo * pdi, bool fAnsi, UINT ichStop)
{
	AssertPtr(m_pzd);
	AssertPtr(pdi);
	if (m_pzd->GetAccessError())
		return false;
	DocPart *& pdp = pdi->pdp;
	UINT cch = PrintCharsInPara(pdp, pdi->iprInPart);

	void * pv = NULL;
	if (pdi->pvStart)
	{
		pv = pdi->pvStart;
		pdi->pvStart = NULL;
		if (fAnsi)
			cch -= ((char *)pv - (char *)pdp->rgpv[pdi->iprInPart]);
		else
			cch -= ((wchar *)pv - (wchar *)pdp->rgpv[pdi->iprInPart]);
	}
	else if (pdi->iprInPart < pdp->cpr)
	{
		pv = pdp->rgpv[pdi->iprInPart];
	}
	if (pv == NULL)
		return false;
	if ((UINT)++pdi->cli > m_cli + 1)
		return false;
	if (pdi->ich + cch > ichStop)
		cch = ichStop - pdi->ich;
	void * pvParaStart = pdp->rgpv[pdi->iprInPart];
	void * pvStart = pv;
	void * pvStop;
	if (pdi->ich == 0)
	{
		// Handle special conditions at the beginning of the file.
		if (!fAnsi && *((wchar *)pv) == 0xFEFF)
		{
			pvParaStart = pv = ((wchar *)pv) + 1;
			cch--;
			pdi->ich++;
		}
	}
	if (fAnsi)
		pvStop = (char *)pv + cch;// - 1;
	else
		pvStop = (wchar *)pv + cch;// - 1;
	int dxpLine = 0;
	int cchLine;
	pdi->prgli[pdi->cli].nSyntax = ::GetTextColor(pdi->hdcMem) | BlockToSyntax(pdi->fInBlock) | BlockPToSyntax(pdi->fInBlockP);
	if (cch > 0)
	{
		while (pv < pvStop)
		{
			if (m_pzd->GetAccessError())
				return false;
			if (fAnsi)
			{
				dxpLine = pdi->rcBounds.right - pdi->rcBounds.left;
				cchLine = DrawLineA(pdi->hdcMem, (char *)pvParaStart, (char *)pv, (char *)pvStop,
					&dxpLine, pdi->rcScreen.left, pdi->ri,
					pdi->fWrap, true, pdi->fShowColors, pdi->fInBlock, pdi->fInBlockP);
				pv = (char *)pv + cchLine;
			}
			else
			{
				dxpLine = pdi->rcBounds.right - pdi->rcBounds.left;
				cchLine = DrawLineW(pdi->hdcMem, (wchar *)pvParaStart, (wchar *)pv, (wchar *)pvStop,
					&dxpLine, pdi->rcScreen.left, pdi->ri,
					pdi->fWrap, true, pdi->fShowColors, pdi->fInBlock, pdi->fInBlockP);
				pv = (wchar *)pv + cchLine;
			}

			// Move the current location to the beginning of the next line.
			POINT pt;
			MoveToEx(pdi->hdcMem, 0, 0, &pt);
			MoveToEx(pdi->hdcMem, pdi->rcBounds.left, pt.y + pdi->dypLine, NULL);

			// Blank the rest of the current line.
			RECT rc = { pt.x, pt.y, pdi->rcBounds.right, pt.y + pdi->dypLine };
			::ExtTextOut(pdi->hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

			pdi->ich += cchLine;
			if (pv < pvStop)
			{
				pdi->prgli[pdi->cli - 1].dxp = dxpLine;
				if (pdi->cli > pdi->cLineLimit)
					return false;
				pdi->prgli[pdi->cli].ich = pdi->ich;
				pdi->prgli[pdi->cli].ipr = pdi->ipr;
				pdi->prgli[pdi->cli].nSyntax = ::GetTextColor(pdi->hdcMem) | BlockToSyntax(pdi->fInBlock) | BlockPToSyntax(pdi->fInBlockP);
				pdi->cli++;
			}
		}
	}
	else
	{
		dxpLine = pdi->ri.CharOut(pdi->hdcMem, g_fg.m_chPara);
		POINT pt;
		MoveToEx(pdi->hdcMem, 0, 0, &pt);
		MoveToEx(pdi->hdcMem, pdi->rcBounds.left, pt.y + pdi->dypLine, NULL);

		// Blank the rest of the current line.
		RECT rc = { pt.x, pt.y, pdi->rcBounds.right, pt.y + pdi->dypLine };
		::ExtTextOut(pdi->hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	}
	pdi->ich += CharsAtEnd(pdp, pdi->iprInPart);

	pdi->fInBlockP = false;
	if (!pdi->fInBlock)
		::SetTextColor(pdi->hdcMem, g_fg.m_cr[kclrText]);

	//if (++pdi->iprInPart >= pdp->cpr)
	if (++pdi->iprInPart >= pdp->cpr && pdp->pdpNext)
	{
		pdi->iprInPart = 0;
		pdp = pdp->pdpNext;
	}
	pdi->prgli[pdi->cli - 1].dxp = dxpLine;
	if (pdi->cli > pdi->cLineLimit || pdi->ich >= ichStop)
		return false;
	pdi->prgli[pdi->cli].ich = pdi->ich;
	pdi->prgli[pdi->cli].ipr = ++pdi->ipr;
	pdi->prgli[pdi->cli].nSyntax = ::GetTextColor(pdi->hdcMem) | BlockToSyntax(pdi->fInBlock) | BlockPToSyntax(pdi->fInBlockP);
	if (pdi->ich == m_pzd->GetCharCount())
	{
		Assert(!pdp || !pdp->pdpNext);
		pdi->ich++;
		return false;
	}
	return pdp != NULL;
}


// ZZZ
/*----------------------------------------------------------------------------------------------
	If fDraw is TRUE, the text will be drawn into the HDC. Otherwise, it will only be measured.
----------------------------------------------------------------------------------------------*/
int CFastEdit::DrawLineA(HDC hdcMem, char * prgchParaMin, char * prgchMin, char * prgchLim,
	int * pdxpLine, int nTabLeft, RenderInfo & ri,
	bool fWrap, bool fDraw, bool fShowColors, bool & fInBlock, bool & fInBlockP)
{
	AssertPtr(m_pzd);
	if (m_pzd->GetAccessError() || !prgchMin || !pdxpLine)
		return 1;
	if (prgchMin >= prgchLim)
	{
		*pdxpLine = ri.GetCharWidth(g_fg.m_chPara);
		if (fDraw)
			ri.CharOut(hdcMem, g_fg.m_chPara);
		// This used to return 1.
		return 0;
	}

	int dxpTab = ri.m_dxpTab;
	int dypLine = ri.m_tm.tmHeight;

	char * prgchPos = prgchMin;
	char * prgchRun = prgchMin;
	char * prgchSpace = NULL;
	int nDrawWidth = *pdxpLine;
	int dxpLine = 0;
	int dxpLineAtSpace = 0;
	POINT pt;
	char ch;

	while (prgchPos < prgchLim)
	{
		ch = *prgchPos;
		if (ch == ' ')
		{
			prgchSpace = prgchPos;
			dxpLineAtSpace = (dxpLine += ri.GetCharWidth(g_fg.m_chSpace));
		}
		else if (ch == '\t')
		{
			dxpLine = ((dxpLine / dxpTab) + 1) * dxpTab;
			if (dxpLine <= nDrawWidth)
			{
				if (fDraw)
				{
					if (prgchPos > prgchRun)
					{
						MyTextOutA(hdcMem, prgchParaMin, prgchRun, prgchPos, fShowColors,
							true, fInBlock, fInBlockP, ri);
					}
					::MoveToEx(hdcMem, 0, 0, &pt);

					// Blank the space used by the tab.
					RECT rc = { pt.x, pt.y, dxpLine + nTabLeft, pt.y + dypLine };
					::ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

					// TODO: Draw an arrow here
					::MoveToEx(hdcMem, dxpLine + nTabLeft, pt.y, NULL);
				}
				// TODO: Does this need to be moved outside of the current brace it's in?
				prgchRun = prgchPos + 1;
				prgchSpace = prgchPos;
				dxpLineAtSpace = dxpLine;
			}
			else if (prgchPos == prgchMin)
				return 1;
		}
		else
		{
			dxpLine += ri.GetCharWidth(ch);
		}

		if (dxpLine > nDrawWidth)
		{
			if (fWrap)
			{
				if (prgchSpace)
				{
					// There was a space/tab in the line already
					prgchPos = prgchSpace + 1;
					dxpLine = dxpLineAtSpace;
				}
				else
				{
					// There wasn't a space, so find out how many characters will fit on the line
					if (prgchPos == prgchMin)
					{
						// If the window gets too small, there isn't enough room to draw even one character.
						// If this happens, we still draw the one character, even though it's too wide.
						// We'll get into an infinite loop if we don't do this, because we'll never get
						// past the one character.
						prgchPos++;
					}
					if (fDraw)
					{
						if (prgchPos > prgchRun)
						{
							MyTextOutA(hdcMem, prgchParaMin, prgchRun, prgchPos,
								fShowColors, TRUE, fInBlock, fInBlockP, ri);
						}
					}
					*pdxpLine = dxpLine;
					return prgchPos - prgchMin;
				}
			}
			else
				prgchPos++;
			if (fDraw)
			{
				if (prgchPos > prgchRun)
				{
					MyTextOutA(hdcMem, prgchParaMin, prgchRun, prgchPos - 1, fShowColors,
						TRUE, fInBlock, fInBlockP, ri);
				}
			}
			if (!fWrap)
				prgchPos = prgchLim;
			*pdxpLine = dxpLine;
			return prgchPos - prgchMin;
		}
		++prgchPos;
	}

	if (prgchPos > prgchLim)
		prgchPos = prgchLim;
	if (fDraw)
	{
		if (prgchPos > prgchRun)
		{
			MyTextOutA(hdcMem, prgchParaMin, prgchRun, prgchPos, fShowColors, TRUE,
				fInBlock, fInBlockP, ri);
		}
		ri.CharOut(hdcMem, g_fg.m_chPara);
	}
	*pdxpLine = dxpLine + ri.GetCharWidth(g_fg.m_chPara);
	return prgchPos - prgchMin;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::MyTextOutA(HDC hdcMem, char * prgchParaMin, char * prgchMin,
	char * prgchLim, bool fShowColors, bool fStartOfWord, bool & fInBlock, bool & fInBlockP,
	RenderInfo & ri)
{
	char * prgchPos = prgchMin;

	int cch;
	COLORREF cr;
	char ch;
	ColorSchemeInfo * pcsi = m_pzd->GetColorScheme();
	int cchBlockStart = *pcsi->m_szBlockStart ? lstrlen(pcsi->m_szBlockStart) : 0x7FFFFFFF;
	int cchBlockStop = *pcsi->m_szBlockStop ? lstrlen(pcsi->m_szBlockStop) : 0x7FFFFFFF;
	while (prgchPos < prgchLim)
	{
		if (fShowColors)
		{
			if (fInBlock)
			{
				// See if we need to close off the comment block.
				// NOTE: This has to wait until the last character of the block stop pattern in case
				// the pattern is split on multiple lines.
				if (cchBlockStop <= prgchPos - prgchParaMin + 1)
				{
					if (strncmp(pcsi->m_szBlockStop, prgchPos + 1 - cchBlockStop, cchBlockStop) == 0)
					{
						ri.CharOut(hdcMem, *prgchPos);
						prgchPos++;
						::SetTextColor(hdcMem, g_fg.m_cr[kclrText]);
						fInBlock = false;
						continue;
					}
				}
			}
			else
			{
				if (fInBlockP)
				{
					// See if we should close an inline section (i.e a string in double quotes).
					ri.CharOut(hdcMem, *prgchPos);
					if (*prgchPos == ch)
					{
						fInBlockP = false;
						::SetTextColor(hdcMem, g_fg.m_cr[kclrText]);
					}
					prgchPos++;
					continue;
				}
				else if (cchBlockStart <= prgchLim - prgchMin &&
					strncmp(pcsi->m_szBlockStart, prgchPos, cchBlockStart) == 0)
				{
					// See if we need to open a comment block.
					fInBlock = true;
					::SetTextColor(hdcMem, pcsi->m_crBlock);
				}
				else
				{
					if (isspace((unsigned char)*prgchPos))
					{
						// Draw the space character.
						ri.CharOut(hdcMem, g_fg.m_chSpace);
						prgchPos++;
						fStartOfWord = true;
						continue;
					}

					if (pcsi->FindBlockP(prgchPos, prgchLim - prgchPos, &cr, &ch))
					{
						// See if the rest of the line should be commented out.
						fInBlockP = true;
						cr = ::SetTextColor(hdcMem, cr);
						ri.CharOut(hdcMem, *prgchPos);
						prgchPos++;
						continue;
					}

					if (fStartOfWord || strchr(pcsi->m_pszDelim, *prgchPos))
					{
						cch = 0;
						if (pcsi->FindWord(prgchPos, prgchLim - prgchPos, pcsi->m_pszDelim, &cr, &cch))
						{
							// This is a keyword.
							if (prgchLim - prgchPos == cch ||
								strchr(pcsi->m_pszDelim, *prgchPos) ||
								strchr(pcsi->m_pszDelim, *(prgchPos + cch)))
							{
								cr = ::SetTextColor(hdcMem, cr);
								// TODO: What if the keyword needs to wrap. This needs to be changed
								// to use ri.CharOut
								::TextOutA(hdcMem, 0, 0, prgchPos, cch);
								::SetTextColor(hdcMem, cr);
								prgchPos += cch;
								continue;
							}
						}
						if (!fStartOfWord)
						{
							// This is a punctuation character, so print it out, then continue.
							ri.CharOut(hdcMem, *prgchPos);
							prgchPos++;
							fStartOfWord = true;
							continue;
						}
						fStartOfWord = true;
					}
				}
			}
		}

		fStartOfWord = false;
		ri.CharOut(hdcMem, *prgchPos);
		prgchPos++;
	}
}



// ZZZ
/*----------------------------------------------------------------------------------------------
	If fDraw is TRUE, the text will be drawn into the HDC. Otherwise, it will only be measured.
----------------------------------------------------------------------------------------------*/
int CFastEdit::DrawLineW(HDC hdcMem, wchar * prgchParaMin, wchar * prgchMin,
	wchar * prgchLim, int * pdxpLine, int nTabLeft, RenderInfo & ri,
	bool fWrap, bool fDraw, bool fShowColors, bool & fInBlock, bool & fInBlockP)
{
	AssertPtr(m_pzd);
	if (m_pzd->GetAccessError() || !prgchMin || !pdxpLine)
		return 1;
	if (prgchMin >= prgchLim)
	{
		*pdxpLine = ri.GetCharWidth(g_fg.m_chPara);
		if (fDraw)
			ri.CharOut(hdcMem, g_fg.m_chPara);
		return 1;
	}

	int dxpTab = ri.m_dxpTab;
	int dypLine = ri.m_tm.tmHeight;

	wchar * prgchPos = prgchMin;
	wchar * prgchRun = prgchMin;
	wchar * prgchSpace = NULL;
	int nDrawWidth = *pdxpLine;
	int dxpLine = 0;
	int dxpLineAtSpace = 0;
	POINT pt;
	wchar ch;

	while (prgchPos < prgchLim)
	{
		bool fSurrogate = false;
		ch = *prgchPos;
		if (ch == ' ')
		{
			prgchSpace = prgchPos;
			dxpLineAtSpace = (dxpLine += ri.GetCharWidth(g_fg.m_chSpace));
		}
		else if (ch == '\t')
		{
			dxpLine = ((dxpLine / dxpTab) + 1) * dxpTab;
			if (dxpLine <= nDrawWidth)
			{
				if (fDraw)
				{
					if (prgchPos > prgchRun)
					{
						MyTextOutW(hdcMem, prgchParaMin, prgchRun, prgchPos, fShowColors,
							true, fInBlock, fInBlockP, ri);
					}
					::MoveToEx(hdcMem, 0, 0, &pt);

					// Blank the space used by the tab.
					RECT rc = { pt.x, pt.y, dxpLine + nTabLeft, pt.y + dypLine };
					::ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

					// TODO: Draw an arrow here
					::MoveToEx(hdcMem, dxpLine + nTabLeft, pt.y, NULL);
				}
				// TODO: Does this need to be moved outside of the current brace it's in?
				prgchRun = prgchPos + 1;
				prgchSpace = prgchPos;
				dxpLineAtSpace = dxpLine;
			}
		}
		else
		{
			if (ch < 0xd800 || ch > 0xdbff)
			{
				if ((UCHAR)ch <= 0xFF)
				{
					dxpLine += ri.GetCharWidth((char)ch);
				}
				else if (ch <= 0xFFFF)
				{
					SIZE size;
					::GetTextExtentPoint32W(hdcMem, &ch, 1, &size);
					dxpLine += size.cx;
				}
			}
			else if (prgchPos < prgchLim - 1)
			{
				// This is the first character of a surrogate pair.
				SIZE size;
				::GetTextExtentPoint32W(hdcMem, prgchPos, 2, &size);
				dxpLine += size.cx;
				fSurrogate = true;
			}
		}

		if (dxpLine > nDrawWidth)
		{
			if (fWrap)
			{
				if (prgchSpace)
				{
					// There was a space/tab in the line already
					prgchPos = prgchSpace + 1;
					dxpLine = dxpLineAtSpace;
				}
				else
				{
					// There wasn't a space, so find out how many characters will fit on the line
					if (fDraw)
					{
						if (prgchPos > prgchRun)
						{
							MyTextOutW(hdcMem, prgchParaMin, prgchRun, prgchPos, fShowColors,
								true, fInBlock, fInBlockP, ri);
						}
					}
					*pdxpLine = dxpLine;
					return prgchPos - prgchMin;
				}
			}
			else
				prgchPos++;
			if (fDraw)
			{
				if (prgchPos > prgchRun)
				{
					MyTextOutW(hdcMem, prgchParaMin, prgchRun, prgchPos - 1, fShowColors,
						true, fInBlock, fInBlockP, ri);
				}
			}
			if (!fWrap)
				prgchPos = prgchLim;
			*pdxpLine = dxpLine;
			return prgchPos - prgchMin;
		}
		++prgchPos;
		if (fSurrogate)
			++prgchPos;
	}

	if (prgchPos > prgchLim)
		prgchPos = prgchLim;
	if (fDraw)
	{
		if (prgchPos > prgchRun)
		{
			MyTextOutW(hdcMem, prgchParaMin, prgchRun, prgchPos, fShowColors,
				true, fInBlock, fInBlockP, ri);
		}
		ri.CharOut(hdcMem, g_fg.m_chPara);
	}
	*pdxpLine = dxpLine + ri.GetCharWidth(g_fg.m_chPara);
	return prgchPos - prgchMin;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::MyTextOutW(HDC hdcMem, wchar * prgchParaMin, wchar * prgchMin,
	wchar * prgchLim, bool fShowColors, bool fStartOfWord, bool & fInBlock, bool & fInBlockP,
	RenderInfo & ri)
{
	wchar * prgchPos = prgchMin;

	int cch;
	COLORREF cr;
	wchar chBlock;
	ColorSchemeInfo * pcsi = m_pzd->GetColorScheme();
	int cchBlockStart = *pcsi->m_wszBlockStart ? wcslen(pcsi->m_wszBlockStart) : 0x7FFFFFFF;
	int cchBlockStop = *pcsi->m_wszBlockStop ? wcslen(pcsi->m_wszBlockStop) : 0x7FFFFFFF;
	while (prgchPos < prgchLim)
	{
		wchar ch = *prgchPos;
		if (fShowColors)
		{
			if (fInBlock)
			{
				// See if we need to close off the comment block.
				// NOTE: This has to wait until the last character of the block stop pattern in case
				// the pattern is split on multiple lines.
				if (cchBlockStop <= prgchLim - prgchParaMin + 1)
				{
					if (wcsncmp(pcsi->m_wszBlockStop, prgchPos + 1 - cchBlockStop, cchBlockStop) == 0)
					{
						::TextOutW(hdcMem, 0, 0, prgchPos, 1);
						prgchPos++;
						::SetTextColor(hdcMem, g_fg.m_cr[kclrText]);
						fInBlock = false;
						continue;
					}
				}
			}
			else
			{
				if (fInBlockP)
				{
					// See if we should close an inline section (i.e a string in double quotes).
					if (ch < 0xd800 || ch > 0xdbff || prgchPos >= prgchLim - 1)
						::TextOutW(hdcMem, 0, 0, prgchPos, 1);
					else
						::TextOutW(hdcMem, 0, 0, prgchPos++, 2);
					if (*prgchPos == chBlock)
					{
						fInBlockP = false;
						::SetTextColor(hdcMem, g_fg.m_cr[kclrText]);
					}
					prgchPos++;
					continue;
				}
				else if (cchBlockStart <= prgchLim - prgchMin &&
					wcsncmp(pcsi->m_wszBlockStart, prgchPos, cchBlockStart) == 0)
				{
					// See if we need to open a comment block.
					fInBlock = true;
					::SetTextColor(hdcMem, pcsi->m_crBlock);
				}
				else
				{
					if (iswspace(*prgchPos))
					{
						// Draw the space character.
						::TextOutA(hdcMem, 0, 0, (char *)&g_fg.m_chSpace, 1);
						prgchPos++;
						fStartOfWord = true;
						continue;
					}

					if (pcsi->FindBlockP(prgchPos, prgchLim - prgchPos, &cr, &chBlock))
					{
						// See if the rest of the line should be commented out.
						fInBlockP = true;
						cr = ::SetTextColor(hdcMem, cr);
						::TextOutW(hdcMem, 0, 0, prgchPos, 1);
						prgchPos++;
						continue;
					}

					if (fStartOfWord || strchr(pcsi->m_pszDelim, *prgchPos))
					{
						cch = 0;
						if (pcsi->FindWord(prgchPos, prgchLim - prgchPos, pcsi->m_pszDelim, &cr, &cch))
						{
							if (prgchLim - prgchPos == cch ||
								strchr(pcsi->m_pszDelim, (char)*prgchPos) ||
								strchr(pcsi->m_pszDelim, (char)*(prgchPos + cch)))
							{
								cr = ::SetTextColor(hdcMem, cr);
								::TextOutW(hdcMem, 0, 0, prgchPos, cch);
								::SetTextColor(hdcMem, cr);
								prgchPos += cch;
								continue;
							}
						}
						if (!fStartOfWord)
						{
							// This is a punctuation character, so print it out, then continue.
							if (ch < 0xd800 || ch > 0xdbff || prgchPos >= prgchLim - 1)
								::TextOutW(hdcMem, 0, 0, prgchPos, 1);
							else
								::TextOutW(hdcMem, 0, 0, prgchPos++, 2);
							prgchPos++;
							fStartOfWord = true;
							continue;
						}
						fStartOfWord = true;
					}
				}
			}
		}

		fStartOfWord = false;
		if (ch < 0xd800 || ch > 0xdbff || prgchPos >= prgchLim - 1)
			::TextOutW(hdcMem, 0, 0, prgchPos, 1);
		else
			::TextOutW(hdcMem, 0, 0, prgchPos++, 2);
		prgchPos++;
	}
}


// ZZZ
/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::DrawSelection(HDC hdcScreen, bool fEraseCursors)
{
	AssertPtr(m_pzd);
	if (m_pzd->GetAccessError())
		return;

	RECT rect;
	::GetClientRect(m_hwnd, &rect);
	::SelectClipRgn(hdcScreen, NULL); // Remove any previous clip region.

	HRGN hrgnSel = NULL;
	if (m_ichSelMin != m_ichSelLim)
	{
		POINT ptStart = PointFromChar(m_ichSelMin);
		POINT ptStop = PointFromChar(m_ichSelLim);
		POINT ptTemp;

		if (ptStop.y >= m_rcScreen.top && ptStart.y < rect.bottom)
		{
			int dypLine = g_fg.m_ri.m_tm.tmHeight;
			if (ptStop.y >= m_rcScreen.top && ptStart.y < rect.bottom)
			{
				// Hilight the selection that is visible in the memory DC
				if (ptStart.y < m_rcScreen.top)
				{
					ptStart.x = m_rcScreen.left;
					ptStart.y = m_rcScreen.top;
				}
				if (ptStop.y > rect.bottom)
					ptStop.y = rect.bottom - m_rcScreen.bottom;
				ptTemp = ptStart;

				// Draw the highlighted background
				int iLine = (ptStart.y - m_rcScreen.top) / dypLine;
				Assert((UINT)iLine < m_cli);
				int nWidth;
				if (ptTemp.y < ptStop.y)
				{
					// Add the end of the current line to the selection region.
					nWidth = m_rcScreen.left + m_prgli[iLine++].dxp - ptTemp.x;
					hrgnSel = ::CreateRectRgn(ptTemp.x, ptTemp.y, ptTemp.x + nWidth, ptTemp.y + dypLine);
					ptTemp.x = m_rcScreen.left;
					ptTemp.y += dypLine;
				}
				HRGN hrgn;
				while (ptTemp.y < ptStop.y)
				{
					// Add the entire current line to the selection region.
					Assert((UINT)iLine <= m_cli);
					nWidth = m_prgli[iLine++].dxp;
					hrgn = ::CreateRectRgn(ptTemp.x, ptTemp.y, ptTemp.x + nWidth, ptTemp.y + dypLine);
					if (hrgnSel)
					{
						::CombineRgn(hrgnSel, hrgnSel, hrgn, RGN_OR);
						::DeleteObject(hrgn);
					}
					else
					{
						hrgnSel = hrgn;
					}
					ptTemp.y += dypLine;
				}
				// Add the beginning of the final line to the selection region.
				// This is either from the beginning of the line (for a selection that
				// crosses multiple lines) or from the beginning of the selection (for
				// a selection that is only on one line).
				hrgn = ::CreateRectRgn(ptTemp.x, ptTemp.y, ptStop.x, ptTemp.y + dypLine);
				if (hrgnSel)
				{
					::CombineRgn(hrgnSel, hrgnSel, hrgn, RGN_OR);
					::DeleteObject(hrgn);
				}
				else
				{
					hrgnSel = hrgn;
				}
			}
		}
	}

	// We now have two HRGNs:
	//   m_hrgnSel contains the region of the previous selection, or NULL if there wasn't one.
	//   hrgnSel contains the region of the new selection.
	// We want to ignore the intersection of these two regions, then invert the rest of both regions.
	HRGN hrgnToInvert = NULL;
	if (!m_hrgnSel)
		hrgnToInvert = hrgnSel;
	else if (!hrgnSel)
		hrgnToInvert = m_hrgnSel;
	else
	{
		hrgnToInvert = ::CreateRectRgn(0, 0, 0, 0);
		::CombineRgn(hrgnToInvert, hrgnSel, m_hrgnSel, RGN_XOR);
	}
	if (fEraseCursors)
		g_fg.DrawColumnMarkers(hdcScreen, true, rect, m_rcScreen.left - g_fg.m_rcMargin.left);
	if (hrgnToInvert)
	{
		::InvertRgn(hdcScreen, hrgnToInvert);
		if (hrgnToInvert != hrgnSel)
		{
			// Only delete the region if it's not the new region we're going to save.
			::DeleteObject(hrgnToInvert);
		}
	}
	RECT rectMargin = rect;
	rectMargin.bottom = knWedgeHeight;
	::ExtTextOut(hdcScreen, 0, 0, ETO_OPAQUE, &rectMargin, NULL, 0, NULL);
	rectMargin.bottom = rect.bottom;
	rectMargin.top = rect.bottom - knWedgeHeight;
	::ExtTextOut(hdcScreen, 0, 0, ETO_OPAQUE, &rectMargin, NULL, 0, NULL);
	g_fg.DrawColumnMarkers(hdcScreen, false, rect, m_rcScreen.left - g_fg.m_rcMargin.left);

	// Keep track of the new selection region.
	if (m_hrgnSel)
		::DeleteObject(m_hrgnSel);
	m_hrgnSel = hrgnSel;
}


// ZZZ
/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::GetBlockValues(DocPart * pdp, UINT iprInPart, void * pv, HDC hdc,
	bool & fInBlock, bool & fInBlockP)
{
	static UINT iFirstChar = -1;
	static bool fInBlockOld = false;
	static bool fInBlockPOld = false;
	static COLORREF crOld = 0;

	if (m_ichTopLine == iFirstChar)
	{
		// Since the first character shown is the same as the last time this function was called,
		// we can return the cached values. This saves a lot of time when this function gets called
		// multiple times without scrolling the screen (i.e. during typing).
		fInBlock = fInBlockOld;
		fInBlockP = fInBlockPOld;
		::SetTextColor(hdc, crOld);
		return;
	}

	bool fAnsiCurrent = m_pzd->IsAnsi(pdp, iprInPart);

	// Calculate the correct value for fInBlock

	fInBlock = false;
	ColorSchemeInfo * pcsi = m_pzd->GetColorScheme();
	if (*pcsi->m_szBlockStart)
	{
		// Go back 1000 paragraphs or to the beginning of the file, whichever is closer.
		// If the block comment end is found, exit the loop and leave fInBlock as FALSE.
		// If the block comment start is found, exit the loop and set fInBlock to TRUE.
		// If we get back to 500 paragraphs, leave fInBlock as FALSE.
		UINT cprBack = 0;
		DocPart * pdpCur = pdp;
		UINT iprInPartCur = iprInPart;
		if (pv == pdpCur->rgpv[iprInPartCur])
		{
			if (iprInPartCur-- == 0)
			{
				pdpCur = pdpCur->pdpPrev;
				if (pdpCur)
					iprInPartCur = pdpCur->cpr - 1;
			}
			cprBack = 1;
		}
		if (pdpCur)
		{
			char * pBlockStartEnd = pcsi->m_szBlockStart + lstrlen(pcsi->m_szBlockStart) - 1;
			char * pBlockStopEnd = pcsi->m_szBlockStop + lstrlen(pcsi->m_szBlockStop) - 1;
			char * pBlockStartPos = pBlockStopEnd;
			char * pBlockStopPos = pBlockStopEnd;
			bool fContinue = true;
			while (fContinue)
			{
				if (m_pzd->IsAnsi(pdpCur, iprInPartCur))
				{
					char ch;
					char * pStart = (char *)pdpCur->rgpv[iprInPartCur];
					char * pPosCur;
					if (cprBack == 0)
						pPosCur = (char *)pv;
					else
						pPosCur = pStart + PrintCharsInPara(pdpCur, iprInPartCur) - 1;
					if (pPosCur > pStart)
					{
						while (pPosCur >= pStart)
						{
							if ((ch = *pPosCur--) != *pBlockStopPos)
								pBlockStopPos = pBlockStopEnd;
							if (ch == *pBlockStopPos)
							{
								if (pBlockStopPos-- == pcsi->m_szBlockStop)
								{
									// The end of a block was found, so exit both loops
									fContinue = false;
									break;
								}
							}
							if (ch != *pBlockStartPos)
								pBlockStartPos = pBlockStartEnd;
							if (ch == *pBlockStartPos)
							{
								if (pBlockStartPos-- == pcsi->m_szBlockStart)
								{
									// The start of a block was found, so exit both loops
									fContinue = false;
									fInBlock = true;
									break;
								}
							}
						}
					}
				}
				else
				{
					wchar ch;
					wchar * pStart = (wchar *)pdpCur->rgpv[iprInPartCur];
					wchar * pPosCur;
					if (cprBack == 0)
						pPosCur = (wchar *)pv;
					else
						pPosCur = pStart + PrintCharsInPara(pdpCur, iprInPartCur) - 1;
					if (pPosCur > pStart)
					{
						while (pPosCur >= pStart)
						{
							if ((ch = *pPosCur--) == (wchar)*pBlockStopPos)
							{
								if (pBlockStopPos-- == pcsi->m_szBlockStop)
								{
									// The end of a block was found, so exit both loops
									fContinue = false;
									break;
								}
							}
							else
							{
								pBlockStopPos = pBlockStopEnd;
							}
							if (ch == (wchar)*pBlockStartPos)
							{
								if (pBlockStartPos-- == pcsi->m_szBlockStart)
								{
									// The start of a block was found, so exit both loops
									fContinue = false;
									fInBlock = true;
									break;
								}
							}
							else
							{
								pBlockStartPos = pBlockStartEnd;
							}
						}
					}
				}
				if (++cprBack == 1000)
					break;
				if (iprInPartCur-- == 0)
				{
					pdpCur = pdpCur->pdpPrev;
					if (!pdpCur)
						break;
					iprInPartCur = pdpCur->cpr - 1;
				}
			}
		}
	}

	// Calculate the correct value for fInBlockP

	fInBlockP = false;
	COLORREF cr;
	if (pcsi->m_vpbp.size() > 0)
	{
		if (fAnsiCurrent)
		{
			char ch;
			char * pStop = (char *)pdp->rgpv[iprInPart] + PrintCharsInPara(pdp, iprInPart);
			for (char * pT = (char *)pdp->rgpv[iprInPart] - 1; ++pT < (char *)pv; )
			{
				if (fInBlockP)
				{
					if (*pT == ch)
						fInBlockP = false;
				}
				else
				{
					if (pcsi->FindBlockP(pT, pStop - pT, &cr, &ch))
						fInBlockP = true;
				}
			}
		}
		else
		{
			wchar wch;
			wchar * pStop = (wchar *)pdp->rgpv[iprInPart] + PrintCharsInPara(pdp, iprInPart);
			for (wchar * pT = (wchar *)pdp->rgpv[iprInPart] - 1; ++pT < (wchar *)pv; )
			{
				if (fInBlockP)
				{
					if (*pT == wch)
						fInBlockP = false;
				}
				else
				{
					if (pcsi->FindBlockP(pT, pStop - pT, &cr, &wch))
						fInBlockP = true;
				}
			}
		}
	}

	if (fInBlock)
		cr = pcsi->m_crBlock;
	else if (!fInBlockP)
		cr = g_fg.m_cr[kclrText];
	::SetTextColor(hdc, cr);

	fInBlockOld = fInBlock;
	fInBlockPOld = fInBlockP;
	crOld = cr;
	iFirstChar = m_ichTopLine;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
SelectionType CFastEdit::GetSelection(UINT * pichSelMin, UINT * pichSelLim)
{
	if (pichSelMin)
		*pichSelMin = m_ichSelMin;
	if (pichSelLim)
		*pichSelLim = m_ichSelLim;
	if (m_ichSelMin != m_ichDragStart)
		return kstSelAfter;
	if (m_ichSelMin != m_ichSelLim)
		return kstSelBefore;
	return kstNone;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFastEdit::SetSelection(UINT ichSelMin, UINT ichSelLim, bool fScroll, bool fForceRedraw)
{
	::HideCaret(m_hwnd);

	UINT cchTotal = m_pzd->GetCharCount();
	UINT cprTotal = m_pzd->GetParaCount();
	if (ichSelMin > cchTotal)
		ichSelMin = cchTotal;
	if (ichSelLim > cchTotal)
		ichSelLim = cchTotal;

	// Make sure ichSelMin and ichSelLim are valid positions (i.e. not between CR and LF).
	bool fNoSel = ichSelMin == ichSelLim;
	void * pv;
	UINT ipr;
	DocPart * pdp = m_pzd->GetPart(ichSelMin, false, &pv, &ipr);
	UINT cchOffset = m_pzd->CharOffset(pdp, ipr, pv);
	if (cchOffset > PrintCharsInPara(pdp, ipr))
	{
		// The Min position is past the printable characters on the line, so advance
		// it to the next line.
		ichSelMin += CharsInPara(pdp, ipr) - cchOffset;
	}
	if (fNoSel)
	{
		ichSelLim = ichSelMin;
	}
	else
	{
		// Repeat the test for the Lim position.
		pdp = m_pzd->GetPart(ichSelLim, false, &pv, &ipr);
		cchOffset = m_pzd->CharOffset(pdp, ipr, pv);
		if (cchOffset > PrintCharsInPara(pdp, ipr))
		{
			// The Lim position is past the printable characters on the line, so advance
			// it to the next line.
			ichSelLim += CharsInPara(pdp, ipr) - cchOffset;
		}
	}

	if (ichSelMin <= ichSelLim)
	{
		m_ichSelMin = ichSelMin;
		m_ichSelLim = ichSelLim;
	}
	else
	{
		m_ichSelMin = ichSelLim;
		m_ichSelLim = ichSelMin;
	}
	m_ichDragStart = ichSelMin;

	bool fRedraw = true;
	UINT ichTopLine = m_ichTopLine;
	if (fScroll)
	{
		// Find the values for the top line we need to draw.
		UINT ich = (m_ichDragStart == m_ichSelMin) ? m_ichSelLim : m_ichSelMin;
		UINT ipr = m_pzd->ParaFromChar(ich);
		UINT iprTopLine = m_iprTopLine;
		// There are 3 cases where we need to scroll:
		// 1) The character is above the current view
		// 2) The character is below the current view
		// 3) There is extra white-space at the bottom of the view
		//if (ich < m_ichTopLine || ich >= m_prgli[m_cli].ich || m_prgli[m_cli].ich == -1)
		// 2007-03-26: Changed this line to make the selection work at the bottom of the file.
		if (ich < m_ichTopLine || (ich >= m_prgli[m_cli].ich && ich < cchTotal) || m_prgli[m_cli].ich == -1)
		{
			// The requested character is either above the current screen, below the
			// current screen, or on the last partially visible line (m_cli - 1),
			// so center it on the screen.
			ichTopLine = min(ShowCharAtLine(ipr, ich, m_cli / 2), m_ichMaxTopLine);
			m_iprTopLine = m_pzd->ParaFromChar(ichTopLine);
		}

		if (!m_fWrap)
		{
			// See if we need to scroll horizontally.
			// First make sure the start of the selection is visible.
			// Then make sure the end of the selection is visible (even if this causes the
			// start of the selection to disappear.
			RECT rc;
			::GetClientRect(m_hwnd, &rc);
			int dxpClient = rc.right - rc.left;

			int dxpOffsetMin = 0; // A positive value means move the view right (scroll left).
			POINT ptMin = PointFromChar(m_ichSelMin);
			if (ptMin.x < g_fg.m_rcMargin.left)
				dxpOffsetMin = g_fg.m_rcMargin.left - ptMin.x;// + (int)(dxpClient * 0.25);
			else if (ptMin.x > rc.right - g_fg.m_rcMargin.right)
				dxpOffsetMin = (rc.right - g_fg.m_rcMargin.right) - ptMin.x;// - (int)(dxpClient * 0.25);
			if (dxpOffsetMin)
				::OffsetRect(&m_rcScreen, dxpOffsetMin, 0);

			int dxpOffsetLim = 0; // A positive value means move the view right (scroll left).
			POINT ptLim = PointFromChar(m_ichSelLim);
			if (ptLim.x < g_fg.m_rcMargin.left)
				dxpOffsetLim = g_fg.m_rcMargin.left - ptLim.x;// + (int)(dxpClient * 0.25);
			else if (ptLim.x > rc.right - g_fg.m_rcMargin.right)
				dxpOffsetLim = (rc.right - g_fg.m_rcMargin.right) - ptLim.x;// - (int)(dxpClient * 0.25);
			if (dxpOffsetLim)
				::OffsetRect(&m_rcScreen, dxpOffsetLim, 0);

			if (dxpOffsetMin || dxpOffsetLim)
			{
				fForceRedraw = true;
				if (m_rcScreen.left > g_fg.m_rcMargin.left)
				{
					// Make sure we didn't scroll too far horizontally to the left.
					::OffsetRect(&m_rcScreen, g_fg.m_rcMargin.left - m_rcScreen.left, 0);
				}
				SCROLLINFO si = { sizeof(si), SIF_POS };
				si.nPos = g_fg.m_rcMargin.left - m_rcScreen.left;
				m_pzf->SetScrollInfo(this, SB_HORZ, &si);
			}
		}
	}

	if (m_ichTopLine != ichTopLine || fForceRedraw)
	{
		// We need to redraw the entire screen.
		m_ichTopLine = ichTopLine;
		m_ichMaxTopLine = ShowCharAtLine(cprTotal, cchTotal, m_cli);

		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_PAGE | SIF_POS | SIF_RANGE };
		si.nPos = m_ichTopLine;
		si.nPage = cchTotal - m_ichMaxTopLine;
		si.nMax = m_ichMaxTopLine ? cchTotal - 1 : 0;
		m_pzf->SetScrollInfo(this, SB_VERT, &si);

		::InvalidateRect(m_hwnd, NULL, FALSE);
		::UpdateWindow(m_hwnd);
	}
	else
	{
		// The selection is contained on the screen.
		HDC hdc = ::GetDC(m_hwnd);
		DrawSelection(hdc, true);
		::ReleaseDC(m_hwnd, hdc);
	}

	// Position the caret and show it.
	m_ptRealPos = m_ptCaretPos = PointFromChar(ichSelLim);
	::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	::ShowCaret(m_hwnd);

	return true;
}


/*----------------------------------------------------------------------------------------------
	This function returns the character position for the top line if we want to show
	the specified character (ich) at the specified line (iLine) on the screen.
----------------------------------------------------------------------------------------------*/
UINT CFastEdit::ShowCharAtLine(UINT ipr, UINT ich, int iLine)
{
	AssertPtr(m_pzd);
	if (m_pzd->GetAccessError() || !m_pzd->GetCharCount())
		return 0;

	// Find out the line offset from the beginning of the para
	if (iLine < 0)
		return m_ichTopLine;
	if (iLine == 0)
		return ich;

	// This part is looking at the requested paragraph index and subtracting the
	// number of lines we want to show above it.
	void * pv;
	UINT iprInPart;
	UINT cchBefore;
	UINT iprTopLine = (ipr >= (UINT)iLine) ? ipr - iLine : 0;
	DocPart * pdp = m_pzd->GetPart(iprTopLine, true, &pv, &iprInPart, &cchBefore);
	for (UINT ipr = 0; ipr < iprInPart; ipr++)
		cchBefore += CharsInPara(pdp, ipr);
	if (!m_fWrap)
	{
		// Since each paragraph is 1 and only 1 line, we're done.
		return cchBefore;
	}

	// Now we need to calculate each of the paragraphs to see how many lines it
	// will take up.
	UINT * prgichPos = new UINT[m_cli + 1];
	if (!prgichPos)
		return 0;
	ZSmartPtr zsp(prgichPos); // This will delete prgichPos when we return.

	void * pvParaMin = pdp->rgpv[iprInPart];
	void * pvParaLim = m_pzd->LastPrintCharInPara(pdp, iprInPart);
	UINT cchInPara;
	int cLines = 0;
	RECT rc;
	::GetClientRect(m_hwnd, &rc);
	RECT rcBounds;
	::SetRect(&rcBounds, m_rcScreen.left, m_rcScreen.top,
		rc.right - m_rcScreen.right, rc.bottom - m_rcScreen.bottom);
	int dxpLine;
	bool fDummy;
	HDC hdc = ::GetDC(m_hwnd);
	do
	{
		dxpLine = rcBounds.right - rcBounds.left;
		prgichPos[cLines] = cchBefore;
		if (m_pzd->IsAnsi(pdp, iprInPart))
		{
			cchInPara = DrawLineA(hdc, (char *)pvParaMin, (char *)pv, (char *)pvParaLim,
				&dxpLine, m_rcScreen.left, g_fg.m_ri, m_fWrap, false, false, fDummy, fDummy);
			pv = (char *)pv + cchInPara;
		}
		else
		{
			cchInPara = DrawLineW(hdc, (wchar *)pvParaMin, (wchar *)pv, (wchar *)pvParaLim,
				&dxpLine, m_rcScreen.left, g_fg.m_ri, m_fWrap, false, false, fDummy, fDummy);
			pv = (wchar *)pv + cchInPara;
		}
		if (pv >= pvParaLim)
		{
			cchInPara += CharsAtEnd(pdp, iprInPart);
			if (++iprInPart >= pdp->cpr)
			{
				pdp = pdp->pdpNext;
				iprInPart = 0;
				if (!pdp)
					break;
			}
			pv = pdp->rgpv[iprInPart];
			pvParaMin = pv;
			pvParaLim = m_pzd->LastPrintCharInPara(pdp, iprInPart);
		}
		cchBefore += cchInPara;
		if (cchBefore < ich)
		{
			if (++cLines > iLine)
			{
				// Move all the values up one position in the array
				cLines--;
				memmove(prgichPos, prgichPos + 1, sizeof(UINT) * iLine);
			}
		}
	} while (cchBefore < ich);

	::ReleaseDC(m_hwnd, hdc);
	UINT ichTopLine = prgichPos[0];
	return ichTopLine;
}


// ZZZ
/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::ShowText(HDC hdcScreen, RECT & rect)
{
	if (m_hrgnSel)
	{
		::DeleteObject(m_hrgnSel);
		m_hrgnSel = NULL;
	}

	if (!m_pzd || m_pzd->GetAccessError() || !m_pzd->GetCharCount())
	{
		::GetClientRect(m_hwnd, &rect);
		::SetBkColor(hdcScreen, g_fg.m_cr[kclrWindow]);
		::ExtTextOut(hdcScreen, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
		g_fg.DrawColumnMarkers(hdcScreen, false, rect, m_rcScreen.left - g_fg.m_rcMargin.left);
		m_prgli[0].ich = 0;
		for (UINT i = 1; i <= m_cli; i++)
			m_prgli[i].ich = -1;
		return;
	}

	UINT iprInPart = 0;
	ParaDrawInfo pdi;
	pdi.ri.Initialize(hdcScreen, g_fg.m_ri.m_cchSpacesInTab);
	DocPart * pdp = m_pzd->GetPart(m_ichTopLine, false, &pdi.pvStart, &iprInPart);
	::MoveToEx(hdcScreen, rect.left, rect.top, NULL);
	pdi.ich = m_ichTopLine;
	pdi.ipr = m_iprTopLine;
	pdi.prgli = m_prgli;
	pdi.prgli[0].ipr = m_iprTopLine;
	pdi.prgli[0].ich = m_ichTopLine;
	pdi.hdcMem = hdcScreen;
	pdi.cli = 0;
	pdi.cLineLimit = m_cli;
	pdi.dypLine = pdi.ri.m_tm.tmHeight;
	pdi.pdp = pdp;
	pdi.iprInPart = iprInPart;
	pdi.rcBounds = rect;
	pdi.rcScreen = m_rcScreen;
	pdi.fWrap = m_fWrap;
	pdi.fShowColors = true;
	GetBlockValues(pdp, iprInPart, pdi.pvStart, hdcScreen, pdi.fInBlock, pdi.fInBlockP);
	pdi.prgli[0].nSyntax = ::GetTextColor(pdi.hdcMem) |
		BlockToSyntax(pdi.fInBlock) | BlockPToSyntax(pdi.fInBlockP);

	int nOldROP = ::SetROP2(hdcScreen, R2_WHITE);

	FileType ft = m_pzd->GetFileType();
	if (ft == kftAnsi)
	{
		while (DrawPara(&pdi, true) && pdi.cli <= pdi.cLineLimit)
		{
		}
	}
	else
	{
		while (DrawPara(&pdi, ft == kftUnicode8 && !IsParaInMem(pdi.pdp, pdi.iprInPart)) &&
			pdi.cLineLimit <= pdi.cLineLimit)
		{
		}
	}

	::SetROP2(hdcScreen, nOldROP);

	// Blank out any white space at the bottom of the window.
	POINT pt;
	::MoveToEx(hdcScreen, 0, 0, &pt);
	RECT rcBottom = { rect.left, pt.y, rect.right, rect.bottom };
	::SetBkColor(hdcScreen, g_fg.m_cr[kclrWindow]);
	::ExtTextOut(hdcScreen, 0, 0, ETO_OPAQUE, &rcBottom, NULL, 0, NULL);

	if ((UINT)pdi.cli < m_cli)
		pdi.prgli[pdi.cli].dxp = 0;
	UINT iLastPara = pdi.prgli[pdi.cli].ipr + 1;
	for (UINT i = pdi.cli + 1; i <= m_cli; i++)
	{
		pdi.prgli[i].ipr = iLastPara;
		pdi.prgli[i].dxp = 0;
		//pdi.prgli[i].ich = pdi.prgli[pdi.cli].ich + 1;
		pdi.prgli[i].ich = -1;
		pdi.prgli[i].nSyntax = 0;
	}

	DrawSelection(hdcScreen, false);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFastEdit::Print(PrintInfo * ppi)
{
	AssertPtr(m_pzd);
	AssertPtr(ppi);
	if (!ppi->hdc || !ppi->pszFooter || !ppi->pszHeader || ppi->ichMin >= ppi->ichStop)
		return false;

	// Get some screen dimensions
	HDC hdcScreen = ::GetDC(m_hwnd);
	int cScreenLogPixelsX = ::GetDeviceCaps(hdcScreen, LOGPIXELSX);
	int cScreenLogPixelsY = ::GetDeviceCaps(hdcScreen, LOGPIXELSY);
	::ReleaseDC(m_hwnd, hdcScreen);

	// Get some printer dimensions
	HDC hdcPrinter = ppi->hdc;
	int cPrinterLogPixelsX = ::GetDeviceCaps(hdcPrinter, LOGPIXELSX);
	int cPrinterLogPixelsY = ::GetDeviceCaps(hdcPrinter, LOGPIXELSY);

	int nOffsetX = ::GetDeviceCaps(hdcPrinter, PHYSICALOFFSETX);
	int nOffsetY = ::GetDeviceCaps(hdcPrinter, PHYSICALOFFSETY);
	int nPageWidth = ::GetDeviceCaps(hdcPrinter, PHYSICALWIDTH);
	int nPageHeight = ::GetDeviceCaps(hdcPrinter, PHYSICALHEIGHT);
	RECT rcTwipMargin = {
		::MulDiv(ppi->rcMargin.left, cPrinterLogPixelsX, 1440),
		::MulDiv(ppi->rcMargin.top, cPrinterLogPixelsY, 1440),
		::MulDiv(ppi->rcMargin.right, cPrinterLogPixelsX, 1440),
		::MulDiv(ppi->rcMargin.bottom, cPrinterLogPixelsY, 1440)};

	RECT rcPrint = { rcTwipMargin.left, rcTwipMargin.top,
		nPageWidth - rcTwipMargin.right, nPageHeight - rcTwipMargin.bottom };
	::OffsetRect(&rcPrint, -nOffsetX, -nOffsetY);

	// Header and footer positions
	int nhfLeft = rcPrint.left;
	int nhfRight = rcPrint.right;
	int nhfCenter = nhfLeft + ((nhfRight - nhfLeft) / 2);
	int nhfTop = ::MulDiv(ppi->pthf.x, cPrinterLogPixelsX, 1440) - nOffsetY;
	int nhfBottom = nPageHeight - ::MulDiv(ppi->pthf.y, cPrinterLogPixelsX, 1440) - nOffsetY;

	// Create a font with the same dimensions as the screen for the printer
	LOGFONT lf;
	::GetObject(g_fg.m_hFont, sizeof(LOGFONT), &lf);
	lf.lfHeight = ::MulDiv(lf.lfHeight, cPrinterLogPixelsY, cScreenLogPixelsY);
	HFONT hNewFont = ::CreateFontIndirect(&lf);
	HFONT hOldFont = (HFONT)::SelectObject(hdcPrinter, hNewFont);

	ParaDrawInfo pdi;
	pdi.ri.Initialize(hdcPrinter, g_fg.m_ri.m_cchSpacesInTab);
	TEXTMETRIC & tm = pdi.ri.m_tm;

	int cLines = (rcPrint.bottom - rcPrint.top) / tm.tmHeight - 1;
	// This isn't used for anything, but DrawPara requires it.
	pdi.prgli = new LineInfo[cLines + 1];
	if (!pdi.prgli)
	{
		::DeleteObject(::SelectObject(hdcPrinter, hOldFont)); // hNewFont
		return false;
	}
	pdi.hdcMem = hdcPrinter;
	pdi.cLineLimit = cLines;
	pdi.dypLine = tm.tmHeight;
	pdi.rcBounds = rcPrint;
	pdi.rcScreen = rcPrint;
	pdi.fWrap = true;
	pdi.fShowColors = pdi.fInBlock = pdi.fInBlockP = false;

	// These will hold the header (left, center, right) and footer (left, center, right) strings
	char szHeader[3][100];
	char szFooter[3][100];
	char szTime[50];
	char szDate[50];
	SYSTEMTIME sysTime;
	::GetLocalTime(&sysTime);
	sprintf_s(szTime, "%02d:%02d:%02d", sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	sprintf_s(szDate, "%02d/%02d/%04d", sysTime.wMonth, sysTime.wDay, sysTime.wYear);

	int cColCopy = ppi->fCollate ? ppi->cCopy : 1;
	int cNonColCopy = ppi->fCollate ? 1 : ppi->cCopy;
	char szProgress[10];
	char szFilename[MAX_PATH];
	char * pPos = strrchr(m_pzd->GetFilename(), '\\');
	lstrcpy(szFilename, pPos ? pPos + 1 : m_pzd->GetFilename());

	HeaderFooterInfo hfi;
	hfi.pszDate = szDate;
	hfi.pszFilename = szFilename;
	hfi.pszPathname = m_pzd->GetFilename();
	hfi.pszTime = szTime;

	DOCINFO di = { sizeof(di) };
	di.lpszDocName = m_pzd->GetFilename(); // The whole filename (including the path)
	di.lpszOutput = NULL;
	if (!::StartDoc(hdcPrinter, &di))
		return false;

	bool fSuccess = true;
	FileType ft = m_pzd->GetFileType();
	UINT ichStop = ppi->ichStop;
	for (int iColCopy = 0; iColCopy < cColCopy; iColCopy++)
	{
		ppi->iPage = 0;
		pdi.pdp = m_pzd->GetPart(ppi->ichMin, false, &pdi.pvStart, &pdi.iprInPart);
		pdi.ich = ppi->ichMin;
		pdi.ipr = m_pzd->ParaFromChar(pdi.ich);

		do
		{
			UINT ich = pdi.ich;
			UINT ipr = pdi.ipr;
			UINT iprInPart = pdi.iprInPart;
			DocPart * pdp = pdi.pdp;
			void * pvStart = pdi.pvStart;

			// Update the print dialog
			hfi.iPage = ++ppi->iPage;
			sprintf_s(szProgress, "Page: %d", ppi->iPage);
			::SetDlgItemText(ppi->hdlgAbort, IDC_PRINTPAGE, szProgress);

			m_pzd->ParseHFString(ppi->pszHeader, szHeader, &hfi);
			m_pzd->ParseHFString(ppi->pszFooter, szFooter, &hfi);

			for (int iNonColCopy = 0; iNonColCopy < cNonColCopy; iNonColCopy++)
			{
				if (::StartPage(hdcPrinter) < 0)
				{
					fSuccess = false;
					break;
				}

				::SelectObject(hdcPrinter, hNewFont);
				::SetTextAlign(hdcPrinter, TA_TOP | TA_LEFT | TA_UPDATECP);

				// Set the state back to the beginning of the page
				pdi.ich = ich;
				pdi.ipr = ipr;
				pdi.iprInPart = iprInPart;
				pdi.pdp = pdp;
				pdi.pvStart = pvStart;
				pdi.cli = 0;

				// Draw the text for this page
				::MoveToEx(hdcPrinter, rcPrint.left, rcPrint.top, NULL);
				if (ft == kftAnsi)
				{
					while (DrawPara(&pdi, true, ichStop) &&
						pdi.cli <= pdi.cLineLimit)
					{
						;
					}
				}
				else
				{
					while (DrawPara(&pdi, m_pzd->IsAnsi(pdi.pdp, pdi.iprInPart), ichStop) &&
						pdi.cLineLimit <= pdi.cLineLimit)
					{
						;
					}
				}

				// Draw the header for this page
				::SetTextAlign(hdcPrinter, TA_TOP | TA_LEFT);
				::TextOut(hdcPrinter, nhfLeft, nhfTop, szHeader[0], strlen(szHeader[0]));
				::SetTextAlign(hdcPrinter, TA_TOP | TA_CENTER);
				::TextOut(hdcPrinter, nhfCenter, nhfTop, szHeader[1], strlen(szHeader[1]));
				::SetTextAlign(hdcPrinter, TA_TOP | TA_RIGHT);
				::TextOut(hdcPrinter, nhfRight, nhfTop, szHeader[2], strlen(szHeader[2]));

				// Draw the footer for this page
				::SetTextAlign(hdcPrinter, TA_BOTTOM | TA_LEFT);
				::TextOut(hdcPrinter, nhfLeft, nhfBottom, szFooter[0], strlen(szFooter[0]));
				::SetTextAlign(hdcPrinter, TA_BOTTOM | TA_CENTER);
				::TextOut(hdcPrinter, nhfCenter, nhfBottom, szFooter[1], strlen(szFooter[1]));
				::SetTextAlign(hdcPrinter, TA_BOTTOM | TA_RIGHT);
				::TextOut(hdcPrinter, nhfRight, nhfBottom, szFooter[2], strlen(szFooter[2]));

				if (::EndPage(hdcPrinter) < 0)
				{
					fSuccess = FALSE;
					break;
				}
				if (ppi->fCancel)
					break;
			}
			if (ppi->fCancel | !fSuccess)
				break;
		}
		while (pdi.pdp && pdi.ich < ichStop);
		if (ppi->fCancel | !fSuccess)
			break;
	}

	if (fSuccess)
		::EndDoc(hdcPrinter);
	::SelectObject(hdcPrinter, hOldFont);
	::DeleteObject(hNewFont);
	delete [] pdi.prgli;
	return fSuccess && !ppi->fCancel;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFastEdit::RecalculateLines(int dyp)
{
	Assert(m_pzd);

	int dypFont = g_fg.m_ri.m_tm.tmHeight;
	int cLines = 0;
	if (dypFont)
		cLines = (dyp - g_fg.m_rcMargin.top - g_fg.m_rcMargin.bottom) / dypFont;
	LineInfo * prgli = new LineInfo[cLines + 2];
	if (!prgli)
		return false;
	if (m_prgli)
		delete [] m_prgli;
	m_prgli = prgli;
	m_cli = cLines;
	memset(m_prgli, 0, sizeof(LineInfo) * (cLines + 2));
	for (UINT ili = 0; ili < m_cli + 2; ili++)
		m_prgli[ili].ich = -1;
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::InitializeDC(HDC hdc)
{
	Assert(hdc);
	::SelectObject(hdc, g_fg.m_hFont);
	::SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_UPDATECP);
	::SetBkColor(hdc, 0xFFFFFF);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CFastEdit::CharFromPoint(POINT point, bool fUpdateCaretPos)
{
	AssertPtr(m_pzd);
	if (!m_hwnd || m_pzd->GetAccessError())
		return 0;

	POINT ptNew = m_ptCaretPos;
	RECT rect;
	::GetClientRect(m_hwnd, &rect);
	point.x = min(rect.right - m_rcScreen.right, max(m_rcScreen.left, point.x));
	point.y = min(rect.bottom - m_rcScreen.bottom, max(m_rcScreen.top, point.y)) - m_rcScreen.top;

	// Find the number of lines from the top of the client area
	int iLine;
	UINT cchTotal = m_pzd->GetCharCount();
	if (cchTotal == 0)
	{
		// TODO: Is this separate step necessary or will the lower case work?
		iLine = 0;
	}
	else
	{
		iLine = point.y / g_fg.m_ri.m_tm.tmHeight;
		Assert((UINT)iLine <= m_cli);
		//if (nLine > m_dwTotalParas - 1 || m_dwLinePositions[nLine].ich > m_dwFileLength)
		if (m_prgli[iLine].ich > cchTotal)
		{
			if (fUpdateCaretPos)
				m_ptCaretPos = PointFromChar(cchTotal);
			return cchTotal;
		}
	}
	ptNew.y = iLine * g_fg.m_ri.m_tm.tmHeight + m_rcScreen.top;

	if (point.x >= m_rcScreen.left + (int)m_prgli[iLine].dxp -
		g_fg.m_ri.GetCharWidth(g_fg.m_chPara))
	{
		// The mouse was clicked at the end or to the right of the end of the line.
		ptNew.x = m_rcScreen.left + m_prgli[iLine].dxp;
		UINT ich = m_prgli[iLine].ich;
		UINT cchLine = m_prgli[iLine + 1].ich - ich;
		UINT ipr;
		void * pv;
		DocPart * pdp = m_pzd->GetPart(ich, false, &pv, &ipr);
		UINT cchPara;
		if (m_pzd->IsAnsi(pdp, ipr))
			cchPara = CharsInPara(pdp, ipr) - ((char *)pv - (char *)pdp->rgpv[ipr]);
		else
			cchPara = CharsInPara(pdp, ipr) - ((wchar *)pv - (wchar *)pdp->rgpv[ipr]);
		// If the paragraph wraps to the next line, add the length of the line minus the
		// final space. Otherwise, move the cursor to the end of the paragraph except for
		// the non-printing end-of-line characters.
		// TODO: Does this incorrectly put the cursor one from the end of a line for a line
		// that wraps at a non-space?
		if (cchLine < cchPara)
			ich += cchLine - 1;
		else
			ich += cchPara - CharsAtEnd(pdp, ipr);
		if (fUpdateCaretPos)
			m_ptCaretPos = PointFromChar(ich);
		return ich;
	}

	UINT cchBefore;
	cchBefore = m_prgli[iLine].ich;
	//if (cchBefore == -1 || !(pdp = m_pzd->GetPart(cchBefore, false, &pv, &iprInPart)))
	if (cchBefore >= cchTotal)
	{
		if (fUpdateCaretPos)
			m_ptCaretPos = PointFromChar(cchTotal);
		return cchTotal;
	}

	void * pv;
	UINT iprInPart;
	DocPart * pdp = m_pzd->GetPart(cchBefore, false, &pv, &iprInPart);
	/*int nLength = 0;
	ptNew.x = -1;*/
	ptNew.x = 0;
	int nWidth = 0;
	/*int dxpLine = m_rcScreen.left + m_prgli[nLine].dxp;
	while (nWidth < point.x && nWidth != ptNew.x && nWidth < dxpLine)
	{
		ptNew.x = nWidth;
		nWidth = m_rcScreen.left + m_pzd->GetTabbedTextWidth(m_hdc, pv, ++nLength, m_pzd->IsAnsi(pdp, iprInPart));
	}*/

	int nTextWidth = 0;
	int dxpTab = g_fg.m_ri.m_dxpTab;
	if (m_pzd->IsAnsi(pdp, iprInPart))
	{
		char * prgch = (char *)pv;
#ifdef _DEBUG
		char * prgchLim = (char *)pdp->rgpv[iprInPart] + PrintCharsInPara(pdp, iprInPart);
#endif
		while (nWidth + m_rcScreen.left < point.x)
		{
			// prgch should never run off the end of the paragraph.
			// That case should be caught above.
			Assert(prgch < prgchLim);
			ptNew.x = nWidth;
			nWidth = g_fg.m_ri.AddCharWidth(nWidth, prgch);
			prgch++;
		}
		cchBefore += prgch - (char *)pv;
	}
	else
	{
		wchar * prgch = (wchar *)pv;
		wchar * prgchLim = (wchar *)pdp->rgpv[iprInPart] + PrintCharsInPara(pdp, iprInPart);
		HDC hdc = ::GetDC(m_hwnd);
		HFONT hfontOld = (HFONT)::SelectObject(hdc, g_fg.m_hFont);
		while (nWidth + m_rcScreen.left < point.x)
		{
			// prgch should never run off the end of the paragraph.
			// That case should be caught above.
			Assert(prgch < prgchLim);
			ptNew.x = nWidth;
			nWidth = g_fg.m_ri.AddCharWidth(nWidth, prgch, prgchLim, hdc);
			prgch++;
		}
		::SelectObject(hdc, hfontOld);
		::ReleaseDC(m_hwnd, hdc);
		cchBefore += prgch - (wchar *)pv;
	}
	// Advance to the next character if needed, or correct the character count.
	if (nWidth + m_rcScreen.left - point.x < point.x - ptNew.x)
		ptNew.x = nWidth;
	else
	{
		Assert(cchBefore > 0);
		cchBefore--;
	}
	ptNew.x += m_rcScreen.left;
	/*if (nLength > 0 && nWidth == ptNew.x)
		nLength--;
	if (nLength > 0)
	{
		if (nWidth - point.x < point.x - ptNew.x)
			ptNew.x = nWidth;
		else
			nLength--;
	}
	else
		ptNew.x = m_rcScreen.left;
	cchBefore += nLength;*/
	if (fUpdateCaretPos)
		m_ptCaretPos = ptNew;
	Assert(cchBefore <= cchTotal);
	return cchBefore;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
POINT CFastEdit::PointFromChar(UINT ich)
{
	AssertPtr(m_pzd);
	POINT point = { m_rcScreen.left, m_rcScreen.top };

	UINT cchTotal = m_pzd->GetCharCount();
	if (cchTotal == 0 || m_pzd->GetAccessError() || !m_prgli)
		return point;

	int iLine = 0;
	UINT ichFirst = ich;
	bool fOnScreen = false;
	if (ich > cchTotal)
		ich = cchTotal;
	if (ich < m_ichTopLine)
	{
		// The requested character is above the current view.
		point.y = -(g_fg.m_ri.m_tm.tmHeight * 2);
	}
	//else if (ich >= m_prgli[m_cli].ich) // 2007-03-26: Keep the caret from disappearing at the end of the file.
	else if (ich > m_prgli[m_cli].ich || (ich == m_prgli[m_cli].ich && ich < cchTotal))
	{
		// The requested character is below the current view.
		RECT rect;
		GetClientRect(m_hwnd, &rect);
		point.y = rect.bottom + (g_fg.m_ri.m_tm.tmHeight * 2);
	}
	else if (m_prgli[0].ich <= ich)
	{
		// The requested character is currently somewhere on the screen.
		fOnScreen = true;
		Assert(m_prgli[0].ich <= ich);
		while (m_prgli[++iLine].ich <= ich)
		{
			Assert((UINT)iLine < m_cli || ich == cchTotal);
		}
		// 2004-08-20 - This next line was put in to keep there from
		// being problems with a selection at the end of the file.
		if (m_prgli[iLine].ich >= cchTotal && m_prgli[iLine].ich != -1 &&
			ich == cchTotal && m_prgli[iLine].dxp == 0)
		{
			ichFirst = m_prgli[iLine].ich;
		}
		else
		{
			ichFirst = m_prgli[--iLine].ich;
		}
		point.y += (iLine * g_fg.m_ri.m_tm.tmHeight);
	}

	void * pv;
	UINT iprInPart;
	DocPart * pdp = m_pzd->GetPart(ichFirst, false, &pv, &iprInPart);
	if (!fOnScreen)
	{
		// Move ichFirst back to the beginning of the paragraph that contains ich.
		if (m_pzd->IsAnsi(pdp, iprInPart))
			ichFirst -= ((char *)pv - (char *)pdp->rgpv[iprInPart]);
		else
			ichFirst -= ((wchar *)pv - (wchar *)pdp->rgpv[iprInPart]);
		pv = pdp->rgpv[iprInPart];
	}
	UINT cch = ich - ichFirst;
	Assert(cch < CharsInPara(pdp, iprInPart) || ich == cchTotal);
	UINT cchInPara = PrintCharsInPara(pdp, iprInPart);
	if (ich == cchTotal && cchInPara != 0 && iLine > 0)
	{
		point.y -= g_fg.m_ri.m_tm.tmHeight;
		cch = ich - m_prgli[iLine - 1].ich;
		if (m_pzd->IsAnsi(pdp, iprInPart))
			pv = (char *)pv - cch;
		else
			pv = (wchar *)pv - cch;
		Assert(pv >= pdp->rgpv[iprInPart]);
	}
	cch = min(cch, cchInPara);

	int nWidth = 0;
	int dxpTab = g_fg.m_ri.m_dxpTab;
	if (m_pzd->IsAnsi(pdp, iprInPart))
	{
		AssertArray((char *)pv, cch);
		char * prgch = (char *)pv - 1;
		char * prgchLim = (char *)pv + cch;
		while (++prgch < prgchLim)
			nWidth = g_fg.m_ri.AddCharWidth(nWidth, prgch);
	}
	else
	{
		AssertArray((wchar *)pv, cch);
		wchar * prgch = (wchar *)pv - 1;
		wchar * prgchLim = (wchar *)pv + cch;
		wchar * prgchParaLim = (wchar *)pdp->rgpv[iprInPart] + cchInPara;
		HDC hdc = ::GetDC(m_hwnd);
		HFONT hfontOld = (HFONT)::SelectObject(hdc, g_fg.m_hFont);
		while (++prgch < prgchLim)
			nWidth = g_fg.m_ri.AddCharWidth(nWidth, prgch, prgchParaLim, hdc);
		::SelectObject(hdc, hfontOld);
		::ReleaseDC(m_hwnd, hdc);
	}
	point.x += nWidth;
	return point;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::SetWrap(bool fWrap)
{
	AssertPtr(m_pzd);
	m_fWrap = fWrap;
	SCROLLINFO si = {sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE};
	UINT cTotalChars = m_pzd->GetCharCount();
	UINT cTotalParas = m_pzd->GetParaCount();

	m_ichMaxTopLine = ShowCharAtLine(cTotalParas, cTotalChars, m_cli - 2);
	si.nPage = cTotalChars - m_ichMaxTopLine;
	si.nMax = m_ichMaxTopLine ? cTotalChars - 1 : 0;
	m_pzf->SetScrollInfo(this, SB_VERT, &si);

	if (fWrap)
	{
		/*m_ichMaxTopLine = ShowCharAtLine(cTotalParas, cTotalChars, m_cli - 2);
		si.nPage = cTotalChars - m_ichMaxTopLine;
		si.nMax = m_ichMaxTopLine ? cTotalChars - 1 : 0;
		m_pzf->SetScrollInfo(this, SB_VERT, &si);*/

		si.nMin = si.nMax = si.nPage = 0;
		OffsetRect(&m_rcScreen, g_fg.m_rcMargin.left - m_rcScreen.left, 0);
		m_pzf->SetScrollInfo(this, SB_HORZ, &si);
	}
	else
	{
		// TODO
		/*si.nPage = m_cli;
		si.nMax = m_dwTotalParas - 1 + si.nPage;
		m_pzf->SetScrollInfo(this, SB_VERT, &si);

		RECT rect;
		GetClientRect(m_hwnd, &rect);
		si.nPage = rect.right;
		si.nMax = m_dwCharsInLongestLine * g_fg.m_tm.tmMaxCharWidth + si.nPage;
		m_pzf->SetScrollInfo(this, SB_HORZ, &si);*/
	}
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
	m_ptRealPos = m_ptCaretPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::UpdateScrollBars()
{
	AssertPtr(m_pzd);

	// Update scrollbars
	SCROLLINFO si = {sizeof(si), SIF_PAGE | SIF_RANGE};
	if (m_fWrap)
	{
		UINT cch = m_pzd->GetCharCount();
		m_ichMaxTopLine = ShowCharAtLine(m_pzd->GetParaCount(), cch, m_cli - 2);
		si.nPage = cch - m_ichMaxTopLine;
		si.nMax = m_ichMaxTopLine ? cch - 1 : 0;
		m_pzf->SetScrollInfo(this, SB_VERT, &si);
	}
	else
	{
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
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::NotifyDelete(UINT ichMin, UINT ichStop, bool fRedraw)
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
	bool fCurrentView = (m_pzf->GetCurrentView() == this);
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

	if (ichMin < m_ichTopLine)
	{
		if (m_ichTopLine < ichMin + cch)
			m_ichTopLine = ichMin;
		else
			m_ichTopLine -= cch;
	}

	if (!fRedraw)
		return;

	if (ichStop >= m_ichTopLine && ichMin < m_prgli[m_cli].ich)
	{
		// The deleted characters are currently in view.
		InvalidateRect(m_hwnd, NULL, FALSE);
		UpdateWindow(m_hwnd);
	}

	if (fCaretAtEnd)
		m_ichDragStart = m_ichSelMin;
	else
		m_ichDragStart = m_ichSelLim;

	if (fCurrentView)
	{
		if (fCaretAtEnd)
		{
			m_ichDragStart = m_ichSelMin;
			m_ptCaretPos = PointFromChar(m_ichSelLim);
		}
		else
		{
			m_ichDragStart = m_ichSelLim;
			m_ptCaretPos = PointFromChar(m_ichSelMin);
		}
		if (GetFocus() == m_hwnd)
			SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
		m_ptRealPos = m_ptCaretPos;
	}

	m_ichMaxTopLine = ShowCharAtLine(m_pzd->GetParaCount(), m_pzd->GetCharCount(), m_cli - 2);

	SCROLLINFO si = {sizeof(si), SIF_PAGE | SIF_RANGE | SIF_POS};
	UINT cchTotal = m_pzd->GetCharCount();
	si.nPage = cchTotal - m_ichMaxTopLine;
	si.nMax = m_ichMaxTopLine ? cchTotal - 1 : 0;
	si.nPos = m_ichTopLine;
	m_pzf->SetScrollInfo(this, SB_VERT, &si);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::NotifyInsert(UINT ichMin, UINT cch)
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
	if (ichMin >= m_ichTopLine && ichMin < m_prgli[m_cli].ich)
	{
		InvalidateRect(m_hwnd, NULL, FALSE);
		//UpdateWindow(m_hwnd);
	}

	m_ichMaxTopLine = ShowCharAtLine(m_pzd->GetParaCount(), m_pzd->GetCharCount(), m_cli - 2);

	// TODO: Fix the vertical scrollbar
}


/***********************************************************************************************
	CColumnMarker methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CColumnMarker::CColumnMarker(int iColumn, COLORREF cr)
{
	m_iColumn = iColumn;
	m_cr = cr;
	m_nPixelOffset = 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CColumnMarker::OnDraw(HDC hdc, bool fErase, RECT & rect, int nXOffset)
{
	int nHalfWidth = knWedgeWidth >> 1;
	int nLeft = g_fg.m_rcMargin.left + rect.left + nXOffset +
		(m_iColumn * g_fg.m_ri.m_tm.tmAveCharWidth) - nHalfWidth;
	if (fErase)
	{
		RECT rc;
		::SetRect(&rc, nLeft, 0, nLeft + knWedgeWidth, knWedgeHeight);
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
		rc.bottom = rect.bottom;
		rc.top = rc.bottom - knWedgeHeight;
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	}
	else
	{
		m_nPixelOffset = nLeft;
		ImageList_Draw(g_fg.m_himlCM, 0, hdc, nLeft, 0, ILD_NORMAL);
		ImageList_Draw(g_fg.m_himlCM, 1, hdc, nLeft, rect.bottom - knWedgeHeight, ILD_NORMAL);
	}

	HPEN hPenOld = (HPEN)::SelectObject(hdc, ::CreatePen(PS_SOLID, 1, m_cr));
	int nOldROP = ::SetROP2(hdc, R2_NOTXORPEN);

	::MoveToEx(hdc, nLeft + nHalfWidth, knWedgeHeight, NULL);
	::LineTo(hdc, nLeft + nHalfWidth, rect.bottom - knWedgeHeight);

	::SetROP2(hdc, nOldROP);
	::DeleteObject(::SelectObject(hdc, hPenOld));
	return true;
}