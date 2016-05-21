#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


/***********************************************************************************************
	CFastEdit methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFastEdit::OnColumnMarkerDrag()
{
	// TODO: This probably needs to redraw the full window.
	/*HDC hdc = ::GetDC(m_hwnd);
	DrawSelection(hdc);
	::ReleaseDC(m_hwnd, hdc);*/
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	AssertPtr(m_pzef);
	AssertPtr(m_pzd);

	m_pzd->OnChar(m_ichSelMin, m_ichSelLim, nChar, m_fOvertype, m_fOnlyEditing);

	if (nChar == VK_ESCAPE || nChar == VK_BACK)
		return 0;
	if (nChar == VK_TAB && (m_ichSelMin != m_ichSelLim))
		return 0;

	if (nChar == 10 || nChar == 13)
	{
		m_fOnlyEditing = false;
		m_ichDragStart++;
	}

	// Deal with indenting stuff.
	ColorSchemeInfo * pcsi = m_pzd->GetColorScheme();
	AssertPtr(pcsi);
	IndentType it = pcsi->m_it;
	// TODO: Fix indent stuff so we can get rid of the Ansi check.
	// I need to come up with a real indent strategy here that will work for multiple languages.
	if (it != kitNone && m_pzd->GetFileType() == kftAnsi)
	{
		int cchSpacesInTab = g_fg.m_ri.m_cchSpacesInTab;

		if (nChar == 10 || nChar == 13)
		{
			// Get the level of indent on the previous line (in space characters).
			int cch = 0;
			UINT ipr;
			DocPart * pdp = m_pzd->GetPart(m_ichDragStart + 1, false, NULL, &ipr);
			if (--ipr == -1)
			{
				pdp = pdp->pdpPrev;
				if (pdp)
					ipr = pdp->cpr - 1;
			}
			if (pdp)
			{
				char * prgch = (char *)pdp->rgpv[ipr];
				char * prgchLim = prgch + PrintCharsInPara(pdp, ipr);
				while (prgch < prgchLim && isspace((unsigned char)*prgch))
				{
					if (*prgch == '\t')
						cch = ((cch / cchSpacesInTab) + 1) * cchSpacesInTab;
					else
						cch++;
					prgch++;
				}
				if (it == kitSmart && prgch < prgchLim)
				{
					// If the right-most non-space, non-comment character is not an ;, then
					// we can indent the next line.
					if (*prgch == '{')
					{
						// Make sure the brace isn't closed.
						while (prgch < prgchLim && *prgch != '}')
							prgch++;
						if (prgch == prgchLim)
							cch += cchSpacesInTab;
					}
					else
					{
						if (prgch + 1 < prgchLim && strncmp(prgch, "if", 2) == 0)
							cch += cchSpacesInTab;
						else if (prgch + 2 < prgchLim && strncmp(prgch, "for", 2) == 0)
							cch += cchSpacesInTab;
						else if (prgch + 3 < prgchLim && strncmp(prgch, "else", 2) == 0)
							cch += cchSpacesInTab;
					}
				}
			}
			void * pvInsert = NULL;
			if (cch)
				pvInsert = new char[cch];
			if (pvInsert)
			{
				if (g_fg.m_fUseSpaces)
				{
					memset(pvInsert, ' ', cch);
				}
				else
				{
					int cchT = cch / cchSpacesInTab;
					memset(pvInsert, '\t', cchT);
					cch %= cchSpacesInTab;
					memset((char *)pvInsert + cchT, ' ', cch);
					cch += cchT;
				}
				m_pzd->InsertText(m_ichDragStart + 1, (char *)pvInsert, cch, 0, true, kurtEmpty);
				delete pvInsert;
				// Update the top CUndoRedo entry.
				CUndoRedo * pur = m_pzd->GetUndo();
				AssertPtr(pur);
				Assert(pur->m_urt == kurtTyping);
				pur->m_urt = kurtSmartIndent;
				pur->m_cch += cch;
				m_ichDragStart += cch;
			}
		}
		else if (it == kitSmart)
		{
			if (nChar == '{' || nChar == '}')
			{
				// Make sure the only thing in front of the close brace is white space.
				int cch = 0;
				void * pv;
				UINT ipr;
				DocPart * pdp = m_pzd->GetPart(m_ichDragStart + 1, false, &pv, &ipr);
				char * prgchStart = (char *)pdp->rgpv[ipr];
				char * prgchT = (char *)pv - 2;
				int cchBefore = prgchT - prgchStart + 1;
				Assert(cchBefore >= 0);
				while (prgchT >= prgchStart && isspace((unsigned char)*prgchT))
					prgchT--;
				int cOpenBrace = 1;
				if (prgchT == prgchStart - 1)
				{
					// TODO: Make the number of lines a user-defined value.
					int cLine = 0;
					while (cOpenBrace && cLine++ < 400)
					{
						if (--ipr == -1)
						{
							pdp = pdp->pdpPrev;
							if (!pdp)
								break;
							ipr = pdp->cpr - 1;
						}
						prgchStart = (char *)pdp->rgpv[ipr];
						prgchT = prgchStart + PrintCharsInPara(pdp, ipr) - 1;
						if (nChar == '}')
						{
							// Find the matching open brace up to 400 lines.
							while (prgchT >= prgchStart)
							{
								if (*prgchT == '}')
								{
									cOpenBrace++;
								}
								else if (*prgchT == '{')
								{
									if (--cOpenBrace == 0)
										break;
								}
								prgchT--;
							}
						}
						else
						{
							// Find the first previous line that contains non-space characters.
							while (prgchT >= prgchStart && isspace((unsigned char)*prgchT--));
							if (prgchT >= prgchStart)
								cOpenBrace = 0;
						}
					}
					if (cOpenBrace == 0)
					{
						// We have found the matching open brace.
						AssertPtr(pdp);
						Assert((UINT)ipr < pdp->cpr);
						AssertPtr(prgchStart);
						AssertPtr(prgchT);
						prgchT = prgchStart;
						while (isspace((unsigned char)*prgchT))
						{
							if (*prgchT == '\t')
								cch = ((cch / cchSpacesInTab) + 1) * cchSpacesInTab;
							else
								cch++;
							prgchT++;
						}
					}

					void * pvInsert = NULL;
					if (cch || cchBefore)
						pvInsert = new char[cch];
					if (pvInsert)
					{
						if (g_fg.m_fUseSpaces)
						{
							memset(pvInsert, ' ', cch);
						}
						else
						{
							int cchT = cch / cchSpacesInTab;
							memset(pvInsert, '\t', cchT);
							cch %= cchSpacesInTab;
							memset((char *)pvInsert + cchT, ' ', cch);
							cch += cchT;
						}
						m_pzd->InsertText(m_ichDragStart - cchBefore, (char *)pvInsert, cch, cchBefore, true, kurtPaste);
						delete pvInsert;
					}
				}
			}
		}
	}
	m_ichSelMin = m_ichSelLim = ++m_ichDragStart;
	SetSelection(m_ichSelMin, m_ichSelLim, true, true);
	m_pzef->UpdateTextPos();
	m_pzef->m_pfd->ResetFind();
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnContextMenu(HWND hwnd, POINT pt)
{
	AssertPtr(m_pzd);

	m_fOnlyEditing = false;
	RECT rect;
	::GetClientRect(m_hwnd, &rect);
	POINT ptClient = pt;
	::MapWindowPoints(NULL, m_hwnd, &ptClient, 1);
	if (ptClient.y <= knWedgeHeight || ptClient.y >= rect.bottom - knWedgeHeight)
	{
		int iColumn = g_fg.GetColumnMarker(ptClient);
		if (iColumn != -1)
		{
			Assert((UINT)iColumn < (UINT)g_fg.m_vpcm[iColumn]);
			HMENU hMenu = m_pzef->GetContextMenu()->GetSubMenu(kcmColumn);
			int nID = ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON |
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
				::DialogBox(g_fg.m_hinst, MAKEINTRESOURCE(IDD_COLUMNMARKER), m_hwnd,
					&CZEditFrame::ColumnProc);
			}
			if (nID != 0)
			{
				// TODO: Invalidate all windows.
				// TODO: Why is this necessary?
				/*HDC hdc = ::GetDC(m_hwnd);
				DrawSelection(hdc);
				::ReleaseDC(m_hwnd, hdc);*/
			}
			return 0;
		}
	}
	HMENU hMenu = m_pzef->GetContextMenu()->GetSubMenu(kcmEdit);
	::SendMessage(m_pzef->GetHwnd(), WM_INITMENUPOPUP, (WPARAM)hMenu, 0);
	::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0,
		m_pzef->GetHwnd(), NULL);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFastEdit::OnFontChanged()
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
	m_ptRealPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	m_ptCaretPos = m_ptRealPos;
	::CreateCaret(m_hwnd, NULL, 2, g_fg.m_ri.m_tm.tmHeight);
	::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	::ShowCaret(m_hwnd);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnHScroll(UINT nSBCode, UINT nPos, HWND hwndScroll)
{
	if (!m_hwnd)
		return 1;
	return 1;

	// TODO
	/*SCROLLINFO si = {sizeof(SCROLLINFO)};
	RECT rect;
	GetClientRect(m_hwnd, &rect);

	switch (nSBCode)
	{
		case SB_LINELEFT:
			OffsetRect(&m_rcScreen, g_fg.m_tm.tmMaxCharWidth, 0);
			if (m_rcScreen.left > g_fg.m_rcMargin.left)
				m_rcScreen = g_fg.m_rcMargin;
			break;

		case SB_LINERIGHT:
			{
				OffsetRect(&m_rcScreen, -g_fg.m_tm.tmMaxCharWidth, 0);
				DWORD dwTotalWidth = m_dwCharsInLongestLine * g_fg.m_tm.tmMaxCharWidth;
				if (g_fg.m_rcMargin.left - m_rcScreen.left > dwTotalWidth)
					OffsetRect(&m_rcScreen, g_fg.m_rcMargin.left - m_rcScreen.left - dwTotalWidth, 0);
				break;
			}

		case SB_PAGELEFT:
			OffsetRect(&m_rcScreen, rect.right, 0);
			if (m_rcScreen.left > g_fg.m_rcMargin.left)
				m_rcScreen = g_fg.m_rcMargin;
			break;

		case SB_PAGERIGHT:
			{
				OffsetRect(&m_rcScreen, -rect.right, 0);
				DWORD dwTotalWidth = m_dwCharsInLongestLine * g_fg.m_tm.tmMaxCharWidth;
				if (g_fg.m_rcMargin.left - m_rcScreen.left > dwTotalWidth)
					OffsetRect(&m_rcScreen, g_fg.m_rcMargin.left - m_rcScreen.left - dwTotalWidth, 0);
				break;
			}

		case SB_THUMBTRACK:
			si.fMask |= SIF_TRACKPOS;
			m_pzf->GetScrollInfo(this, SB_VERT, &si);
			m_rcScreen = g_fg.m_rcMargin;
			OffsetRect(&m_rcScreen, -si.nTrackPos, 0);
			break;

		default:
			return 0;
	}

	si.fMask = SIF_POS;
	si.nPos = g_fg.m_rcMargin.left - m_rcScreen.left;
	m_pzf->SetScrollInfo(this, SB_HORZ, &si);
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
	POINT pt = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	m_ptRealPos.x += (pt.x - m_ptCaretPos.x);
	m_ptRealPos.y += (pt.y - m_ptCaretPos.y);
	m_ptCaretPos = pt;
	SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	return 0;*/
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	AssertPtr(m_pzd);
	AssertPtr(m_pzef);
	if (!m_hwnd)
		return 1;

	bool fShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
	bool fControl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	bool fRedraw = false;
	bool fUpdateReal = true;
	bool fSelection = m_ichSelMin != m_ichSelLim;
	bool fMoveDown = true;
	bool fForceRedraw = false;
	UINT ichCursor = (m_ichDragStart == m_ichSelMin) ? m_ichSelLim : m_ichSelMin;

	switch (nChar)
	{
		case VK_BACK:
		{
			if (m_ichSelMin == 0 && m_ichSelLim == 0)
				return 1;
			UINT iStart = m_ichSelMin;
			if (m_ichSelMin == m_ichSelLim)
			{
				iStart--;
				void * pv;
				UINT ipr;
				DocPart * pdp = m_pzd->GetPart(m_ichSelMin, false, &pv, &ipr);
				if (pv == pdp->rgpv[ipr])
					iStart--;
			}
			UINT cch = m_ichSelLim - iStart;
			CUndoRedo * pur = new CUndoRedo(kurtDelete, iStart, cch, 0);
			if (pur)
			{
				m_pzd->GetText(iStart, cch, &pur->m_pzdo);
				m_pzd->PushUndoEntry(pur);
			}
			m_pzd->DeleteText(iStart, m_ichSelLim, true);
			SetSelection(iStart, iStart, true, false);
			m_pzef->UpdateTextPos();
			return 0;
		}

		case VK_LEFT:
			{
				fMoveDown = false;
				UINT * pich = (m_ichDragStart == m_ichSelLim) ? &m_ichSelMin : &m_ichSelLim;
				if (fControl)
				{
					// Move to the beginning of the previous word
					void * pv;
					UINT ipr;
					DocPart * pdp = m_pzd->GetPart(*pich, false, &pv, &ipr);
					void * pvPara = pdp->rgpv[ipr];
					if (m_pzd->IsAnsi(pdp, ipr))
					{
						// pv is pointing to an 8-bit string
						char * pch = (char *)pv - 1;
						if (pch >= pvPara)
						{
							// Skip past spaces.
							while (pch >= pvPara && isspace((unsigned char)*pch))
								pch--;
							if (pch >= pvPara)
							{
								if (__iscsym((unsigned char)*pch))
								{
									// Go to the beginning of the letters.
									while (pch >= pvPara && __iscsym((unsigned char)*pch))
										pch--;
								}
								else
								{
									// Go to the first non-punctuation character.
									while (pch >= pvPara && ispunct(*pch))
										pch--;
								}
							}
							*pich += pch - (char *)pv + 1;
						}
						else
						{
							// We are at the beginning of a paragraph, so move
							// to the end of the previous paragraph.
							if (ipr > 0)
								(*pich) -= CharsAtEnd(pdp, ipr - 1);
							else if (pdp->pdpPrev)
								(*pich) -= CharsAtEnd(pdp->pdpPrev, pdp->pdpPrev->cpr - 1);
							else
								*pich = 0; // We are at the top of the file.
						}
					}
					else
					{
						// pv is pointing to a 16-bit string
						wchar * pwch = (wchar *)pv - 1;
						if (pwch >= pvPara)
						{
							// Skip past spaces.
							while (pwch >= pvPara && iswspace((unsigned char)*pwch))
								pwch--;
							if (pwch >= pvPara)
							{
								if (__iscsym((unsigned char)*pwch))
								{
									// Go to the beginning of the letters.
									while (pwch >= pvPara && __iscsym((unsigned char)*pwch))
										pwch--;
								}
								else
								{
									// Go to the first non-punctuation character.
									while (pwch >= pvPara && iswpunct(*pwch))
										pwch--;
								}
							}
							*pich += pwch - (wchar *)pv + 1;
						}
						else
						{
							// We are at the beginning of a paragraph, so move
							// to the end of the previous paragraph.
							if (ipr > 0)
								(*pich) -= CharsAtEnd(pdp, ipr - 1);
							else if (pdp->pdpPrev)
								(*pich) -= CharsAtEnd(pdp->pdpPrev, ipr);
							else
								*pich = 0; // We are at the top of the file.
						}
					}
				}
				else
				{
					// Move left one character.
					if (*pich > 0 && (fShift || !fSelection))
					{
						void * pv;
						UINT ipr;
						DocPart * pdp = m_pzd->GetPart(*pich, false, &pv, &ipr);
						if (pv == pdp->rgpv[ipr])
						{
							// We are at the beginning of a paragraph.
							if (ipr > 0)
								(*pich) -= CharsAtEnd(pdp, ipr - 1);
							else
							{
								AssertPtr(pdp->pdpPrev);
								(*pich) -= CharsAtEnd(pdp->pdpPrev, pdp->cpr - 1);
							}
						}
						else if (m_pzd->IsAnsi(pdp, ipr))
						{
							(*pich)--;
						}
						else
						{
							UINT ichInPara = (((char *)pv - (char *)pdp->rgpv[ipr]) >> (int)(!m_pzd->IsAnsi(pdp, ipr)));
							wchar ch = (ichInPara > 1) ? ((wchar *)pdp->rgpv[ipr])[ichInPara - 2] : 0;
							if (ch < 0xd800 || ch > 0xdbff)
								(*pich)--;
							else
								*pich -= 2;
						}
					}
				}
				if (*pich > m_pzd->GetCharCount())
					*pich = 0;
				if (fShift && pich == &m_ichSelLim)
					fMoveDown = true;
				break;
			}

		case VK_RIGHT:
			{
				UINT * pich = (m_ichDragStart == m_ichSelMin) ? &m_ichSelLim : &m_ichSelMin;
				if (fControl)
				{
					void * pv;
					UINT ipr;
					DocPart * pdp = m_pzd->GetPart(*pich, false, &pv, &ipr);
					if (m_pzd->IsAnsi(pdp, ipr))
					{
						char * pch = (char *)pv;
						char * pchStop = (char *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
						if (pch < pchStop)
						{
							if (__iscsym((unsigned char)*pch))
							{
								while (pch < pchStop && __iscsym((unsigned char)*pch))
									pch++;
							}
							else
							{
								while (pch < pchStop && !__iscsym((unsigned char)*pch) && !isspace((unsigned char)*pch))
									pch++;
							}
							while (pch < pchStop && isspace((unsigned char)*pch))
								pch++;
							*pich += pch - (char *)pv;
						}
						else
						{
							(*pich) += CharsAtEnd(pdp, ipr);
						}
					}
					else
					{
						wchar * pwch = (wchar *)pv;
						wchar * pwchStop = (wchar *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
						if (pwch < pwchStop)
						{
							if (__iscsym((unsigned char)*pwch))
							{
								while (pwch < pwchStop && __iscsym((unsigned char)*pwch))
									pwch++;
							}
							else
							{
								while (pwch < pwchStop && !__iscsym((unsigned char)*pwch) && !iswspace((unsigned char)*pwch))
									pwch++;
							}
							while (pwch < pwchStop && iswspace((unsigned char)*pwch))
								pwch++;
							*pich += pwch - (wchar *)pv;
						}
						else
						{
							(*pich) += CharsAtEnd(pdp, ipr);
						}
					}
				}
				else
				{
					if (*pich < m_pzd->GetCharCount() && (fShift || !fSelection))
					{
						void * pv;
						UINT ipr;
						DocPart * pdp = m_pzd->GetPart(*pich, false, &pv, &ipr);
						UINT ichInPara = ((char *)pv - (char *)pdp->rgpv[ipr]) >> (int)(!m_pzd->IsAnsi(pdp, ipr));
						if (ichInPara == PrintCharsInPara(pdp, ipr))
						{
							(*pich) += 2;
						}
						else if (m_pzd->IsAnsi(pdp, ipr))
						{
							(*pich)++;
						}
						else
						{
							wchar ch = ((wchar *)pdp->rgpv[ipr])[ichInPara];
							if (ch < 0xd800 || ch > 0xdbff)
								(*pich)++;
							else
								*pich += 2;
						}
					}
				}
				if (fShift && pich == &m_ichSelMin)
					fMoveDown = false;
				break;
			}

		case VK_UP:
		{
			if (fControl)
			{
				OnVScroll(SB_LINEUP, 0, 0);
				m_pzef->UpdateTextPos();
				return 1;
			}
			if (m_ichTopLine == 0 && m_ptCaretPos.y == g_fg.m_rcMargin.top)
			{
				m_ichSelLim = m_ichSelMin;
				break;
			}
			if (m_ichDragStart == m_ichSelMin && !fShift && m_ichSelMin != m_ichSelLim)
				m_ptRealPos = PointFromChar(m_ichSelMin);
			m_ptRealPos.y -= g_fg.m_ri.m_tm.tmHeight;
			if (m_ptRealPos.y < m_rcScreen.top)
			{
				OnVScroll(SB_LINEUP, 0, 0);
				m_ptRealPos.y -= g_fg.m_ri.m_tm.tmHeight;
			}
			m_ichSelMin = CharFromPoint(m_ptRealPos, true);
			fUpdateReal = false;
			fMoveDown = false;
			break;
		}

		case VK_DOWN:
		{
			if (fControl)
			{
				OnVScroll(SB_LINEDOWN, 0, 0);
				m_pzef->UpdateTextPos();
				return 1;
			}
			RECT rc;
			GetClientRect(m_hwnd, &rc);
			if (m_ichDragStart == m_ichSelLim && !fShift && m_ichSelMin != m_ichSelLim)
				m_ptRealPos = PointFromChar(m_ichSelLim);
			if (m_ptRealPos.y > (rc.bottom - m_rcScreen.bottom) - (g_fg.m_ri.m_tm.tmHeight << 1))
				OnVScroll(SB_LINEDOWN, 0, 0);
			m_ptRealPos.y += g_fg.m_ri.m_tm.tmHeight;
			m_ichSelLim = CharFromPoint(m_ptRealPos, true);
			fUpdateReal = false;
			break;
		}

		case VK_HOME:
			if (fControl)
			{
				m_ichSelMin = 0;
				if (m_ichTopLine != 0) // Not already at the top
				{
					m_iprTopLine = m_ichTopLine = 0;
					fRedraw = true;
					SCROLLINFO si = {sizeof(SCROLLINFO), SIF_POS};
 					m_pzf->SetScrollInfo(this, SB_VERT, &si);
					fForceRedraw = true;
				}
			}
			else
			{
				UINT ich;
				if (fShift)
					ich = m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin;
				else
					ich = m_ichSelMin;
				UINT ichSelMin;
				if (m_ichTopLine <= ich && ich < m_prgli[m_cli].ich)
				{
					// The character is currently being shown on the screen.
					int iLine = 0;
					if (m_prgli[0].ich <= ich)
					{
						/*if (m_ptCaretPos.x == m_rcScreen.left)
							ich++;*/
						// 2005-7-9 - changed this because it wasn't selecting the
						// beginning of the line correctly when there was a selection.
						//int cchTotal = m_pzd->GetCharCount();
						//while (m_prgli[++iLine].ich <= ich && ich < cchTotal)
						
						
						while (m_prgli[++iLine].ich < ich)
						{
							Assert((UINT)iLine < m_cli);
						}
						if (m_prgli[iLine].ich > ich || m_prgli[iLine].ipr >= m_pzd->GetParaCount())
							iLine--;
						/*&& m_prgli[iLine].dxp == 0 &&
							m_prgli[iLine].ipr >= m_pzd->GetParaCount())
						{
							iLine--;
						}*/
						ichSelMin = m_prgli[iLine].ich;
						


						/*while (m_prgli[iLine + 1].ich <= ich)
						{
							iLine++;
							Assert((UINT)iLine < m_cli);
						}
						if (m_prgli[iLine].ich == -1 && iLine > 0)
							iLine--;
						ichSelMin = m_prgli[iLine].ich;*/
						/*while (m_prgli[++iLine].ich < ich)
						{
							Assert((UINT)iLine < m_cli);
						}
						if (m_prgli[iLine].ich == ich && m_prgli[iLine].dxp > 0)
							ichSelMin = m_prgli[iLine].ich;
						else
							ichSelMin = m_prgli[--iLine].ich;*/
					}
					else
					{
						ichSelMin = m_prgli[0].ich;
					}
				}
				else
				{
					ichSelMin = m_pzd->CharFromPara(m_pzd->ParaFromChar(ich));
				}

				if (m_pzd->GetColorScheme()->m_it != kitNone)
				{
					UINT ipr;
					int cchWhite;
					DocPart * pdp = m_pzd->GetPart(m_ichSelMin, false, NULL, &ipr);
					if (m_pzd->IsAnsi(pdp, ipr))
					{
						char * prgch = (char *)pdp->rgpv[ipr];
						char * prgchLim = prgch + PrintCharsInPara(pdp, ipr);
						while (prgch < prgchLim && isspace((unsigned char)*prgch))
							prgch++;
						cchWhite = prgch - (char *)pdp->rgpv[ipr];
					}
					else
					{
						wchar * prgch = (wchar *)pdp->rgpv[ipr];
						wchar * prgchLim = prgch + PrintCharsInPara(pdp, ipr);
						while (prgch < prgchLim && iswspace((wchar)*prgch))
							prgch++;
						cchWhite = prgch - (wchar *)pdp->rgpv[ipr];
					}
					ich = (!fShift || m_ichDragStart == m_ichSelMin) ? m_ichSelLim : m_ichSelMin;
					//if (ich == m_ichSelMin || ich > m_ichSelMin + cchWhite)
					if (ichSelMin + cchWhite != ich)
						ichSelMin += cchWhite;
				}
				m_ichSelMin = ichSelMin;
			}
			fMoveDown = false;
			break;

		case VK_END:
			if (fControl)
			{
				m_ichSelLim = m_pzd->GetCharCount();
				if (m_ichTopLine < m_ichMaxTopLine) // Not already at the bottom
				{
					m_ichTopLine = m_ichMaxTopLine;
					m_iprTopLine = m_pzd->ParaFromChar(m_ichTopLine);
					fRedraw = true;
					SCROLLINFO si = {sizeof(SCROLLINFO), SIF_POS};
					si.nPos = m_ichTopLine;
					m_pzf->SetScrollInfo(this, SB_VERT, &si);
					fForceRedraw = true;
				}
			}
			else
			{
				UINT ich;
				if (fShift)
					ich = m_ichDragStart == m_ichSelLim ? m_ichSelMin : m_ichSelLim;
				else
					ich = m_ichSelLim;
				if (m_ichTopLine <= ich && ich < m_prgli[m_cli].ich)
				{
					int iLine = 0;
					if (m_prgli[0].ich <= ich)
					{
						while (m_prgli[++iLine].ich <= ich)
						{
							Assert((UINT)iLine < m_cli);
						}
					}
					m_ichSelLim = m_prgli[iLine].ich;
				}
				else
				{
					m_ichSelLim = m_pzd->CharFromPara(m_pzd->ParaFromChar(ich) + 1);
				}
				Assert(m_ichSelLim > 0);
				UINT ipr;
				void * pv;
				UINT cchTotal = m_pzd->GetCharCount();
				if (m_ichSelLim == -1)
					m_ichSelLim = cchTotal + 1;
				DocPart * pdp = m_pzd->GetPart(--m_ichSelLim, false, &pv, &ipr);
				if (CharsAtEnd(pdp, ipr) == 0)
					m_ichSelLim = cchTotal;
				else
				{
					int cchPara = PrintCharsInPara(pdp, ipr);
					if (m_pzd->IsAnsi(pdp, ipr))
					{
						if ((char *)pdp->rgpv[ipr] + cchPara < pv)
							m_ichSelLim -= (char *)pv - ((char *)pdp->rgpv[ipr] + cchPara);
					}
					else
					{
						if ((wchar *)pdp->rgpv[ipr] + cchPara < pv)
							m_ichSelLim -= (wchar *)pv - ((wchar *)pdp->rgpv[ipr] + cchPara);
					}
				}
				Assert(m_ichSelLim <= cchTotal);
			}
			break;

		case VK_PRIOR: // page up
		{
			if (m_ichTopLine == 0)
			{
				m_ptRealPos.y = 0;
			}
			else
			{
			/* TODO: Try the following:*/
				m_ichTopLine = ShowCharAtLine(m_prgli[0].ipr,
					m_prgli[0].ich, m_cli - 1);
				m_iprTopLine = m_pzd->ParaFromChar(m_ichTopLine);
				::InvalidateRect(m_hwnd, NULL, FALSE);
				::UpdateWindow(m_hwnd);
				SCROLLINFO si = { sizeof(si), SIF_POS };
				//if (m_fWrap)
					si.nPos = m_ichTopLine;
				/*else
					si.nPos = m_iprTopLine;*/
				m_pzf->SetScrollInfo(this, SB_VERT, &si);
			}
			//OnVScroll(SB_PAGEUP, 0, 0);
			// instead of the above line.
			if (fControl)
			{
				m_pzef->UpdateTextPos();
				return 1;
			}
			m_ichSelMin = CharFromPoint(m_ptRealPos, true);
			fUpdateReal = false;
			fMoveDown = false;
			break;
		}

		case VK_NEXT: // page down
		{
			// TODO: Make sure this works properly.
			if (m_ichTopLine >= m_ichMaxTopLine)
				return 0;
			/* Maybe use the following:*/
			m_ichTopLine = m_prgli[m_cli - 2].ich;
			m_iprTopLine = m_prgli[m_cli - 2].ipr;
			InvalidateRect(m_hwnd, NULL, FALSE);
			UpdateWindow(m_hwnd);
			SCROLLINFO si = { sizeof(si), SIF_POS };
			//if (m_fWrap)
				si.nPos = m_ichTopLine;
			/*else
				si.nPos = m_iprTopLine;*/
			m_pzf->SetScrollInfo(this, SB_VERT, &si);
			//OnVScroll(SB_PAGEDOWN, 0, 0);
			// instead of the above line.
			if (fControl)
			{
				m_pzef->UpdateTextPos();
				return 1;
			}
			if (m_pzd->GetParaCount() <= m_prgli[m_cli].ipr)
				m_ichSelLim = m_pzd->GetCharCount();
			else
				m_ichSelLim = CharFromPoint(m_ptRealPos, true);
			fUpdateReal = false;
			break;
		}

		case VK_INSERT:
			m_fOvertype = m_fOvertype ? false : true;
			m_fOnlyEditing = false;
			m_pzef->SetStatusText(ksboInsert, m_fOvertype ? "OVR" : "INS");
			return 0;

		case VK_DELETE:
		{
			if (m_ichSelMin >= m_pzd->GetCharCount())
				return 1;
			UINT cch = m_ichSelLim - m_ichSelMin;
			if (!cch)
			{
				void * pv;
				UINT ipr;
				DocPart * pdp = m_pzd->GetPart(m_ichSelMin, false, &pv, &ipr);
				if ((char *)pv == (char *)pdp->rgpv[ipr] + (PrintCharsInPara(pdp, ipr) << (int)(!m_pzd->IsAnsi(pdp, ipr))))
					cch = 2;
				else
					cch = 1;
			}
			CUndoRedo * pur = new CUndoRedo(kurtDelete, m_ichSelMin, cch, 0);
			if (pur)
			{
				m_pzd->GetText(m_ichSelMin, cch, &pur->m_pzdo);
				m_pzd->PushUndoEntry(pur);
			}
			m_pzd->DeleteText(m_ichSelMin, m_ichSelMin + cch, true);
			SetSelection(m_ichSelMin, m_ichSelMin, true, false);
			m_pzef->UpdateTextPos();
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

	int nOldX = m_ptRealPos.x;
	if (m_ichSelMin == m_ichDragStart)
		SetSelection(m_ichSelMin, m_ichSelLim, true, fForceRedraw);
	else
		SetSelection(m_ichSelLim, m_ichSelMin, true, fForceRedraw);
	if (!fUpdateReal)
		m_ptRealPos.x = nOldX;

	switch (nChar)
	{
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
	case VK_HOME:
	case VK_END:
	case VK_PRIOR:
	case VK_NEXT:
		if (m_pzd->GetColorScheme()->m_it == kitSmart)
		{
			if (m_pzd->CanUndo())
			{
				// If we just performed a smart indent and we're on an empty line (except
				// for white space), remove the inserted white space.
				CUndoRedo * pur = m_pzd->GetUndo();
				AssertPtr(pur);
				if (pur->m_urt == kurtSmartIndent)
				{
					// TODO: Fix indent stuff so that this doesn't have to be true.
					Assert(m_pzd->GetFileType() == kftAnsi);
					UINT iprOffset;
					UINT cchBefore;
					DocPart * pdp = m_pzd->GetPart(ichCursor, false, NULL, &iprOffset, &cchBefore);
					char * prgch = (char *)pdp->rgpv[iprOffset];
					char * prgchLim = prgch + PrintCharsInPara(pdp, iprOffset);
					if (prgch != prgchLim)
					{
						while (prgch < prgchLim && isspace((unsigned char)*prgch))
							prgch++;
						if (prgch == prgchLim)
						{
							// The line consists of only spaces, so remove them.
							for (UINT iprT = 0; iprT < iprOffset; iprT++)
								cchBefore += CharsInPara(pdp, iprT);
							UINT cchRemoved = PrintCharsInPara(pdp, iprOffset);
							m_pzd->DeleteText(cchBefore, cchBefore + cchRemoved, true);
							// Update the top CUndoRedo entry.
							Assert(pur->m_cch > cchRemoved);
							pur->m_urt = kurtTyping;
							pur->m_cch -= cchRemoved;
						}
					}
				}
			}
		}
		// Fall-through.
	case VK_DELETE:
		m_pzef->UpdateTextPos();
		break;
	}

	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnKillFocus(HWND hwndGetFocus)
{
	if (!m_hwnd)
		return 1;

	m_fOnlyEditing = false;
	::HideCaret(m_hwnd);
	::DestroyCaret();
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnLButtonDblClk(UINT nFlags, POINT point)
{
	AssertPtr(m_pzd);

	RECT rect;
	::GetClientRect(m_hwnd, &rect);
	if (point.y <= knWedgeHeight || point.y >= rect.bottom - knWedgeHeight)
	{
		return (::DialogBox(g_fg.m_hinst, MAKEINTRESOURCE(IDD_COLUMNMARKER), m_hwnd,
			&CZEditFrame::ColumnProc));
	}

	if (m_pzd->GetCharCount() == 0)
		return 0;

	void * pv;
	UINT ich = m_ichSelMin;
	UINT iprOffset = 0;
	UINT iStart;
	UINT iStop;
	m_fOnlyEditing = false;
	DocPart * pdp = m_pzd->GetPart(ich, false, &pv, &iprOffset);
	void * pvStart = pdp->rgpv[iprOffset];
	void * pvStop = pvStart;

	// If the line is empty, we're done.
	if (CharsInPara(pdp, iprOffset) == 0)
		return 0;

	if (m_pzd->IsAnsi(pdp, iprOffset))
	{
		char * prgchPos = (char *)pv;
		while (prgchPos >= pvStart && __iscsym((unsigned char)*prgchPos))
			prgchPos--;
		iStart = ich + prgchPos - (char *)pv + 1;

		prgchPos = (char *)pv;
		pvStop = (char *)pvStop + PrintCharsInPara(pdp, iprOffset);
		while (prgchPos < pvStop && __iscsym((unsigned char)*prgchPos))
			prgchPos++;
		iStop = ich + prgchPos - (char *)pv;
	}
	else
	{
		wchar * prgchPos = (wchar *)pv;
		while (prgchPos >= pvStart && __iscsym((unsigned char)*prgchPos))
			prgchPos--;
		iStart = ich + prgchPos - (wchar *)pv + 1;

		prgchPos = (wchar *)pv;
		pvStop = (wchar *)pvStop + PrintCharsInPara(pdp, iprOffset);
		while (prgchPos < pvStop && __iscsym((unsigned char)*prgchPos))
			prgchPos++;
		iStop = ich + prgchPos - (wchar *)pv;
	}

	SetSelection(iStart, iStop, true, false);
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnLButtonDown(UINT nFlags, POINT point)
{
	AssertPtr(m_pzd);
	AssertPtr(m_pzf);
	if (!m_hwnd)
		return 1;

	m_pzf->SetCurrentView(this);
	m_pzef->UpdateTextPos();
	::SetFocus(m_hwnd);
	m_fOnlyEditing = false;

	RECT rect;
	::GetClientRect(m_hwnd, &rect);
	if (point.y <= knWedgeHeight || point.y >= rect.bottom - knWedgeHeight)
	{
		g_fg.m_iColumnDrag = g_fg.GetColumnMarker(point);
		if (g_fg.m_iColumnDrag != -1)
		{
			// Create the tooltip window.
			Assert((UINT)g_fg.m_iColumnDrag < (UINT)g_fg.m_vpcm.size());
			HWND hwndToolTip = g_fg.GetToolTipHwnd();
			if (hwndToolTip)
			{
				TOOLINFO ti = {sizeof(TOOLINFO), TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE};
				ti.hwnd = m_pzef->GetHwnd();
				ti.uId = (UINT)m_hwnd;
				ti.hinst = g_fg.m_hinst;
				char szText[10];
				_itoa_s(g_fg.m_vpcm[g_fg.m_iColumnDrag]->m_iColumn, szText, 10);
				ti.lpszText = szText;
				::SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

				// Create the tooltip offscreen so we can move it to the right
				// position later once we know how big it is.
				::SendMessage(hwndToolTip, TTM_TRACKPOSITION, 0, MAKELPARAM(-50000, -50000));
				// Activate/show the tooltip.
				::SendMessage(hwndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
				// Find the starting position for the tooltip.
				POINT pt = { g_fg.m_vpcm[g_fg.m_iColumnDrag]->m_iColumn *
					g_fg.m_ri.m_tm.tmAveCharWidth + m_rcScreen.left };
				RECT rect;
				::GetClientRect(m_hwnd, &rect);
				::MapWindowPoints(m_hwnd, NULL, (POINT *)&rect, 2);
				pt.x += rect.left;
				pt.y = rect.top;
				::GetWindowRect(hwndToolTip, &rect);
				pt.y -= (rect.bottom - rect.top + 2);
				pt.x -= (rect.right - rect.left - 1) / 2;
				::SendMessage(hwndToolTip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));
			}

			::MapWindowPoints(m_hwnd, NULL, (POINT *)&rect, 2);
			::ClipCursor(&rect);
		}
		return 0;
	}

	if (m_pzd->GetCharCount() == 0)
		return 0;

	if (m_ichSelMin != m_ichSelLim)
	{
		UINT ich = CharFromPoint(point, false);
		if (m_ichSelMin <= ich && ich < m_ichSelLim)
		{
			// Start an OLE drag operation
			CZDataObject * pzdo;
			UINT cch = m_pzd->GetText(m_ichSelMin, m_ichSelLim - m_ichSelMin, &pzdo);
			if (pzdo)
			{
				DWORD dwEffect;
				m_fIsDropSource = true;
				::DoDragDrop(pzdo, m_pds, (ULONG)-1, &dwEffect);
				::CreateCaret(m_hwnd, NULL, 2, g_fg.m_ri.m_tm.tmHeight);
				::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
				::ShowCaret(m_hwnd);
				if (m_fIsDropSource && (dwEffect & DROPEFFECT_MOVE))
				{
					CUndoRedo * pur = new CUndoRedo(kurtMove, m_ichSelMin, cch, 0);
					if (pur)
					{
						pur->m_mt = kmtCut;
						pur->m_pzdo = pzdo;
						pzdo = NULL;
						m_pzd->PushUndoEntry(pur);
					}
					m_pzd->DeleteText(m_ichSelMin, m_ichSelLim, true);
					SetSelection(m_ichSelMin, m_ichSelMin, true, false);
					m_fIsDropSource = false;
				}
				if (pzdo)
					pzdo->Release();
			}
			return 0;
		}
	}

	if (nFlags & MK_CONTROL)
	{
		m_ichSelMin = CharFromPoint(point, false);
		return OnLButtonDblClk(nFlags, point);
	}

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
	m_fSelecting = true;
	if (iOldSelStart != iOldSelStop || m_ichSelMin != m_ichSelLim)
	{
		::InvalidateRect(m_hwnd, NULL, FALSE);
		::UpdateWindow(m_hwnd);
	}
	::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnLButtonUp(UINT nFlags, POINT point)
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
		m_ptRealPos = m_ptCaretPos;
		::ShowCaret(m_hwnd);
	}
	else if (g_fg.m_iColumnDrag != -1)
	{
		g_fg.m_iColumnDrag = -1;
		::ClipCursor(NULL);

		HWND hwndToolTip = g_fg.GetToolTipHwnd();
		if (hwndToolTip)
		{
			TOOLINFO ti = { sizeof(TOOLINFO) };
			ti.hwnd = m_pzef->GetHwnd();
			ti.uId = (UINT)m_hwnd;
			::SendMessage(hwndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
			::SendMessage(hwndToolTip, TTM_DELTOOL, 0, (LPARAM)&ti);
		}
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnMButtonDown(UINT nFlags, POINT point)
{
	return 1;
	// TODO
	/*if (!m_hwnd)
		return 1;

	m_fOnlyEditing = false;

	SetCapture(m_hwnd);

	// Find the coordinate to show the background image
	POINT ptImage;
	ptImage.x = point.x - 7;
	ptImage.y = point.y - 11;

	// Load imagelist and cursors
	HINSTANCE hinst = (HINSTANCE)GetWindowLong(m_hwnd, GWL_HINSTANCE);
	HIMAGELIST himl = ImageList_LoadImage(hinst, MAKEINTRESOURCE(IDB_SCROLL), 14, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	HCURSOR hcurUp = LoadCursor(hinst, MAKEINTRESOURCE(IDC_SCROLL_UP));
	HCURSOR hcurMiddle = LoadCursor(hinst, MAKEINTRESOURCE(IDC_SCROLL_MIDDLE));
	HCURSOR hcurDown = LoadCursor(hinst, MAKEINTRESOURCE(IDC_SCROLL_DOWN));

	MSG msg;
	RECT rect;
	GetClientRect(m_hwnd, &rect);

	BOOL fHasScrolled = FALSE;
	BOOL fContinue = TRUE;
	BOOL fScrollingDown = FALSE;
	int nOffset = 0;

	// Create a temporary DC in memory for new lines used in partial-line scrolling
	HDC hdcScreen = GetDC(m_hwnd);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	RECT rectMem = {0, 0, rect.right, g_fg.m_tm.tmHeight * 2};
	HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, CreateCompatibleBitmap(hdcScreen, rectMem.right, rectMem.bottom));
	HFONT hOldFont = (HFONT)SelectObject(hdcMem, g_fg.m_hFont);
	SetTextAlign(hdcMem, TA_TOP | TA_LEFT | TA_UPDATECP);
	SetBkColor(hdcMem, g_fg.m_cr[kclrWindow]);
	::SetTextColor(hdcMem, g_fg.m_cr[kclrText]);
	ImageList_Draw(himl, 0, hdcScreen, ptImage.x, ptImage.y, ILD_NORMAL);
	BOOL fInBlock;
	BOOL fInBlockP;

	// TODO: For some reason, the cursor gets lost when outside of this window.
	// Loop until a key is pressed, or a mouse button is clicked, or the middle button is released
	while (fContinue)
	{
		while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_LBUTTONDOWN ||
				msg.message == WM_RBUTTONDOWN ||
				(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) ||
				msg.message == WM_MOUSEWHEEL)
			{
				fContinue = FALSE;
			}
			if (msg.message == WM_MBUTTONUP && fHasScrolled)
				fContinue = FALSE;
			if (msg.message == WM_MBUTTONDOWN && !fHasScrolled)
				fHasScrolled = TRUE;
		}

		// Find out where the cursor is in relation to where it was when the scroll started
		POINT ptCursor;
		GetCursorPos(&ptCursor);
		MapWindowPoints(NULL, m_hwnd, &ptCursor, 1);
		int nDiff = 0;
		if (ptCursor.y < ptImage.y)
		{
			nDiff = ptCursor.y - ptImage.y;
			SetCursor(hcurUp);
		}
		else if (ptCursor.y > ptImage.y + 22)
		{
			nDiff = ptCursor.y - ptImage.y - 22;
			SetCursor(hcurDown);
		}
		else
		{
			SetCursor(hcurMiddle);
			continue;
		}
		fHasScrolled = TRUE;

		HideCaret(m_hwnd);
		int nAbsDiff = abs(nDiff);
		if (nAbsDiff > 50)
		{
			// Scroll by one or more lines
			if (nAbsDiff > 150)
			{
				// TODO: Do something a little more intelligent here
				OnVScroll(nDiff < 0 ? SB_PAGEUP : SB_PAGEDOWN, 0, 0);
				OnVScroll(nDiff < 0 ? SB_PAGEUP : SB_PAGEDOWN, 0, 0);
			}
			else if (nDiff > 110)
			{
				OnVScroll(nDiff < 0 ? SB_PAGEUP : SB_PAGEDOWN, 0, 0);
			}
			else if (nDiff > 80)
			{
				// TODO: Do something a little more intelligent here
				OnVScroll(nDiff < 0 ? SB_LINEUP : SB_LINEDOWN, 0, 0);
				OnVScroll(nDiff < 0 ? SB_LINEUP : SB_LINEDOWN, 0, 0);
			}
			else
			{
				OnVScroll(nDiff < 0 ? SB_LINEUP : SB_LINEDOWN, 0, 0);
			}
		}
		if (nAbsDiff <= 50 || !fContinue)
		{
			do
			{
				static BOOL fOldScrollingDown = FALSE;
				fScrollingDown = nDiff > 0;
				// Scroll by partial lines
				if (++nOffset == 1 || fOldScrollingDown != fScrollingDown)
				{
					if (nDiff > 0)
					{
						if (m_ichTopLine > m_ichMaxTopLine)
						{
							nOffset = 0;
							continue;
						}
						// Draw the bottom two lines into the memory DC
						ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rectMem, NULL, 0, NULL);
						int dxpLine = rect.right - g_fg.m_rcMargin.left - g_fg.m_rcMargin.right;
						void * pPos;
						void * pStop;
						DWORD dwParaOffset;
						int cch;
						MoveToEx(hdcMem, m_rcScreen.left, 0, NULL);
						DocPart * pdp = GetPart(FALSE, m_aprgnLineInfo[m_cli - 1].ich, &pPos, &dwParaOffset);
						GetBlockValues(pdp, dwParaOffset, pPos, hdcMem, fInBlock, fInBlockP);
						if (m_pzd->IsAnsi(pdp, dwParaOffset))
						{
							pStop = (char *)pdp->rgpv[dwParaOffset] + ParaLength(pdp, dwParaOffset) - 1;
							cch = DrawLineA(hdcMem, (char *)pPos, (char *)pStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, TRUE, TRUE, fInBlock, fInBlockP);
							pPos = (char *)pPos + cch;
						}
						else
						{
							pStop = (wchar *)pdp->rgpv[dwParaOffset] + ParaLength(pdp, dwParaOffset) - 1;
							cch = DrawLineW(hdcMem, (wchar *)pPos, (wchar *)pStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, TRUE, TRUE, fInBlock, fInBlockP);
							pPos = (wchar *)pPos + cch;
						}
						if (pPos >= pStop)
						{
							// Advance to the beginning of the next paragraph
							if (ParaLength(pdp, dwParaOffset) != 1)
								cch++;
							if (++dwParaOffset >= pdp->dwParas)
							{
								dwParaOffset = 0;
								pdp = pdp->pdpNext;
							}
							if (pdp)
								pPos = pdp->rgpv[dwParaOffset];
						}
						if (pdp)
						{
							dxpLine = rect.right - g_fg.m_rcMargin.left - g_fg.m_rcMargin.right;
							MoveToEx(hdcMem, m_rcScreen.left, g_fg.m_tm.tmHeight, NULL);
							if (m_pzd->IsAnsi(pdp, dwParaOffset))
							{
								pStop = (char *)pdp->rgpv[dwParaOffset] + ParaLength(pdp, dwParaOffset) - 1;
								DrawLineA(hdcMem, (char *)pPos, (char *)pStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, TRUE, TRUE, fInBlock, fInBlockP);
							}
							else
							{
								pStop = (wchar *)pdp->rgpv[dwParaOffset] + ParaLength(pdp, dwParaOffset) - 1;
								DrawLineW(hdcMem, (wchar *)pPos, (wchar *)pStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, TRUE, TRUE, fInBlock, fInBlockP);
							}
						}
					}
					else
					{
						if (m_ichTopLine == 0)
						{
							nOffset = 0;
							continue;
						}
						// Draw the top line into the memory DC
						ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rectMem, NULL, 0, NULL);
						int dxpLine = rect.right - g_fg.m_rcMargin.left - g_fg.m_rcMargin.right;
						void * pPos;
						void * pStop;
						DWORD dwParaOffset;
						DWORD dwChar = ShowCharAtLine(m_aprgnLineInfo[0].ipr, m_aprgnLineInfo[0].ich, 1);
						DocPart * pdp = GetPart(FALSE, dwChar, &pPos, &dwParaOffset);
						MoveToEx(hdcMem, m_rcScreen.left, 0, NULL);
						GetBlockValues(pdp, dwParaOffset, pPos, hdcMem, fInBlock, fInBlockP);
						if (m_pzd->IsAnsi(pdp, dwParaOffset))
						{
							pStop = (char *)pdp->rgpv[dwParaOffset] + ParaLength(pdp, dwParaOffset) - 1;
							DrawLineA(hdcMem, (char *)pPos, (char *)pStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, TRUE, TRUE, fInBlock, fInBlockP);
						}
						else
						{
							pStop = (wchar *)pdp->rgpv[dwParaOffset] + ParaLength(pdp, dwParaOffset) - 1;
							DrawLineW(hdcMem, (wchar *)pPos, (wchar *)pStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, TRUE, TRUE, fInBlock, fInBlockP);
						}
					}
				}
				if (nOffset == g_fg.m_tm.tmHeight)
				{
					OnVScroll(nDiff < 0 ? SB_LINEUP : SB_LINEDOWN, 0, 0);
					nOffset = 0;
				}
				else
				{
					if (fOldScrollingDown != fScrollingDown)
						nOffset = - nOffset + 1;
					HideCaret(m_hwnd);
					if (nDiff > 0)
					{
						// Scroll down
						int nTop = m_rcScreen.top + ((m_cli - 1) * g_fg.m_tm.tmHeight) - nOffset;
						BitBlt(hdcScreen, 0, 0, rect.right, nTop, m_hdc, 0, nOffset, SRCCOPY);
						BitBlt(hdcScreen, 0, nTop, rect.right, g_fg.m_tm.tmHeight * 2, hdcMem, 0, 0, SRCCOPY);
						SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y - nOffset);
					}
					else
					{
						// Scroll up
						BitBlt(hdcScreen, 0, nOffset, rect.right, rect.bottom - nOffset, m_hdc, 0, 0, SRCCOPY);
						BitBlt(hdcScreen, 0, nOffset - g_fg.m_tm.tmHeight, rect.right, g_fg.m_tm.tmHeight, hdcMem, 0, 0, SRCCOPY);
						SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y + nOffset);
					}
					ShowCaret(m_hwnd);
				}
				if (!fContinue)
					Sleep(min(50, 100 - (min(nAbsDiff, 50) * 2)));
				fOldScrollingDown = fScrollingDown;
			} while (!fContinue && nOffset != 0);
		}
		if (fContinue)
			ImageList_Draw(himl, 0, hdcScreen, ptImage.x, ptImage.y, ILD_NORMAL);
		ShowCaret(m_hwnd);
		if (nAbsDiff <= 50)
			Sleep(100 - (nAbsDiff * 2));
		else if (nAbsDiff <= 150)
			Sleep(150 - nAbsDiff);
	}
	ImageList_Destroy(himl);
	DeleteObject(SelectObject(hdcMem, hbmpOld));
	SelectObject(hdcMem, hOldFont);
	DeleteDC(hdcMem);
	ReleaseDC(m_hwnd, hdcScreen);
	ReleaseCapture();

	HideCaret(m_hwnd);
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
	m_ptCaretPos = m_ptRealPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	ShowCaret(m_hwnd);
	return 0;*/
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnMouseMove(UINT nFlags, POINT pt)
{
	static bool fFirstMove = true;
	if (!m_hwnd)
		return 1;

	// TODO: Why was this here?
	//m_pzef->UpdateTextPos();

	RECT rect;
	::GetClientRect(m_hwnd, &rect);

	if (m_fSelecting)
	{
		::SetCursor(g_fg.m_hCursor[kcsrIBeam]);
		if (fFirstMove)
		{
			::HideCaret(m_hwnd);
			fFirstMove = false;
		}

		RECT rcScroll;
		int dypOffset = g_fg.m_ri.m_tm.tmHeight;
		::SetRect(&rcScroll, rect.left, rect.top + dypOffset + m_rcScreen.top, 
			rect.right, rect.bottom - dypOffset - m_rcScreen.bottom);

		if (!::PtInRect(&rcScroll, pt))
		{
			int nDifference = 0;
			if (!m_fWrap)
			{
				if (pt.x < 0)
					OnHScroll(SB_LINELEFT, 0, NULL);
				else if (pt.x > rcScroll.right)
					OnHScroll(SB_LINERIGHT, 0, NULL);
			}
			if (pt.x < 0)
				pt.x = 0;
			else if (pt.x > rcScroll.right)
				pt.x = rcScroll.right;
			if (pt.y < 0)
			{
				nDifference = -pt.y;
				OnVScroll(SB_LINEUP, 0, NULL);
				pt.y = 0;
			}
			if (pt.y > rcScroll.bottom)
			{
				nDifference = pt.y - rcScroll.bottom;
				OnVScroll(SB_LINEDOWN, 0, NULL);
				pt.y = rcScroll.bottom;
			}
			if (nDifference < 20)
			{
				if (m_mt != kmtSlow)
					::KillTimer(m_hwnd, m_mt);
				m_mt = (MouseTimer)::SetTimer(m_hwnd, kmtSlow, 100, NULL);
			}
			else if (nDifference < 80)
			{
				if (m_mt != kmtMedium)
					::KillTimer(m_hwnd, m_mt);
				m_mt = (MouseTimer)::SetTimer(m_hwnd, kmtMedium, 50, NULL);
			}
			else
			{
				if (m_mt != kmtFast)
					::KillTimer(m_hwnd, m_mt);
				m_mt = (MouseTimer)::SetTimer(m_hwnd, kmtFast, 1, NULL);
			}
		}
		else if (m_mt != kmtNone)
		{
			::KillTimer(m_hwnd, m_mt);
			m_mt = kmtNone;
		}
		
		m_ichSelLim = CharFromPoint(pt, true);
		if (m_ichSelLim >= m_ichDragStart)
		{
			m_ichSelMin = m_ichDragStart;
		}
		else
		{
			m_ichSelMin = m_ichSelLim;
			m_ichSelLim = m_ichDragStart;
		}
		HDC hdc = ::GetDC(m_hwnd);
		/*::SetTextColor(hdc, g_fg.m_cr[kclrText]);
		SetBkColor(hdc, g_fg.m_cr[kclrWindow]);*/
		DrawSelection(hdc, true);
		::ReleaseDC(m_hwnd, hdc);
		m_ptRealPos = m_ptCaretPos;
		return 1;
	}
	else if (g_fg.m_iColumnDrag != -1)
	{
		AssertPtr(g_fg.m_vpcm[g_fg.m_iColumnDrag]);
		int iOldColumn = g_fg.m_vpcm[g_fg.m_iColumnDrag]->m_iColumn;
		g_fg.SetColumnMarkerPos(m_pzef, m_hwnd, pt, m_rcScreen.left - g_fg.m_rcMargin.left);
	}
	else
	{
		fFirstMove = true;
		if ((pt.y <= knWedgeHeight || pt.y >= rect.bottom - knWedgeHeight) &&
			g_fg.GetColumnMarker(pt) != -1)
		{
			::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
			return 0;
		}
		if (m_ichSelMin != m_ichSelLim)
		{
			// See if the mouse cursor is within a selection.
			// Note that this also handles the case when the cursor is to the right
			// of the end of a line but still within the selection.
			// The other way to do this would be to look at m_hrgnSel, but this
			// doesn't handle the above case properly.
			UINT ich = CharFromPoint(pt, false);
			if (m_ichSelMin <= ich && ich < m_ichSelLim)
			{
				::SetCursor(::LoadCursor(NULL, IDC_ARROW));
				return 0;
			}
		}
		::SetCursor(g_fg.m_hCursor[kcsrIBeam]);
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnMouseWheel(UINT nFlags, short zDelta, POINT pt)
{
	RECT rect;
	::GetWindowRect(m_hwnd, &rect);
	if (::PtInRect(&rect, pt))
	{
		if (nFlags & MK_CONTROL)
		{
			LOGFONT lf;
			::GetObject(g_fg.m_hFont, sizeof(LOGFONT), &lf);
			if (zDelta > 0)
			{
				if (lf.lfHeight < -5)
					lf.lfHeight++;
			}
			else
			{
				if (lf.lfHeight > -100)
					lf.lfHeight--;
			}
			g_pzea->SetFont(::CreateFontIndirect(&lf), false);
			return 0;
		}
		if (m_cli < 3)
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
			UINT iFirstChar;
			if (zDelta > 0)
				iFirstChar = ShowCharAtLine(m_iprTopLine, m_ichTopLine, 3);
			else
				iFirstChar = m_prgli[3].ich;
			if (m_fWrap)
				iFirstChar = min(iFirstChar, m_ichMaxTopLine);
			if (m_ichTopLine != iFirstChar)
			{
				m_ichTopLine = iFirstChar;
				m_iprTopLine = m_pzd->ParaFromChar(iFirstChar);

				// Update the scrollbar
				SCROLLINFO si = {sizeof(si), SIF_POS};
				if (m_fWrap)
					si.nPos = m_ichTopLine;
				else
					si.nPos = m_iprTopLine;
				m_pzf->SetScrollInfo(this, SB_VERT, &si);

				::InvalidateRect(m_hwnd, NULL, FALSE);
				::UpdateWindow(m_hwnd);
				m_ptCaretPos = m_ptRealPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
				::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
			}
		}
	}
	else
	{
		::PostMessage(::WindowFromPoint(pt), WM_MOUSEWHEEL, MAKEWPARAM(nFlags, zDelta),
			MAKELPARAM(pt.x, pt.y));
	}
	return 0;
}


// ZZZ
/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnPaint()
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
LRESULT CFastEdit::OnSetFocus(HWND hwndLoseFocus)
{
	if (!m_hwnd || !m_pzd)
		return 1;

	// TODO: Why are we still getting an Assert in PointFromChar?
	// Force the screen to be repained before the caret is set.
	::InvalidateRect(m_hwnd, NULL, FALSE);
	::UpdateWindow(m_hwnd);
	m_ptRealPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	m_ptCaretPos = m_ptRealPos;
	::CreateCaret(m_hwnd, NULL, 2, g_fg.m_ri.m_tm.tmHeight);
	::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	::ShowCaret(m_hwnd);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnSize(UINT nType, int cx, int cy)
{
	AssertPtrN(m_pzd);
	if (!m_pzd || !m_hwnd || cx == 0 || cy == 0)
		return 1;

	RecalculateLines(cy);

	// Update scrollbars.
	SCROLLINFO si = { sizeof(si), SIF_PAGE | SIF_RANGE };
	UINT cchTotal = m_pzd->GetCharCount();
	UINT cprTotal = m_pzd->GetParaCount();
	if (m_fWrap)
	{
		m_ichMaxTopLine = ShowCharAtLine(cprTotal, cchTotal, m_cli - 2);
		if (m_ichTopLine > m_ichMaxTopLine)
		{
			m_ichTopLine = m_ichMaxTopLine;
			m_iprTopLine = m_pzd->ParaFromChar(m_ichTopLine);
		}
		si.nPage = cchTotal - m_ichMaxTopLine;
		si.nMax = m_ichMaxTopLine ? cchTotal - 1 : 0;
		m_pzf->SetScrollInfo(this, SB_VERT, &si);
	}
	else
	{
		// TODO
		/*si.nPage = m_cli;
		si.nMax = m_dwTotalParas - 1 + si.nPage;
		m_pzf->SetScrollInfo(this, SB_VERT, &si);

		si.nPage = cx;
		si.nMax = m_dwCharsInLongestLine * g_fg.m_tm.tmMaxCharWidth + si.nPage;
		m_pzf->SetScrollInfo(this, SB_HORZ, &si);*/
	}
	::InvalidateRect(m_hwnd, NULL, FALSE);
	::UpdateWindow(m_hwnd);
	m_ptRealPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	m_ptCaretPos = m_ptRealPos;
	if (::GetFocus() == m_hwnd)
		::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnTimer(UINT nIDEvent)
{
	/*if (kmtSmoothScroll == nIDEvent)
	{
		POINT point;
		GetCursorPos(&point);
		MapWindowPoints(NULL, m_hwnd, &point, 1);
		int nDiff = 0;
		static int nOldDiff = 0;
		if (point.y < m_ssi.ptImage.y)
		{
			nDiff = point.y - m_ssi.ptImage.y;
			SetCursor(m_ssi.hcurUp);
		}
		else if (point.y > m_ssi.ptImage.y + 22)
		{
			nDiff = point.y - m_ssi.ptImage.y - 22;
			SetCursor(m_ssi.hcurDown);
		}
		else
			SetCursor(m_ssi.hcurMiddle);
		if (nDiff < 0)
		{
			m_ssi.fScrollDown = FALSE;
			if (nDiff > -60)
			{
				// Scroll the screen by a fraction
				if (nDiff != nOldDiff)
				{
					nOldDiff = nDiff;
					KillTimer(m_hwnd, kmtSmoothScroll);
					if (nDiff < -40)
						SetTimer(m_hwnd, kmtSmoothScroll, 25, NULL);
						//m_ssi.nVertOffset = max(m_tm.tmHeight / 2, 1);
					else if (nDiff < -25)
						SetTimer(m_hwnd, kmtSmoothScroll, 50, NULL);
						//m_ssi.nVertOffset = max(m_tm.tmHeight / 4, 1);
					else if (nDiff < -10)
						SetTimer(m_hwnd, kmtSmoothScroll, 75, NULL);
						//m_ssi.nVertOffset = max(m_tm.tmHeight / 8, 1);
					else
						SetTimer(m_hwnd, kmtSmoothScroll, 100, NULL);
						//m_ssi.nVertOffset = max(m_tm.tmHeight / 16, 1);
				}
				m_ssi.nVertOffset = max(m_tm.tmHeight / 16, 1);
				m_rcScreen.top += m_ssi.nVertOffset;
				m_ptCaretPos.y = (m_ptRealPos.y += m_ssi.nVertOffset);
				if (m_rcScreen.top <= m_ssi.rcOldScreen.top)
				{
					HideCaret(m_hwnd);
					InvalidateRect(m_hwnd, NULL, FALSE);
					UpdateWindow(m_hwnd);
					SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
					ShowCaret(m_hwnd);
				}
				else
				{
					if (!m_ssi.fHasScrolled)
						m_rcScreen.top -= m_tm.tmHeight;
					else
						m_rcScreen.top = m_ssi.rcOldScreen.top - m_tm.tmHeight;
					if (m_ichTopLine == 0)
						m_rcScreen.top = m_ssi.rcOldScreen.top;
					else
						OnVScroll(SB_LINEUP, 0, 0);
				}
			}
			else
			{
				if (nOldDiff == 0 || nOldDiff <= -60)
				{
					KillTimer(m_hwnd, kmtSmoothScroll);
					SetTimer(m_hwnd, kmtSmoothScroll, 100, NULL);
				}
				if (nDiff < -150)
				{
					// TODO: Do something a little more intelligent here
					OnVScroll(SB_PAGEUP, 0, 0);
					OnVScroll(SB_PAGEUP, 0, 0);
				}
				else if (nDiff < -110)
					OnVScroll(SB_PAGEUP, 0, 0);
				else if (nDiff < -80)
				{
					// TODO: Do something a little more intelligent here
					OnVScroll(SB_LINEUP, 0, 0);
					OnVScroll(SB_LINEUP, 0, 0);
				}
				else
					OnVScroll(SB_LINEUP, 0, 0);
			}
		}
		else if (nDiff > 0)
		{
			if (m_ichTopLine >= m_ichMaxTopLine)
			{
				m_ssi.fHasScrolled = TRUE;
				return 0;
			}
			m_ssi.fScrollDown = TRUE;
			if (nDiff > 150)
			{
				// TODO: Do something a little more intelligent here
				OnVScroll(SB_PAGEDOWN, 0, 0);
				OnVScroll(SB_PAGEDOWN, 0, 0);
			}
			else if (nDiff > 110)
				OnVScroll(SB_LINEDOWN, 0, 0);
			else if (nDiff > 80)
			{
				// TODO: Do something a little more intelligent here
				OnVScroll(SB_LINEDOWN, 0, 0);
				OnVScroll(SB_LINEDOWN, 0, 0);
			}
			else if (nDiff > 60)
				OnVScroll(SB_LINEDOWN, 0, 0);
			else
			{
				// Scroll the screen by a fraction
				if (nDiff > 40)
					m_ssi.nVertOffset = max(m_tm.tmHeight / 2, 1);
				else if (nDiff > 25)
					m_ssi.nVertOffset = max(m_tm.tmHeight / 4, 1);
				else if (nDiff > 10)
					m_ssi.nVertOffset = max(m_tm.tmHeight / 8, 1);
				else
					m_ssi.nVertOffset = max(m_tm.tmHeight / 16, 1);
				if (m_ichTopLine > m_ichMaxTopLine && m_rcScreen.top == m_ssi.rcOldScreen.top)
				{
					m_ssi.fHasScrolled = TRUE;
					return 0;
				}
				m_rcScreen.top -= m_ssi.nVertOffset;
				m_ptCaretPos.y = (m_ptRealPos.y -= m_ssi.nVertOffset);
				if (m_rcScreen.top <= m_ssi.rcOldScreen.top - m_tm.tmHeight)
				{
					m_rcScreen.top = m_ssi.rcOldScreen.top;
					OnVScroll(SB_LINEDOWN, 0, 0);
				}
				else
				{
					HideCaret(m_hwnd);
					InvalidateRect(m_hwnd, NULL, FALSE);
					UpdateWindow(m_hwnd);
					SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
					ShowCaret(m_hwnd);
				}
			}
		}
		if (nDiff)
			m_ssi.fHasScrolled = TRUE;
		return 0;
	}*/

	POINT point;
	::GetCursorPos(&point);
	RECT rect;
	::GetWindowRect(m_hwnd, &rect);
	point.x -= rect.left;
	point.y -= rect.top;
	return OnMouseMove(0, point);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CFastEdit::OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScroll)
{
	AssertPtr(m_pzd);
	if (!m_hwnd)
		return 1;

	SCROLLINFO si = { sizeof(SCROLLINFO) };
	UINT iNewFirstPara = m_iprTopLine;
	UINT iNewFirstChar = m_ichTopLine;
	//static UINT s_iFirstChar = 0;
	static HWND s_hwndToolTip = NULL;

	Assert(!s_hwndToolTip || nSBCode == SB_THUMBTRACK || nSBCode == SB_THUMBPOSITION ||
		nSBCode == SB_ENDSCROLL);
	switch (nSBCode)
	{
		case SB_LINEUP:
			if (m_ichTopLine == 0)
				return 0;
			iNewFirstChar = ShowCharAtLine(m_iprTopLine, m_ichTopLine, 1);
			iNewFirstPara = m_pzd->ParaFromChar(iNewFirstChar);
			break;

		case SB_LINEDOWN:
			if (m_ichTopLine >= m_ichMaxTopLine)
				return 0;
			iNewFirstChar = m_prgli[1].ich;
			iNewFirstPara = m_prgli[1].ipr;
			break;

		case SB_PAGEUP:
			iNewFirstChar = ShowCharAtLine(m_iprTopLine, m_ichTopLine, m_cli - 2);
			iNewFirstPara = m_pzd->ParaFromChar(iNewFirstChar);
			break;

		case SB_PAGEDOWN:
			//if (m_aprgnLineInfo[m_cli - 1].ipr >= m_pzd->GetParaCount())
			// TODO: Make sure this works.
			if (m_prgli[m_cli - 1].ich >= m_pzd->GetCharCount())
				return 0;
			iNewFirstChar = min(m_ichMaxTopLine, m_prgli[m_cli - 2].ich);
			iNewFirstPara = m_pzd->ParaFromChar(iNewFirstChar);
			break;

		//case SB_THUMBPOSITION:
			// TODO: Does this need to be here?

		case SB_THUMBTRACK:
			si.fMask = SIF_PAGE | SIF_TRACKPOS;
			m_pzf->GetScrollInfo(this, SB_VERT, &si);
			//if (m_fWrap)
			{
				iNewFirstChar = ShowCharAtLine(m_pzd->ParaFromChar(si.nTrackPos), si.nTrackPos, 0);
				iNewFirstPara = m_pzd->ParaFromChar(iNewFirstChar);
			}
			/*else
			{
				iNewFirstPara = si.nTrackPos;
				iNewFirstChar = m_pzd->CharFromPara(iNewFirstPara);
			}*/
			if (!s_hwndToolTip)
			{
				// This is the first message that says we are starting to track the thumb,
				// so create the tooltip now.
				s_hwndToolTip = ::CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP,
					0, 0, 0, 0, NULL, NULL, g_fg.m_hinst, NULL);
				if (s_hwndToolTip)
				{
					TOOLINFO ti = {sizeof(ti), TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE};
					ti.hwnd = m_pzef->GetHwnd();
					ti.uId = (UINT)m_hwnd;
					ti.hinst = g_fg.m_hinst;
					ti.lpszText = "-1";
					::SendMessage(s_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
					// Create the tooltip offscreen so we can move it to the right
					// position later once we know how big it is.
					::SendMessage(s_hwndToolTip, TTM_TRACKPOSITION, 0, MAKELPARAM(-50000, -50000));
					// Activate/show the tooltip.
					::SendMessage(s_hwndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
				}
			}
			if (s_hwndToolTip)
			{
				// Update the text in the tooltip.
				char szText[20];
				TOOLINFO ti = {sizeof(ti)};
				_itoa_s(iNewFirstPara + 1, szText, 10);
				ti.lpszText = szText;
				ti.hwnd = m_pzef->GetHwnd();
				ti.uId = (UINT)m_hwnd;
				::SendMessage(s_hwndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

				// Move the tooltip to the correct position.
				RECT rect;
				::GetClientRect(m_hwnd, &rect);
				::MapWindowPoints(m_hwnd, NULL, (POINT *)&rect, 2);
				POINT pt;
				::GetCursorPos(&pt);
				pt.x = rect.right;
				::GetWindowRect(s_hwndToolTip, &rect);
				pt.y -= (rect.bottom - rect.top) / 2;
				pt.x -= rect.right - rect.left + 1;
				::SendMessage(s_hwndToolTip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));
			}
			break;

		case SB_ENDSCROLL:
			DestroyWindow(s_hwndToolTip);
			s_hwndToolTip = NULL;
			break;

		default:
			return 0;
	}

	if (iNewFirstChar == m_ichTopLine)
		return 0;
	m_ichTopLine = iNewFirstChar;
	m_iprTopLine = iNewFirstPara;

	// Update scrollbar
	si.fMask = SIF_POS | SIF_PAGE;
	si.nPos = iNewFirstChar;
	m_ichMaxTopLine = ShowCharAtLine(m_pzd->GetParaCount(), m_pzd->GetCharCount(), m_cli - 2);
	si.nPage = m_pzd->GetCharCount() - m_ichMaxTopLine;
	m_pzf->SetScrollInfo(this, SB_VERT, &si);

	::HideCaret(m_hwnd);
	// TODO: These aren't working properly yet.
	/*if (nSBCode == SB_LINEUP)
	{
		memmove(m_aprgnLineInfo + 1, m_aprgnLineInfo, sizeof(UINT[4]) * m_cli - 1);
		m_ptCaretPos.y += g_fg.m_tm.tmHeight;

		RECT rect;
		GetClientRect(m_hwnd, &rect);
		int dxpLine = rect.right - g_fg.m_rcMargin.left - g_fg.m_rcMargin.right;

		ScrollDC(m_hdc, 0, g_fg.m_tm.tmHeight, NULL, NULL, NULL, NULL);
		RECT rectLine = {0, 0, rect.right, g_fg.m_rcMargin.top + g_fg.m_tm.tmHeight};
		ExtTextOut(m_hdc, 0, 0, ETO_OPAQUE, &rectLine, NULL, 0, NULL);
		MoveToEx(m_hdc, m_rcScreen.left, m_rcScreen.top, NULL);
		bool fInBlock;
		bool fInBlockP;
		void * pv;
		void * pvStop;
		UINT ipr;
		DocPart * pdp = m_pzd->GetPart(m_ichTopLine, false, &pv, &ipr);
		GetBlockValues(pdp, ipr, pv, m_hdc, fInBlock, fInBlockP);
		if (m_pzd->IsAnsi(pdp, ipr))
		{
			pvStop = (char *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
			DrawLineA(m_hdc, (char *)pv, (char *)pvStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left,
				g_fg.m_rgnCharWidth, m_fWrap, true, true, fInBlock, fInBlockP);
		}
		else
		{
			pvStop = (wchar *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
			DrawLineW(m_hdc, (wchar *)pv, (wchar *)pvStop, &dxpLine, g_fg.m_dxpTab, m_rcScreen.left,
				g_fg.m_rgnCharWidth, m_fWrap, true, true, fInBlock, fInBlockP);
		}
		m_aprgnLineInfo[0][klpWidth] = dxpLine;
		m_aprgnLineInfo[0].ipr = m_iprTopLine;
		m_aprgnLineInfo[0].ich = m_ichTopLine;

		HDC hdcScreen = GetDC(m_hwnd);
		DrawSelection(hdcScreen);
		ReleaseDC(m_hwnd, hdcScreen);
	}
	else if (nSBCode == SB_LINEDOWN)
	{
		memmove(m_aprgnLineInfo, m_aprgnLineInfo + 1, sizeof(UINT[4]) * m_cli);
		m_ptCaretPos.y -= g_fg.m_tm.tmHeight;

		// TODO: Do something different if m_cli < 3
		UINT ich = m_aprgnLineInfo[m_cli - 3].ich;
		UINT ipr = m_aprgnLineInfo[m_cli - 3].ipr;
		RECT rect;
		GetClientRect(m_hwnd, &rect);

		ScrollDC(m_hdc, 0, -g_fg.m_tm.tmHeight, NULL, NULL, NULL, NULL);
		RECT rectLine = {0, g_fg.m_rcMargin.top + (m_cli - 3) * g_fg.m_tm.tmHeight, rect.right, 0};
		rectLine.bottom = rectLine.top + g_fg.m_tm.tmHeight * 3;
		ExtTextOut(m_hdc, 0, 0, ETO_OPAQUE, &rectLine, NULL, 0, NULL);
		bool fInBlock;
		bool fInBlockP;
		void * pv;
		void * pvStop;
		int cch;
		UINT iprInPart;
		DocPart * pdp = m_pzd->GetPart(ich, false, &pv, &iprInPart);
		GetBlockValues(pdp, iprInPart, pv, m_hdc, fInBlock, fInBlockP);

		// The first line drawn is the old last line (to make sure the whole line is shown.
		// The second line is the partial last visible line.
		// The third line is measured but not drawn.
		bool fNotLast = true;
		int nOldLineWidth = rect.right - g_fg.m_rcMargin.left - g_fg.m_rcMargin.right;
		for (int i = 0; i < 3; i++)
		{
			if (!pdp)
				break;
			MoveToEx(m_hdc, m_rcScreen.left, rectLine.top, NULL);
			rectLine.top += g_fg.m_tm.tmHeight;
			int dxpLine = nOldLineWidth;
			if (m_pzd->IsAnsi(pdp, iprInPart))
			{
				pvStop = (char *)pdp->rgpv[iprInPart] + PrintCharsInPara(pdp, iprInPart);
				cch = DrawLineA(m_hdc, (char *)pv, (char *)pvStop, &dxpLine, g_fg.m_dxpTab,
					m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, fNotLast, fNotLast, fInBlock, fInBlockP);
				pv = (char *)pv + cch;
			}
			else
			{
				pvStop = (wchar *)pdp->rgpv[iprInPart] + PrintCharsInPara(pdp, iprInPart);
				cch = DrawLineW(m_hdc, (wchar *)pv, (wchar *)pvStop, &dxpLine, g_fg.m_dxpTab,
					m_rcScreen.left, g_fg.m_rgnCharWidth, m_fWrap, fNotLast, fNotLast, fInBlock, fInBlockP);
				pv = (wchar *)pv + cch;
			}
			m_aprgnLineInfo[m_cli - 3 + i][klpWidth] = dxpLine;
			m_aprgnLineInfo[m_cli - 3 + i].ich = ich;
			m_aprgnLineInfo[m_cli - 3 + i].ipr = ipr;
			ich += cch;
			if (pv >= pvStop)
			{
				ich += CharsAtEnd(pdp, iprInPart);
				ipr++;
				// TODO: Is this right?
				//if (ParaLength(pdp, dwParaOffset) != 1)
					//cch++;
				iprInPart++;
				if (i < 2)
				{
					if (iprInPart >= pdp->cpr)
					{
						iprInPart = 0;
						pdp = pdp->pdpNext;
					}
					if (pdp)
						pv = pdp->rgpv[iprInPart];
				}
				else
				{
					fNotLast = true;
				}
			}
			//if (i == 2)
				//m_aprgnLineInfo[m_cli - 11[klpWidth] = dxpLine;
			//ich += cch;
		}
		if (i == 3)
		{
			m_aprgnLineInfo[m_cli].ich = ich;
			m_aprgnLineInfo[m_cli].ipr = ipr;
		}

		HDC hdc = GetDC(m_hwnd);
		DrawSelection(hdc);
		ReleaseDC(m_hwnd, hdc);
	}
	else*/
	{
		::InvalidateRect(m_hwnd, NULL, FALSE);
		::UpdateWindow(m_hwnd);
		m_ptCaretPos = PointFromChar(m_ichDragStart == m_ichSelMin ? m_ichSelLim : m_ichSelMin);
	}
	::SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	m_ptRealPos.y = m_ptCaretPos.y;
	::ShowCaret(m_hwnd);
	return 0;
}
